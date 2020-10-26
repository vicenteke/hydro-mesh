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

#include "LoraMesh/include/loraMesh.hpp"
// #include "LoraMesh/include/loraMesh.h"

using namespace EPOS;

OStream cout;
OStream LoraMesh::cout;
UART LoraMesh::_transparent;
// void GatewayLoraMesh::uartHandler(const unsigned int &);

UART uart(1, 9600, 8, 0, 1);

int main()
{
	Alarm::delay(2000000); //Necessary to use minicom

    cout << "------------ LoRa Mesh Program: Gateway ------------\n";

	GatewayLoraMesh gateway = GatewayLoraMesh();

    char str[] = "pong";

    while (1) {
        gateway.sendToAll(str);
        // gateway.send(1, str);
        Alarm::delay(4000000);
    }

    // cout << "------------ LoRa Mesh Program: EndDevice ------------\n";
    //
	// EndDeviceLoraMesh endDevice = EndDeviceLoraMesh(2);
    // // endDevice.stopReceiver();
    //
    // char str[] = "pang";
    //
	// while (1) {
    //     endDevice.send(str);
    //     Alarm::delay(8000000);
	// }

    return 0;
}
