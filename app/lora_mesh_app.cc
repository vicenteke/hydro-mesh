#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
#include <utility/string.h>
#include <utility/handler.h>

#include <lora_mesh.h>
#include "lora_mesh/include/index.h"

// #include <thread.h>

// #include <tstp.h>

using namespace EPOS;

OStream cout;

// End Devices functions ---------------------------
int read_sensor(char result[], Level_Sensor & level, Turbidity_Sensor & turb,
                Pluviometric_Sensor & pluv, EndDevice_Lora_Mesh & lora)
{

    MessagesHandler msg;

    level.enable();
    turb.enable();

    Alarm::delay(2000000);

    // msg.setLvl(level.sample());
    // msg.setTur(turb.sample());
    // msg.setPlu(pluv.countAndReset());
    msg.setLvl(1);
    msg.setTur(1);
    msg.setPlu(1);
    msg.setUsr(lora.id());

    level.disable();
    turb.disable();

    msg.setTime(lora.timer()->currentTime()); // Sends elapsed seconds since last read
    msg.setX(lora.x());
	msg.setY(lora.y());
	msg.setZ(lora.z());

    msg.toString(result);
    msg.dump();

    return sizeof(DBEntry);
}

void main_func_ED() {

    //level sensor objects, lToggle is the relay pin, and lAdc is the ADC conversor
    GPIO lToggle = GPIO{'B', 0, GPIO::OUT};
    auto lAdc = ADC{ADC::SINGLE_ENDED_ADC2};
    auto level = Level_Sensor{lAdc, lToggle};
    level.disable();

    //turbidity sensor objects, tToggle is the relay pin
    auto tAdc = ADC{ADC::SINGLE_ENDED_ADC4}; //For Low range (current)
    auto tAdc2 = ADC{ADC::SINGLE_ENDED_ADC5};  //For High range (voltage)
    auto tInfrared = GPIO{'B', 2, GPIO::OUT};
    auto tToggle = GPIO{'B', 3, GPIO::OUT};
    auto turbidity = Turbidity_Sensor{tAdc, tAdc2, tToggle, tInfrared};
    turbidity.disable();

    //pluviometric sensor objects, pToggle is the relay pin
    auto pToggle = GPIO{'B', 1, GPIO::OUT};
    auto pInput = GPIO{'B', 4, GPIO::IN};
    auto pluviometric = Pluviometric_Sensor{pInput, pToggle};

    // EndDevice_Lora_Mesh lora = EndDevice_Lora_Mesh(1);
    EndDevice_Lora_Mesh lora = EndDevice_Lora_Mesh(1, 37335860, -42829040, -28887290);
    Interface interface(true);
    char buf[sizeof(DBEntry)];

    Alarm::delay(2000000);
    while(1) {
        read_sensor(buf, level, turbidity, pluviometric, lora);
        lora.send(buf, sizeof(DBEntry));
        interface.show_life();
        Alarm::delay(10000000);
        cout << "ED ts: " << lora.timer()->currentTime() << endl;
        Alarm::delay(10000000);
    }
}

// Gateway functions -------------------------------
int store_in_flash(int id, char data[]) {

    Interface interface(true);
    MessagesHandler msg;
    Sender send(&interface, &msg);

    msg.setTime(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	msg.setLvl(data[4] | (data[5] << 8));
	msg.setTur(data[6] | (data[7] << 8));
	msg.setPlu((unsigned char) data[8]);
	msg.setUsr(data[9]);
	msg.setX(data[10] | (data[11] << 8) | (data[12] << 16) | (data[13] << 24));
	msg.setY(data[14] | (data[15] << 8) | (data[16] << 16) | (data[17] << 24));
	msg.setZ(data[18] | (data[19] << 8) | (data[20] << 16) | (data[21] << 24));

    send.send_or_store();
    interface.blink_success(Interface::SUCCESS::MESSAGESENT);
    return 0;
}

int flash_to_smart(Sender * sender) {
    while(1) {
        Alarm::delay(7000000);
        cout << "real ts: ";
        sender->getTimestamp();
        Alarm::delay(500000);
        sender->try_sending_queue();
    }
    return 0;
}

void main_func_GW() {

    Interface interface(true);
    interface.show_life();

    Alarm::delay(1000000); //Necessary to use minicom

    // cout << "------------ LoRa Mesh Program: Gateway ------------\n";

    MessagesHandler msg;
    Sender sender(&interface, &msg);
    sender.init();

	Gateway_Lora_Mesh lora = Gateway_Lora_Mesh(&store_in_flash);
	// Gateway_Lora_Mesh lora = Gateway_Lora_Mesh();

    // Thread flash_smart_t = Thread(&flash_to_smart, &sender);

    // Serial_Link serial = Serial_Link();
    // Alarm::delay(500000);

    // DB_Series series_data;
    // series_data.version = STATIC_VERSION;
    // series_data.unit = TSTP::Unit::Length; // Water_Level
    // series_data.x = 37331860;
    // series_data.y = -42829040;
    // series_data.z = -28887290;
    // series_data.r = 10 * 100;
    // series_data.t0 = 0;
    // series_data.t1 = -1;
    // series_data.dev = 1;

    // sender.serial()->createSeries(series_data);
    // sender.serial()->finishSeries(series_data);

    // DB_Record record_data;
    // record_data.version = STATIC_VERSION;
    // record_data.unit = TSTP::Unit::Amount_of_Substance; // Err 400
    // // record_data.unit = TSTP::Unit::Length; // Water_Level
    // record_data.value = 2.2;
    // record_data.error = 1;
    // record_data.confidence = 3;
    // record_data.x = 37331860;
    // record_data.y = -42829040;
    // record_data.z = -28887290;
    // record_data.t = lora.timer()->currentTime() * 1000;
    // record_data.dev = 1;

    // char res = sender.serial()->sendRecord(record_data);

    while (1) {
        for (int i = 0; i < 60 * 10; ++i) { // 60 * 10 * ~6 = update TS every +/- 1 hour
            Alarm::delay(6000000);
            // lora.sendToAll("GW on");

            interface.show_life();
            cout << "GW ts: " << lora.timer()->currentTime() << '\n';
            cout << "real ts: ";
            sender.getTimestamp();
            Alarm::delay(4000000);
            // lora.send(1, "hi");

            lora.cleanThreads();
            sender.try_sending_queue();
        }
        lora.timer()->epoch(sender.getTimestamp());
        lora.sendTimestamp(Lora_Mesh::BROADCAST_ID);
    }
}

int main()
{
    main_func_GW();

    return 0;
}
