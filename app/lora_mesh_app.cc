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
#include "LoraMesh/include/loraMesh.hpp"
// #include "LoraMesh/include/loraMesh.h"

using namespace EPOS;

OStream cout;

int main()
{
	Alarm::delay(2000000); //Necessary to use minicom

    cout << "------------ LoRa Mesh Program ------------\n";

	LoraMesh lora = LoraMesh();
    lora.localRead(MASTER_ID);

    cout << "UID: " << lora.uid()
        << "\nID: " << lora.id()
        << "\nSF: " << lora.sf() << '\n';

	while (1) {

	}

    return 0;
}
