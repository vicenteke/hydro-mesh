#include <smart_data.h>

using namespace EPOS;

int main()
{
    OStream cout;
    // cout << "EPOSMote III SmartDoor" << endl;
    Machine::delay(5000000);

    // Accelerometer accelerometer(0, 0, Accelerometer::PRIVATE);
    new Acceleration_LSM330(0, 30000000, Acceleration_LSM330::ADVERTISED);

    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }
    return 0;
}