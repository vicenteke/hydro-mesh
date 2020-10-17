// TSTP Gateway to be used with tools/eposiotgw/eposiotgw

#include <transducer.h>
#include <smart_data.h>
#include <tstp.h>
#include <utility/ostream.h>
#include <utility/predictor.h>
#include <gpio.h>
#include <semaphore.h>
#include <i2c.h>


using namespace EPOS;

IF<Traits<USB>::enabled, USB, UART>::Result io;

typedef TSTP::Coordinates Coordinates;
typedef TSTP::Region Region;

const unsigned int INTEREST_PERIOD = 30 * 1000000;
const unsigned int INTEREST_EXPIRY = 2 * INTEREST_PERIOD;

template<typename T>
void print(const T & d)
{
    bool was_locked = CPU::int_disabled();
    if(!was_locked)
        CPU::int_disable();
    if(EQUAL<T, Smart_Data_Common::DB_Series>::Result)
        io.put('S');
    else
        io.put('R');
    for(unsigned int i = 0; i < sizeof(T); i++)
        io.put(reinterpret_cast<const char *>(&d)[i]);
    for(unsigned int i = 0; i < 3; i++)
        io.put('X');
    if(!was_locked)
        CPU::int_enable();
}

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

private:
    T * _data;
};

typedef Smart_Data<Dummy_Transducer<TSTP::Unit::Get_Quantity<TSTP::Unit::Acceleration,TSTP::Unit::F32>::UNIT>> Dummy_Accelerometer;
template<typename T>
class Acceleration_Transform: public Smart_Data_Common::Observer
{
    static const unsigned int INFINITE = -1;
public:
    Acceleration_Transform(T * t) : _data(t) {
        _data->attach(this);
        _acc = new Dummy_Accelerometer(0,INFINITE,Dummy_Accelerometer::PRIVATE);
        Dummy_Accelerometer::DB_Series db_s = _acc->db_series();
        // print(_acc->db_series());
        cout << db_s << endl;
    }
    ~Acceleration_Transform() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        Acceleration_LSM330::Value v = (*_data);
        Acceleration_LSM330::Time t = _data->time();

        Accelerometer_LSM330::Data d = *(v.data<Accelerometer_LSM330::Data>());

        cout << d << endl;

        // Dummy_Accelerometer::DB_Record db;

        // for(int i = 0; i < Accelerometer_LSM330::Data::SIZE; i++){
        //     t+=(d.m[i].offset * 1000);

        //     (*_acc) = d.m[i].x;
        //     db = _acc->db_record();
        //     db.t = t;
        //     db.dev = 0;
        //     print(db);

        //     (*_acc) = d.m[i].y;
        //     db = _acc->db_record();
        //     db.t = t;
        //     db.dev = 1;
        //     print(db);

        //     (*_acc) = d.m[i].z;
        //     db = _acc->db_record();
        //     db.t = t;
        //     db.dev = 2;
        //     print(db);
        // }
    }

private:
    Acceleration_LSM330 * _data;
    Dummy_Accelerometer * _acc;
    OStream cout;
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
    // g.set(false); // turn off led
    // setup();
    // g.set(true); // turn on led

    // Interest center points
    Coordinates center_car(10, 10, 0);
    // Coordinates center_plate(-5-730, -30-80, -15);
    // Coordinates center_lights1(305-730, -170-80, 220);

    // Regions of interest
    Region region_car(center_car, 0, 0, -1);
    // Region region_plate(center_plate, 0, 0, -1);
    // Region region_lights1(center_lights1, 0, 0, -1);

    new Acceleration_Transform<Acceleration_LSM330>(new Acceleration_LSM330(region_car, 30000000, 500000));

    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }

    return 0;
}
