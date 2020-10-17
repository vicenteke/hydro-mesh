// #include <smart_data.h>
#include <alarm.h>
#include <tsc.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <imu.h>
#include <thread.h>

using namespace EPOS;

// int led_thread()
// {
//     GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
//     char led = 0;
//     while(1) {
//         Alarm::delay(1000000);
//         g.set((led++)%2);
//     }
// }

int main()
{
    OStream cout;
    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    // const char id[] = "CAR";
    const char id[] = "BED";
    // const char id[] = "SUP";
    
    Alarm::delay(5000000);
    
    LSM330 imu;
    imu.beginAcc();
    g.set(true);

    // new Thread(&led_thread);

    int count = 0;
    bool led = true;
    while(1) {
        if(imu.measureAcc())
            cout << id << "," << imu.acc_x << "," << imu.acc_y << "," << imu.acc_z << endl;

        if(count >= 500) {
            led = !led;
            g.set(led);
            count = 0;
        }
        count++;
    }

    return 0;
}