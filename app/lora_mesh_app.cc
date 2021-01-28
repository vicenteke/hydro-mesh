/* TO DO
 *
 * Send series to create.
 *      Use Series_Logger to control.
 */

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

#include <tstp.h>

using namespace EPOS;

OStream cout;

/*int main() {

    Interface interface(true);
    interface.show_life();

    Alarm::delay(1000000); //Necessary to use minicom

    MessagesHandler msg;
    msg.setLvl(0);
    msg.setTur(0);
    msg.setPlu(0);
    msg.setUsr(0);

    Sender sender(&interface, &msg);
    sender.init();

    //try sensing data acquired for 3 times, if it fails will store on the flash memory
    sender.send_or_store();

    //check if there is any data on the flash memory, if there is it will send this data
    sender.try_sending_queue();

    cout << "\nThat's all, folks!\n";

    while(1){
        interface.blink_error(Interface::ERROR::NONETWORK);
        Alarm::delay(2000000);
    }
    return 0;
}*/

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
    EndDevice_Lora_Mesh lora = EndDevice_Lora_Mesh(1, 37331860, -42829040, -28887290);
    Interface interface(true);
    char buf[sizeof(DBEntry)];

    while(1) {
        Alarm::delay(7000000);
        cout << "ED ts: " << lora.timer()->currentTime() << endl;
        read_sensor(buf, level, turbidity, pluviometric, lora);
        lora.send(buf, sizeof(DBEntry));
        interface.show_life();
    }
}

void func2() {
    Interface interface(true);
    char data[10];
    MessagesHandler msg, rec;
    interface.show_life();
    while(true) {
        Alarm::delay(2000000);
        interface.show_life();

        // msg.setLvl(level.sample());
        // msg.setTur(turb.sample());
        // msg.setPlu(pluv.countAndReset());
        msg.setLvl(1);
        msg.setTur(1);
        msg.setPlu(1);

        msg.setUsr(0); // ?
        msg.setTime(0xFFFFFF); // Sends elapsed seconds since last read

        msg.toString(data);

        rec.setTime(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
    	rec.setLvl(data[4] | (data[5] << 8));
    	rec.setTur(data[6] | (data[7] << 8));
    	rec.setPlu(data[8]);
    	rec.setUsr(data[9]);
    	// rec.setPlu((unsigned char) data[8]);
    	// rec.setUsr((unsigned char) data[9]);

        rec.dump();
    }
}

// Gateway functions -------------------------------
void store_in_flash(int id, char data[]) {

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
}

void main_func_GW() {

    Interface interface(true);
    interface.show_life();

    Alarm::delay(1000000); //Necessary to use minicom

    // cout << "------------ LoRa Mesh Program: Gateway ------------\n";

    MessagesHandler msg;
    Sender sender(&interface, &msg);
    sender.init();

	Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh(&store_in_flash);
	// Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh(&anotherPrint);

    // Serial_Link serial = Serial_Link();
    Alarm::delay(500000);

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
    // record_data.t = gateway.timer()->currentTime() * 1000;
    // record_data.dev = 1;

    // char res = sender.serial()->sendRecord(record_data);


    while (1) {
        Alarm::delay(2000000);
        // gateway.sendToAll("GW on");

        // cout << "GW ts: " << gateway.timer()->currentTime() << '\n';
        // interface.show_life();
        Alarm::delay(4000000);
        // gateway.send(1, "hi");

        sender.try_sending_queue();
    }
}

int main()
{
    main_func_ED();
    // Interface interface(true);
    // interface.show_life();
    //
	// Alarm::delay(1000000); //Necessary to use minicom
    //
    // cout << "------------ LoRa Mesh Program: Gateway ------------\n";
    //
    // MessagesHandler msg;
    // Sender sender(&interface, &msg);
    // sender.init();
    //
	// Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh(&store_in_flash);
	// // Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh();
    //
    // // char str[] = "GW ON";
    // // gateway.sendToAll(str);
    //
    // // Alarm::delay(5000000);
    // // gateway.getNodesInNet();
    //
    // while (1) {
    //     Alarm::delay(2000000);
    //     // gateway.sendToAll(str);
    //
    //     // cout << gateway.timer()->currentTime(); << '\n';
    //     interface.show_life();
    //     Alarm::delay(4000000);
    //     // gateway.send(1, "hi");
    //
    //     sender.try_sending_queue();
    // }

    // cout << "------------ LoRa Mesh Program: EndDevice ------------\n";
    //
	// EndDevice_Lora_Mesh endDevice = EndDevice_Lora_Mesh(1);
	// // EndDevice_Lora_Mesh endDevice = EndDevice_Lora_Mesh(1, &anotherPrint);
    // // endDevice.stopReceiver();
    //
    // char str[] = "ping";
    //
	// while (1) {
    //     endDevice.send(str);
    //     Alarm::delay(10000000);
    //     interface.show_life();
	// }

    // cout << "------------ LoRa Mesh Program: EndDevice ------------\n";
    //
	// EndDevice_Lora_Mesh endDevice = EndDevice_Lora_Mesh(2);
	// // EndDevice_Lora_Mesh endDevice = EndDevice_Lora_Mesh(2, &justAnotherPrint);
    // // endDevice.stopReceiver();
    //
    // char str[] = "12345";
    // // char str[] = "123456789012345";
    //
	// while (1) {
    //     endDevice.send(str);
    //     Alarm::delay(8000000);
    //     interface.show_life();
	// }

    // Get epoch time from serial
    // USB io;
    // TSTP::Time epoch = 0;
    // TSTP::Time start = 0;
    // unsigned int bytes = 0;
    // char c = 0;
    // io.put('@');
    // do {
    //     while(!io.ready_to_get());
    //     c = io.get();
    // } while(c != 'X');
    // while(bytes < sizeof(TSTP::Time)){
    //     while(!io.ready_to_get());
    //     c = io.get();
    //     epoch |= (static_cast<unsigned long long>(c) << ((bytes++)*8));
    // }
    // // TSTP::epoch(epoch);
    // // User_Timer timer(0, 0xFFFFFFFF, 0, true);
    // // User_Timer timer(0, 1000000, &handler, true);
    // GW_Timer timer = GW_Timer();
    // start = timer.epoch();
    // // cout << "Epoch set to: " << epoch/1000000 << endl;
    // Alarm::delay(5000000);
    // interface.show_life();
    //
    //
    // while (true) {
    //     epoch = 0;
    //     bytes = 0;
    //     c = 0;
    //     io.put('@');
    //     do {
    //         while(!io.ready_to_get());
    //         c = io.get();
    //     } while(c != 'X');
    //     while(bytes < 8){
    //     // while(bytes < sizeof(TSTP::Time)){
    //         while(!io.ready_to_get());
    //         c = io.get();
    //         epoch |= (static_cast<unsigned long long>(c) << ((bytes++)*8));
    //     }
    //     cout << "PC: " << (epoch - start) / 1000000 << '\n'
    //      << "eMote: " << timer.count() << '\n';
    //     interface.show_life();
    //     Alarm::delay(4000000);
    // }

    return 0;
}
