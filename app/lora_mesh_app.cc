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
char* read_sensor(Level_Sensor & level, Turbidity_Sensor & turb, Pluviometric_Sensor & pluv) {

    void * result;
    MessageHandler msg;

    level.enable();
    turb.enable();

    Alarm::delay(2000000);

    msg.setLvl(level.sample());
    msg.setTur(turb.sample());
    msg.setPlu(pluv.countAndReset());

    msg.setUsr(0); // ?
    msg.setTime(0); // ?

    level.disable();
    turb.disable();

    msg.build(result);
    return (char*) result;
}

void main_func_ED() {

    //level sensor objects, lToggle is the relay pin, and lAdc is the ADC conversor
    GPIO lToggle = GPIO{'B', 0, GPIO::OUTPUT};
    auto lAdc = ADC{ADC::SINGLE_ENDED_ADC2};
    auto level = Level_Sensor{lAdc, lToggle};
    level.disable();

    //turbidity sensor objects, tToggle is the relay pin
    auto tAdc = ADC{ADC::SINGLE_ENDED_ADC4}; //For Low range (current)
    auto tAdc2 = ADC{ADC::SINGLE_ENDED_ADC5};  //For High range (voltage)
    auto tInfrared = GPIO{'B', 2, GPIO::OUTPUT};
    auto tToggle = GPIO{'B', 3, GPIO::OUTPUT};
    auto turbidity = Turbidity_Sensor{tAdc, tAdc2, tToggle, tInfrared};
    turbidity.disable();

    //pluviometric sensor objects, pToggle is the relay pin
    auto pToggle = GPIO{'B', 1, GPIO::OUTPUT};
    auto pInput = GPIO{'B', 4, GPIO::INPUT};
    auto pluviometric = Pluviometric_Sensor{pInput, pToggle};

    EndDevice_Lora_Mesh lora = EndDevice_Lora_Mesh(1);
    Interface interface(true);

    while(1) {
        Alarm::delay(10000000);
        lora.send(read_sensor(&level, &turbidity, &pluviometric));
        interface.show_life();
    }
}

// Gateway functions -------------------------------
void store_in_flash(int id, char* data) {

    Interface interface(true);
    MessagesHandler msg;
    Sender send(&interface, &msg);

    msg.setLvl(id);
    msg.setTur(id);
    msg.setPlu(id);
    msg.setUsr(id);
    // msg.setTime(TSTP::absolute(TSTP::now()) / 1000000);

    // Idea Note: no GW usar setup() de smart.h p/ pegar epoch. ED pedem o tempo p/ GW
    //            e gravam o instante atual; quando recebem da GW fazem
    //            recebido + (instante atual - inst. salvo) / 2. Depois deve ser possível
    //            obter timestamp com TSTP::absolute(TSTP::now()) ou algo do tipo.

    // msg.setTime(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	// msg.setLvl(data[4] | (data[5] << 8));
	// msg.setTur(data[6] | (data[7] << 8));
	// msg.setPlu((unsigned char) data[8]);
	// msg.setUsr((unsigned char) data[9]);

    send.send_or_store();
    // interface.blink_success(Interface::SUCCESS::MESSAGESENT);
}

int main()
{
    Interface interface(true);
    interface.show_life();

	Alarm::delay(1000000); //Necessary to use minicom

    cout << "------------ LoRa Mesh Program: Gateway ------------\n";

    MessagesHandler msg;
    Sender sender(&interface, &msg);
    sender.init();

	Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh(&store_in_flash);
	// Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh();

    // char str[] = "GW ON";
    // gateway.sendToAll(str);

    // Alarm::delay(5000000);
    // gateway.getNodesInNet();

    while (1) {
        Alarm::delay(2000000);
        // gateway.sendToAll(str);

        interface.show_life();
        Alarm::delay(4000000);
        // gateway.send(1, "hi");

        sender.try_sending_queue();
    }

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

    return 0;
}
