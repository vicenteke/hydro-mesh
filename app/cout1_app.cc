#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
#include <machine/cortex_m/emote3_gptm.h>
#include <machine/cortex_m/ic.h>
#include <utility/vector.h>
#include <periodic_thread.h>

#include <cstdint>

using namespace EPOS;

OStream cout;

// UART uart = UART(1);



int main()
{
	eMote3_GPTM::delay(5000000); //Necessary to use minicom

	//cout << "Hello world!\n";

	// GPIO led = EPOS::GPIO{'C', 3, EPOS::GPIO::OUTPUT};
	// led.set();
	UART uart(1, 38400, 8, 0, 1);
	uart.loopback(false);

	while (1) {
		eMote3_GPTM::delay(500000);
		uart.put('a');
		cout << "put\n";
		// cout << uart.get();
	}
	// pingUART();
	// Thread thread_blink = Thread(&blink);
	// int status_blink = thread_blink.join();

	// configReceiver();
	// receiver();

	//uart.config(115200, 8, 0, 1);

	/*Vector<uint8_t, 10> vect;
	uint8_t vectObj[10];
	Vector<uint8_t, 10>::Element * vectElem[10];

	for (int i = 0 ; i < 10 ; i++) {
		vectObj[i] = (uint8_t)i;
		vectElem[i] = new Vector<uint8_t, 10>::Element(&vectObj[i]);
		vect.insert(vectElem[i], i);
	}*/

	//Periodic_Thread thread_ping(RTConf(1000000, Periodic_Thread::INFINITE), &pingUART);
	//Thread thread_print = Thread(&printVector, &vect);

	// Thread thread_receiver = Thread(&receiver);
	// int status_receiver = thread_receiver.join();

	//printVectorUART(vect);

	//int status_ping = thread_ping.join();
	//int status_print = thread_print.join();

    return 0;
}
