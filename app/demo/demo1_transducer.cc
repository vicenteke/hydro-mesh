#include <smart_data.h>
#include <alarm.h>
#include <i2c.h>

using namespace EPOS;

OStream cout;

int main()
{
    Alarm::delay(5000000);
    cout << "Hello! I'm a standalone sensor node" << endl;

    // Local SmartData constructor
    I2C_Temperature t(0, 1000000, I2C_Temperature::PRIVATE); // dev=0, expiry=1s, mode=PRIVATE

    while(true) {
        Alarm::delay(2000000); // 2s
        cout << "Temperature = " << t << " at " << t.location() << ", " << t.time() << endl;
    }

    return 0;
}