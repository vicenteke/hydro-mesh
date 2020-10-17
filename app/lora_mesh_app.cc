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

int receiver() {
// Keeps looking for entries in uart and echos any received message

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

	Thread t_receiver 	= Thread(&receiver);
	Thread t_ping 		= Thread(&ping);

	int s_ping		 = t_ping.join();
	int s_receiver	 = t_receiver.join();

	while (1) {

	}

    return 0;
}
