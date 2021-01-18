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

void store_in_flash(int id, char* data) {

    Interface interface(true);
    MessagesHandler msg;
    Sender send(&interface, &msg);

    msg.setLvl(id);
    msg.setTur(0);
    msg.setPlu(0);
    msg.setUsr(0);
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
