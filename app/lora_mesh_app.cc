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

UART uart(1, 9600, 8, 0, 1);

int pingUART() {

    char str[] = "123456789";
    int len = 9; // No problems with len = 9 in same eMote. Problems with 10 after five/six messages.
                 // Some problems with 2 eMotes actually.

    while (1) {
        Alarm::delay(2000000);
        // str[len++] = (char)(len + 'a');
        // str[len]   = '\0';
        for (int j = 0 ; j < len ; j++) {
            uart.put(str[j]);
        }
        // cout << "put " << str << "\n";
    }

    return 0;
}

int main()
{
	Alarm::delay(2000000); //Necessary to use minicom

    // cout << "------------ LoRa Mesh Program: Gateway ------------\n";
    //
	// GatewayLoraMesh gateway = GatewayLoraMesh();

    cout << "------------ LoRa Mesh Program: EndDevice ------------\n";

	EndDeviceLoraMesh endDevice = EndDeviceLoraMesh(1);
    endDevice.stopReceiver();

    char str[] = "ping";

	while (1) {
        Alarm::delay(3000000);
        endDevice.send(str);
	}

    return 0;
}
