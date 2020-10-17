#include <alarm.h>
#include <transducer.h>

using namespace EPOS;

int kick()
{
    OStream cout;
    cout << "Watchdog init" << endl;

    Watchdog::enable();
    Watchdog::kick();

    while(Periodic_Thread::wait_next())
        Watchdog::kick();    

    return 0;
}

int main()
{
    Water_Flow_M170 flow(0, 1000000, Water_Flow_M170::ADVERTISED);

    Periodic_Thread * watchdog = new Periodic_Thread(500 * 1000, kick);

    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }

    watchdog->join();

    Thread::self()->suspend();

    return 0;
}
