#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
// #include <machine/cortex/emote3_gptm.h>
#include <machine/cortex/ic.h>
#include <utility/vector.h>
// #include <periodic_thread.h>
#include <thread.h>
#include <cstdint>
#include <utility/string.h>

#include <utility/handler.h>

#include "LoraMesh/include/loraMesh.hpp"
// #include "LoraMesh/include/loraMesh.h"

using namespace EPOS;

OStream cout;

void justAnotherPrint(int id, char * msg) {
    cout << id << " > " << msg << '\n';
}

int main()
{
	Alarm::delay(2000000); //Necessary to use minicom

    cout << "------------ LoRa Mesh Program: Gateway ------------\n";

	GatewayLoraMesh gateway = GatewayLoraMesh(&justAnotherPrint);
    // GPIO _interrupt('C', 3, GPIO::IN);
    // _interrupt.handler(GatewayLoraMesh::uartHandler, GPIO::FALLING);
	// GatewayLoraMesh gateway = GatewayLoraMesh();

    // char str[] = "GW ON";
    // gateway.sendToAll(str);

    while (1) {
        // gateway.sendToAll(str);
        // // gateway.send(1, str);
        // Alarm::delay(6000000);

        // gateway.send(1, "hi");
        // Alarm::delay(60000000);
    }

    // cout << "------------ LoRa Mesh Program: EndDevice ------------\n";
    //
	// EndDeviceLoraMesh endDevice = EndDeviceLoraMesh(2);
	// // EndDeviceLoraMesh endDevice = EndDeviceLoraMesh(2, &justAnotherPrint);
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
