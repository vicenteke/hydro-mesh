#include <tstp.h>
#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <transducer.h>
#include <persistent_storage.h>
#include <utility/ostream.h>
#include <watchdog.h>
#include <machine/cortex/esp8266.h>
#include <mutex.h>

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

// Timeout variable
TSC::Time_Stamp _init_timeout;

// Timeout constants
const RTC::Microsecond CONNECT_TO_WIFI_TIMEOUT = 2ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_SERIES_TIMEOUT = 5ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_RECORD_TIMEOUT = 5ull * 60 * 1000000;

// Test time
// const RTC::Microsecond INTEREST_PERIOD = 10 * 1000000;
// const RTC::Microsecond INTEREST_EXPIRY = 2 * INTEREST_PERIOD;
// const RTC::Microsecond HTTP_SEND_PERIOD = 1ull * 60 * 1000000;
// const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

// Production time
const RTC::Microsecond INTEREST_PERIOD = 1ull * 60 * 1000000;
const RTC::Microsecond INTEREST_EXPIRY = 2 * INTEREST_PERIOD;
const RTC::Microsecond HTTP_SEND_PERIOD = 5ull * 60 * 1000000;
const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

const unsigned int DB_RECORDS_NUM = 50;

typedef Smart_Data_Common::SI_Record DB_Record; //this app does not use Digital Records
typedef Smart_Data_Common::DB_Series DB_Series;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

typedef TSTP::Coordinates Coordinates;
typedef TSTP::Region Region;

// Server
const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_ATTACH[] = "/api/attach.php";
const char ROUTE_PUT[] = "/api/put.php";
const char ROUTE_HEALTH_CHECK[] = "/api/health_check.php";

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
    // void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Series series){ _series = series; }
public:    
    // Credentials _credentials;
    DB_Series _series;
}__attribute__((packed));

// TODO: find a generic way to declare the common smartdata variables
template <unsigned int S>
struct _Put_Payload
{
    static const unsigned int SIZE = S;
    // void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Record smartdata, unsigned int index){ _smartdata[index] = smartdata; }

public:    
    // Credentials _credentials;
    DB_Record _smartdata[SIZE];
}__attribute__((packed));
typedef _Put_Payload<DB_RECORDS_NUM> Put_Payload;

ESP8266 * esp;
Mutex mutex;

bool check_server()
{
    OStream cout;
    TSC::Time_Stamp timeout = TSC::time_stamp() + CONNECT_TO_WIFI_TIMEOUT * (TSC::frequency() / 1000000);
    unsigned int error = 0;
    do {
        if(esp->wifi_reconnect()) {
            cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
            cout << "...Checking Server..." << endl;
            int http_code = esp->api_health_check();
            if((http_code >= 200) && (http_code < 300)) {
                cout << "Server is ok" << endl;
                return true;
            }
            else if(http_code < 0) {
                cout << "ESP error while checking server" << endl;
                error++;
            }
            else {
                cout << "Server is down" << endl;
                return false;
            }
        }
        else
            error++;
        if(error == 3) {
            esp->reset();
            error = 0;
        }
        Alarm::delay(2000000);
    } while(TSC::time_stamp() < timeout);
    cout << "Timeout!" << endl;
    return false;
}

bool send_records(const void * data, unsigned int size)
{
    OStream cout;
    TSC::Time_Stamp timeout = TSC::time_stamp() + CONNECT_TO_WIFI_TIMEOUT * (TSC::frequency() / 1000000);
    unsigned int error = 0;
    do {
        if(esp->wifi_reconnect()) {
            cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
            cout << "...Sendind db_records..." << endl;
            int http_code = esp->api_put(data, size);
            cout << "post=" << http_code << endl;
            if((http_code >= 200) && (http_code < 300))
                return true;
            else if(http_code < 0) {
                if(http_code == -7) { 
                    //The server is taking to long to respond to PUTs (issue). The ESP then times out sometimes.
                    //However, the data is being accepted and stored.
                    cout << "ESP timed out while waiting for server response, but the data is there" << endl;
                    return true;
                }
                cout << "ESP internal error" << endl;
                error++;
            }
            else if(http_code == 400) { //Client error: Bad format -> discard payload
                cout << "Server refused payload for Bad format, discarding it" << endl;
                return true;
            }
            else {
                cout << "Server refused connection" << endl;
                return false;
            }
        }
        else
            error++;
        if(error == 3) {
            esp->reset();
            error = 0;
        }
        Alarm::delay(2000000);
    } while(TSC::time_stamp() < timeout);
    cout << "Timeout!" << endl;
    return false;
}

template<typename ...Ts>
int http_send(Ts * ... args)
{
    OStream cout;
    cout << "http_send() init" << endl;

    int dummy1[] = {0,(send_series(args),0)... };

    // cout << "Turn off ESP" << endl;
    // esp->off();

    DB_Record e;
    bool ret = false;
    while(true) {
        for(unsigned int i = 0; i < HTTP_SEND_PERIOD_MULTIPLY; i++)
            Periodic_Thread::wait_next();

        cout << "http_send()" << endl;

        //cout << "Turning ESP on" << endl;
        //esp->on();
        //Alarm::delay(2000000);

        if(check_server()) {

            mutex.lock();
            // CPU::int_disable();
            if(Storage::pop(&e)) {
                // CPU::int_enable();
                mutex.unlock();

                bool popped_next = true;
                unsigned int db_record_index = 0;

                Put_Payload * put_payload = new Put_Payload();

                while(popped_next) {

                    Alarm::delay(1000);
                    put_payload->payload(e, db_record_index);
                    ret = false;

                    cout << "(POP)" << e << endl;

                    mutex.lock();
                    // CPU::int_disable();
                    popped_next = Storage::pop(&e);
                    if(!popped_next)
                            Storage::clear();
                    // CPU::int_enable();
                    mutex.unlock();

                    // Declare a timeout for sending the db_records
                    // _init_timeout = TSC::time_stamp() + SEND_DB_RECORD_TIMEOUT * (TSC::frequency() / 1000000);
                
                    if((db_record_index == (Put_Payload::SIZE-1)) || !popped_next) {
                        ret = send_records(put_payload, sizeof(Put_Payload)-((Put_Payload::SIZE-(db_record_index+1))*sizeof(DB_Record)));
                        if(!ret) {
                            cout << "Pushing all db_records back to the stack, and waiting for other thread cycle" << endl;
                            for(int i = 0; i <= db_record_index; i++) {
                                mutex.lock();
                                // CPU::int_disable();
                                Storage::push(put_payload->_smartdata[i]);
                                // CPU::int_enable();
                                mutex.unlock();
                                Alarm::delay(1000);
                            }
                            esp->reset();
                            Alarm::delay(2000000);
                            break;
                        }
                        db_record_index = 0;
                    } 
                    else
                        db_record_index++;
                }
                delete put_payload;
                if(ret) {
                    RTC::Microsecond t = esp->now();
                    if(t != 0)
                        TSTP::epoch(t);
                }
            } else {
                // CPU::int_enable();
                mutex.unlock();
            }
            cout << "Going to sleep..." << endl;
        }
        //cout << "Turning ESP off" << endl;
        //esp->off();
    }
}

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

template<typename ...Ts>
int tstp_work(Ts * ... args) {
    OStream cout;
    cout << "tstp_work() init" << endl;

    cout << "epoch now = " << TSTP::absolute(TSTP::now()) / 1000000 << endl;

    cout << "Going to sleep..." << endl;
    while(Periodic_Thread::wait_next()) {
        int dummy2[] = {0,(stack_record(args),0)... };
        cout << "Going to sleep..." << endl;
    }
    return 0;
}

template<typename T>
void send_series(T * t) {
    OStream cout;
    bool ret = false;
    Attach_Payload attach_payload;
    attach_payload.payload(t->db_series());
    unsigned int error = 0;
    do {
        if(esp->wifi_reconnect()) {
            cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
            cout << "...Sending db_series..." << endl;
            int http_code = esp->api_attach(&attach_payload, sizeof(Attach_Payload));
            cout << "post=" << http_code << endl;
            if((http_code >= 200) && (http_code < 300))
                ret = true;
            else if(http_code < 0) {
                cout << "ESP internal error" << endl;
                error++;
            }
            else {
                cout << "Server refused connection" << endl;
            }
        }
        else
            error++;
        if(error == 3) {
            esp->reset();
            error = 0;
        }
        Alarm::delay(2000000);
    } while(!ret);
}

template<typename T>
void stack_record(T * t) {
    OStream cout;
    DB_Record r = t->db_record();
    cout << "(PUSH)" << r << endl;
    mutex.lock();
    // CPU::int_disable();
    Storage::push(r);
    // CPU::int_enable();
    mutex.unlock();
}

int main() {
    OStream cout;
    Alarm::delay(1000000);          
    cout << "main()" << endl;

    // Storage::clear(); cout << "storage cleared" << endl; //Uncomment this to clear the storage

    UART uart(0, 115200, 8, 0, 1);
    GPIO rst('C', 0, GPIO::OUT);
    // GPIO rst('B', 7, GPIO::OUT);
    esp = new ESP8266(&uart, &rst);
    esp->set_ssid(WIFI_CREDENTIALS_SSID, sizeof(WIFI_CREDENTIALS_SSID));
    esp->set_password(WIFI_CREDENTIALS_PASSWORD, sizeof(WIFI_CREDENTIALS_PASSWORD));
    //Uncomment the next line to connect to 802.11x networks (be sure of your credentials in credentials.key)
    esp->set_username(WIFI_CREDENTIALS_USERNAME, sizeof(WIFI_CREDENTIALS_USERNAME));

    TSTP::Time t = 0;
    _init_timeout = TSC::time_stamp() + CONNECT_TO_WIFI_TIMEOUT * (TSC::frequency() / 1000000);
    cout << "Connecting to " << WIFI_CREDENTIALS_SSID << endl;

    unsigned int error = 0;
    while(true) {
        bool wifi = esp->wifi_connect();
        if(wifi)
            t = esp->now();
        if(!wifi | t==0) {
            error++;
            if(error == 3) {
                esp->reset();
                error = 0;
            }
            Alarm::delay(2000000);
        }
        else
            break;
    }

    cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
    cout << "epoch=" << t << endl;
    TSTP::epoch(t);

    //SmartData
    Water_Flow_WSTAR flow1(0, 0);

    //Threads config
    new Periodic_Thread(INTEREST_PERIOD, &tstp_work, &flow1);
    new Periodic_Thread(HTTP_SEND_PERIOD, &http_send, &flow1);
    new Periodic_Thread(500 * 1000, kick);

    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }

    // http_sender.join();
    // watchdog.join();

    Thread::self()->suspend();
    return 0;
}