// UART 0: PA0 (RX) / PA1 (TX)
// UART 1: PC3 (RX) / PC4 (TX)

// 7024: master ; 7109: ID 1 ; 7222: ID 2

// Message sent by station:
    // General:
        // msg[15] = name[5] (w/ \0) + timestamp[4] + level[2] + tur[2] + plu + signal
    // Other: no name, msg[10]


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

using namespace EPOS;

OStream cout;
UART uart(0, 115200, 8, 0, 1);

void uartHandler(const unsigned int &) {
	cout << "UART Handler called: " << uart.get() << "\n";
}

void configUARTInterrupt() {
	// IRQ_UART1
	// IC ic = IC();
	// ic.int_vector(NVIC::IRQ_UART0, &uartHandler);
	// ic.enable(NVIC::IRQ_UART0);
	uart.int_disable(true, false, true, true);
	IC::int_vector(NVIC::IRQ_UART0, &uartHandler);
	uart.int_enable(true, false, true, true);
}

int receiver() {
// Keeps looking for entries in _uart and echos any received message

	cout << "receiver() created\n";

	while (1) {
		while (!uart.ready_to_get());
		while(uart.ready_to_get()) {
			cout << uart.get();
			Alarm::delay((int)(500)); // 500 for 115200
		}
		cout << '\n';
	}

	return 0;
}

int ping() {
// Keeps pinging in uart

	cout << "ping() created\n";

	while (1) {
		Alarm::delay(1500000);
		uart.put('p');
		uart.put('i');
		uart.put('n');
		uart.put('g');
	}

	return 0;
}

int main()
{
	Alarm::delay(2000000); //Necessary to use minicom

	uart.loopback(false);

	GPIO led = EPOS::GPIO{'C', 3, EPOS::GPIO::OUT};
	led.set();

	Thread t_receiver = Thread(&receiver);
	int s_receiver = t_receiver.join();

    // UART uart(1, 38400, 8, 0, 1);
	// uart.loopback(false);
    //
	// while (1) {
	// 	eMote3_GPTM::delay(500000);
	// 	uart.put('a');
	// 	cout << "put\n";
	// 	// cout << uart.get();
	// }
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

	while (1) {

	}

    return 0;
}
