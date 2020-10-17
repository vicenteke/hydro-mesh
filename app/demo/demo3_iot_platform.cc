#include <smart_data.h>
#include <alarm.h>
#include <i2c.h>

using namespace EPOS;

USB io;

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t) : _data(t) {
        _data->attach(this);
        print(_data->db_series());
    }
    ~Printer() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        print(_data->db_record());
    }

    template<typename D>
    void print(const D & d)
    {
        bool was_locked = CPU::int_disabled();
        if(!was_locked)
            CPU::int_disable();
        if(EQUAL<D, Smart_Data_Common::DB_Series>::Result)
            io.put('S');
        else
            io.put('R');
        for(unsigned int i = 0; i < sizeof(D); i++)
            io.put(reinterpret_cast<const char *>(&d)[i]);
        for(unsigned int i = 0; i < 3; i++)
            io.put('X');
        if(!was_locked)
            CPU::int_enable();
    }

private:
    T * _data;
};

void setup()
{
    // Get epoch time from serial
    TSTP::Time epoch = 0;
    unsigned int bytes = 0;
    char c = 0;
    do {
        while(!io.ready_to_get());
        c = io.get();
    } while(c != 'X');
    while(bytes < sizeof(TSTP::Time)){
        while(!io.ready_to_get());
        c = io.get();
        epoch |= (static_cast<unsigned long long>(c) << ((bytes++)*8));
    }
    TSTP::epoch(epoch);
    Alarm::delay(5000000);
}

int main()
{
    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    g.set(false); // turn off led
    setup();
    g.set(true); // turn on led

    // Local SmartData constructor
    I2C_Temperature t(0, 1000000, I2C_Temperature::PRIVATE, 5000000); // dev=0, expiry=1s, mode=PRIVATE, period=5s
    Printer<I2C_Temperature> p(&t);

    Thread::self()->suspend();
    return 0;
}