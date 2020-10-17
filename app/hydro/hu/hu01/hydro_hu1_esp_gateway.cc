// TSTP Gateway to be used with tools/eposiotgw/eposiotgw

#include <transducer.h>
#include <smart_data.h>
#include <tstp.h>
#include <utility/ostream.h>
#include <gpio.h>

//This includes the server credentials and wifi credentials constants
// Server Credentials
// const char SERVER_CREDENTIALS_DOMAIN[]   = "...";
// const char SERVER_CREDENTIALS_USERNAME[] = "...";
// const char SERVER_CREDENTIALS_PASSWORD[] = "...";
//WiFi Credentials
// const char WIFI_CREDENTIALS_SSID[] = "...";
// const char WIFI_CREDENTIALS_USERNAME[] = "...";
// const char WIFI_CREDENTIALS_PASSWORD[] = "...";
//it is on svn:ignore, so you need to create the file
#include <credentials.key>

using namespace EPOS;

// IF<Traits<USB>::enabled, USB, UART>::Result * io;

typedef TSTP::Coordinates Coordinates;
typedef TSTP::Region Region;
typedef Smart_Data_Common::SI_Record DB_Record; //this app does not use Digital Records
typedef Smart_Data_Common::DB_Series DB_Series;

struct Credentials
{
    Credentials() {
        _size_domain = sizeof(SERVER_CREDENTIALS_DOMAIN) - 1;
        memcpy(_domain,SERVER_CREDENTIALS_DOMAIN,_size_domain);
        _size_username = sizeof(SERVER_CREDENTIALS_USERNAME) - 1;
        memcpy(_username,SERVER_CREDENTIALS_USERNAME,_size_username);
        _size_password = sizeof(SERVER_CREDENTIALS_PASSWORD) - 1;
        memcpy(_password,SERVER_CREDENTIALS_PASSWORD,_size_password);
    }
    char _size_domain;
    char _domain[sizeof(SERVER_CREDENTIALS_DOMAIN) - 1];
    char _size_username;
    char _username[sizeof(SERVER_CREDENTIALS_USERNAME) - 1];
    char _size_password;
    char _password[sizeof(SERVER_CREDENTIALS_PASSWORD) - 1];
}__attribute__((packed));

struct Attach_Payload
{
    Attach_Payload(const DB_Series s) { _series = s; }
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Series series){ _series = series; }
public:    
    Credentials _credentials;
    DB_Series _series;
}__attribute__((packed));

struct Put_Payload
{
    Put_Payload(const DB_Record r){ _smartdata = r; }
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Record smartdata){ _smartdata = smartdata; }

public:    
    Credentials _credentials;
    DB_Record _smartdata;
}__attribute__((packed));


const unsigned int INTEREST_PERIOD = 10 * 1000000;
const unsigned int INTEREST_EXPIRY = 2 * INTEREST_PERIOD;

UART * io;

template<typename T>
void print(const T & d)
{
    OStream cout;
    bool was_locked = CPU::int_disabled();
    if(!was_locked)
        CPU::int_disable();
    if(EQUAL<T, Attach_Payload>::Result) {
        io->put('S');
        // cout << 'S';
    }
    else {
        io->put('R');
        // cout << 'R';
    }
    for(unsigned int i = 0; i < sizeof(T); i++) {
        // cout << hex << reinterpret_cast<const char *>(&d)[i];
        io->put(reinterpret_cast<const char *>(&d)[i]);
    }
    for(unsigned int i = 0; i < 3; i++) {
        io->put('X');
        // cout << 'X';
    }
    if(!was_locked)
        CPU::int_enable();
    // cout << endl;
}

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t) : _data(t) {
        _data->attach(this);
        DB_Series s = _data->db_series();
        // cout << s << endl;
        print(Attach_Payload(s));
    }
    ~Printer() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        DB_Record r = _data->db_record();
        // cout << r << endl;
        print(Put_Payload(r));
    }

private:
    T * _data;
    OStream cout;
};

void setup()
{
    OStream cout;
    // Get epoch time from serial
    TSTP::Time epoch = 0;
    char c = 0;
    unsigned int count = 0;
    char timestamp[30];
    unsigned int X = 0;
    bool start = false;
    
    while(true) {
        // if(!start)
            // io->put('T');
        while(!io->ready_to_get())
            io->put('T');
        c = io->get();
        // cout << c;
        if(!start) {
            if(c == 'X')
                X++;
            else
                X=0;
            if(X >= 3) {
                start = true;
            }
        }
        else {
            if(c != 'X') {
                timestamp[count] = c;
                count++;  
            }
            if(count >= 10)
                break;
        }
    }
    // cout << endl;
    timestamp[count+1]=0;
    TSTP::epoch(atoi(timestamp)*1000000ULL);
    // Alarm::delay(5000000);
}

int main()
{
    OStream cout;
    Alarm::delay(2000000);
    cout << "main()" << endl;

    io = new UART(0, 115200, 8, 0, 1);
    GPIO rst('C', 0, GPIO::OUT);
    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    setup();

    //TSTP config

    //HU01 as gateway
    // Coordinates center_station1(100,100,100);
    // Coordinates center_station2(-4900,-200,-6100);

    // new Printer<Water_Flow_M170>(new Water_Flow_M170(0, INTEREST_EXPIRY, Water_Flow_M170::PRIVATE, INTEREST_PERIOD));
    // new Printer<Water_Flow_WSTAR>(new Water_Flow_WSTAR(region_station1, INTEREST_EXPIRY, INTEREST_PERIOD));
    // new Printer<Water_Flow_M170>(new Water_Flow_M170(region_station2, INTEREST_EXPIRY, INTEREST_PERIOD));

    // Region region_station1(center_station1, 0, 0, -1);
    // Region region_station2(center_station2, 0, 0, -1);

    //HU03 as gateway
    Coordinates center_station1(4900,200,6100);
    Coordinates center_station2(5000,300,6200);

    Region region_station1(center_station1, 0, 0, -1);
    Region region_station2(center_station2, 0, 0, -1);

    new Printer<Water_Flow_M170>(new Water_Flow_M170(0, INTEREST_EXPIRY, Water_Flow_M170::PRIVATE, INTEREST_PERIOD));
    new Printer<Water_Flow_M170>(new Water_Flow_M170(region_station1, INTEREST_EXPIRY, INTEREST_PERIOD));
    new Printer<Water_Flow_WSTAR>(new Water_Flow_WSTAR(region_station2, INTEREST_EXPIRY, INTEREST_PERIOD));

    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }
    Thread::self()->suspend();
    
    return 0;
}