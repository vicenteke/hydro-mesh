#include <smart_data.h>
#include <alarm.h>
#include <i2c.h>

using namespace EPOS;

OStream cout;

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t) : _data(t) {
        cout << "Printer::Printer() attaching the Printer to SmartData" << endl;
        _data->attach(this);
    }
    ~Printer() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        cout << "Printer::update() printer has been notified by SmartData" << endl;
        cout << "Temperature = " << (*_data) << " at " << _data->location() << ", " << _data->time() << endl;
    }

private:
    T * _data;
};

int main()
{
    Alarm::delay(5000000);
    cout << "Hello! I'm a standalone sensor node" << endl;

    // Local SmartData constructor
    I2C_Temperature t(0, 3000000, I2C_Temperature::PRIVATE, 5000000); // dev=0, expiry=15s, mode=PRIVATE, period=1s
    Printer<I2C_Temperature> p(&t);

    Thread::self()->suspend();
    return 0;
}