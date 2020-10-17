// EPOS Cortex ESP8266 Wi-Fi Board Mediator Declarations

#include <system/config.h>
#ifndef __mmod_zynq__
#ifndef __esp8266_h
#define __esp8266_h

#include <machine.h>
#include <alarm.h>
#include <machine/cortex/uart.h>
#include <machine/cortex/gpio.h>
#include <utility/string.h>

__BEGIN_SYS

class ESP8266
{
public:
    static const unsigned int SSID_MAX = 64;
    static const unsigned int PASS_MAX = 64;
    static const unsigned int USERNAME_MAX = 64;
    static const unsigned int HOST_MAX = 128;
    static const unsigned int ROUTE_MAX = 128;
    static const unsigned int PORT_MAX = 6;
    static const unsigned int IP_MAX = 16;
    static const unsigned int UART_BUFFER_SIZE = 1024;
    static const unsigned int DEFAULT_TIMEOUT = 10000000;

    typedef RTC::Microsecond Microsecond;

    enum Timeout_Action {
        RETURN,
        RESET_ESP,
        REBOOT_MACHINE
    };

    enum Auth_Method {
        PERSONAL,
        ENTERPRISE
    };

    // static const Timeout_Action TIMEOUT_ACTION = REBOOT_MACHINE;

    enum WiFi_Status {
        WL_IDLE_STATUS = 0,     //when Wi-Fi is in process of changing between statuses
        WL_NO_SSID_AVAIL = 1,   //in case configured SSID cannot be reached
        WL_CONNECTED = 3,       //after successful connection is established
        WL_CONNECT_FAILED = 4,  //if password is incorrect
        WL_DISCONNECTED = 6     //if module is not configured in station mode
    };

public:
    ESP8266(UART *uart, GPIO *power) : _rstkey(power), _uart(uart), _auth_method(PERSONAL) { 
        reset();
        Alarm::delay(5000000);
    }

    ~ESP8266() {}

    void on() { _rstkey->set(1); }
    void off() { _rstkey->set(0); }

    RTC::Microsecond now(Microsecond timeout = DEFAULT_TIMEOUT);

    void reset();

    // int connect(const char *ssid, int ssid_size, const char *pass, int pass_size, const char *username = "", int username_size = 0);

    bool wifi_connect(const char *ssid, int ssid_size, const char *pass, int pass_size, const char *username = 0, int username_size = 0);

    bool wifi_connect();

    bool wifi_reconnect();

    void gmt_offset(long gmt_offset_sec);

    void sleep();

    bool command_mode();
    
    int post(const char * data, unsigned int data_size, char * res = 0, unsigned int res_size = 0);

    int api_health_check();
    
    int api_attach(const void * request, unsigned int request_size);
    
    int api_put(const void * request, unsigned int request_size);

    bool wifi_status(WiFi_Status * code = 0);

    void set_ssid(const char * ssid, unsigned int ssid_size);
    
    void set_password(const char * pass, unsigned int pass_size);
    
    void set_username(const char * username, unsigned int username_size);

    const char * ssid() { return _ssid; }
    const char * pass() { return _password; }
    int ip(char * ip);

private:
    void flush_serial();

    void send_command(const char *command, unsigned int size);

    int wait_response(char * response, unsigned int response_size, Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_response(const char * expected, Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_response_1(Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_timeout(Timeout_Action timeout_action = RETURN);

    GPIO * _rstkey;
    UART * _uart;
    TSC::Time_Stamp _last_send;
    TSC::Time_Stamp _init_timeout;

    int _auth_method;
    char * _ssid;
    char * _username;
    char * _password;
};

__END_SYS

#endif
#endif
