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

#include <cstdint>

using namespace EPOS;

OStream cout;
UART uart(0, 115200, 8, 0, 1);

void uartHandler(const unsigned int &) {
	cout << "UART Handler called: " << uart.get() << "\n";
}

void configUARTInterrupt() {
	// IRQ_UART1
	IC ic = IC();
	ic.int_vector(NVIC::IRQ_UART0, &uartHandler);
	ic.enable(NVIC::IRQ_UART0);
	//uart.int_enable();
}

int main()
{
	Alarm::delay(5000000); //Necessary to use minicom

	//cout << "Hello world!\n";
	uart.loopback(false);
	configUARTInterrupt();

	GPIO led = EPOS::GPIO{'C', 3, EPOS::GPIO::OUT};
	led.set();

	while (1) {
		Alarm::delay(1000000);
		uart.put('p');
		uart.put('i');
		uart.put('n');
		uart.put('g');
	}

    return 0;
}
