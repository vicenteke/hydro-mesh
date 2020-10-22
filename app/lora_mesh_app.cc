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

UART uart(1, 9600, 8, 0, 1);

int keepReceivingGateWay() {
    // Keeps looking for entries in _uart and echos any received message

    	cout << "Gateway waiting for messages...\n\n";

        char str[45];
        str[0] = '\0';
        int len = 0;
        char buf, id[2];
        buf = '0';
        id[0] = id[1] = 10;

    	while (1) {
    		while (!uart.ready_to_get());
            id[1] = uart.get();
            id[0] = uart.get();
            Alarm::delay((int)(6000));
    		while(uart.ready_to_get()) {
    			buf = uart.get();
                str[len++] = buf;
    			Alarm::delay((int)(500)); // 500 for 115200
    		}
            str[len] = '\0';
    		cout << "Received from " << (char)(id[0] + '0') << (char)(id[1] + '0') << ": " << str << '\n';
            str[0] = '\0';
            len = 0;
            id[0] = id[1] = 10;
    	}

    	return 0;
}

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

    cout << "------------ LoRa Mesh Program: Gateway ------------\n";

	// LoraMesh lora = LoraMesh();
    // lora.localRead();

    // cout << "UID: " << lora.uid()
    //     << "\nID: " << lora.id()
    //     << "\nNET: " << lora.net()
    //     << "\nSF: " << lora.sf() << '\n';

    Thread receiver = Thread(&keepReceivingGateWay);
    // Thread ping     = Thread(&pingUART);

    int status_receiver = receiver.join();
    // int status_ping     = ping.join();

	while (1) {

	}

    return 0;
}
