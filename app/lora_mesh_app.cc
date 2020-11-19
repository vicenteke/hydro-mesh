#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
#include <utility/string.h>
#include <utility/handler.h>

// #include "LoraMesh/include/loraMesh.hpp"
#include <lora_mesh.h>

// Other includes that are used in hydro_thread_app
// #include <semaphore.h>
// #include <system/meta.h>
// #include <adc.h>
// #include <machine/cortex/emote3_gptm.h>: eMote3_GPTM::delay = Alarm::delay ??
// #include <periodic_thread.h>: not working, problem with Scheduler
// #include <flash.h>: no existence
// #include <machine/cortex_m/emote3_flash.h>: no existence

using namespace EPOS;

OStream cout;

void justAnotherPrint(int id, char * msg) {
    cout << id << " > " << msg << '\n';
}

int main()
{
	Alarm::delay(1000000); //Necessary to use minicom

    cout << "------------ LoRa Mesh Program: Gateway ------------\n";

	// Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh(&justAnotherPrint);
	Gateway_Lora_Mesh gateway = Gateway_Lora_Mesh();

    // char str[] = "GW ON";
    // gateway.sendToAll(str);

    Alarm::delay(5000000);
    gateway.getNodesInNet();

    while (1) {
        // gateway.sendToAll(str);
        // Alarm::delay(4000000);

        // gateway.send(1, "hi");
        // Alarm::delay(4000000);
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
	// }

    // cout << "------------ LoRa Mesh Program: EndDevice ------------\n";
    //
	// EndDevice_Lora_Mesh endDevice = EndDevice_Lora_Mesh(2);
	// // EndDevice_Lora_Mesh endDevice = EndDevice_Lora_Mesh(2, &justAnotherPrint);
    // // endDevice.stopReceiver();
    //
    // char str[] = "123456789012345";
    //
	// while (1) {
    //     endDevice.send(str);
    //     Alarm::delay(8000000);
	// }

    return 0;
}
