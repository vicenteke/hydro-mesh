// EPOS Cortex ESP8266 Wi-Fi Board Mediator Implementation

#include <machine/cortex/esp8266.h>
#ifndef __mmod_zynq__

__BEGIN_SYS

// Class attributes

/* List of commands and responses */
//End of msg
static const char end_of_msg[] = "\r\n";

// Methods

void ESP8266::set_ssid(const char * ssid, unsigned int ssid_size)
{
    assert(ssid_size < SSID_MAX);
    _ssid = new char[ssid_size];
    memcpy(_ssid, ssid, ssid_size);
}

void ESP8266::set_password(const char * pass, unsigned int pass_size)
{
    assert(pass_size < PASS_MAX);
    _password = new char[pass_size];
    memcpy(_password, pass, pass_size);
}

void ESP8266::set_username(const char * username, unsigned int username_size)
{
    assert(username_size < USERNAME_MAX);
    _username = new char[username_size];
    memcpy(_username, username, username_size);
    _auth_method = ENTERPRISE;
}

bool ESP8266::check_timeout(ESP8266::Timeout_Action timeout_action)
{
    if(TSC::time_stamp() > _init_timeout) {
        switch(timeout_action) {
            case ESP8266::Timeout_Action::REBOOT_MACHINE:
                CPU::int_disable();
                Machine::delay(5000000);
                Machine::reboot();
                break;
            case ESP8266::Timeout_Action::RESET_ESP:
                reset();
                // fallthrough
            case ESP8266::Timeout_Action::RETURN:
            default:
                return true;
        }
        return true;
    }
    else
        return false;
}

RTC::Microsecond ESP8266::now(Microsecond timeout)
{
    const char cmd_get_timestamp[] = "AT+GETTIMESTAMP";
    const char res_timestamp[] = "+TIMESTAMP=";
    if(command_mode()) {
        send_command(cmd_get_timestamp, sizeof(cmd_get_timestamp)-1);
        db<ESP8266>(WRN) << "[ESP_CMD]" << cmd_get_timestamp << endl;
        char response[30];
        int ret = wait_response(response, sizeof(response));
        if(ret > 0) {
            response[ret]=0;
            db<ESP8266>(WRN) << "[ESP_RES]" << response << endl;
            int timestamp_res_pos = strstr(response, res_timestamp);
            if(timestamp_res_pos >= 0) {
                int timestamp_pos = timestamp_res_pos + (sizeof(res_timestamp)-1);
                int timestamp_size = ret - timestamp_pos - (sizeof(end_of_msg)-1) + 1;
                char timestamp[timestamp_size];
                strncpy(timestamp, &response[timestamp_pos], timestamp_size-1);
                timestamp[timestamp_size-1]=0;
                return atoi(timestamp) * 1000000ULL;
            }

        }
    }
    return 0;
}

void ESP8266::sleep()
{
    const char cmd_sleep[] = "AT+DEEPSLEEP";
    if(command_mode()) {
        send_command(cmd_sleep, sizeof(cmd_sleep)-1);
    }
}

void ESP8266::reset()
{
    off();
    Machine::delay(500000); // 500ms delay recommended by the manual
    on();
}

bool ESP8266::wifi_connect(const char *ssid, int ssid_size, const char *pass, int pass_size, const char *username, int username_size)
{
    set_ssid(ssid, ssid_size);
    set_password(pass, pass_size);
    if(username)
        set_username(username, username_size);

    ssid_size -= 1;
    pass_size -= 1;
    username_size -= 1;

    const char cmd_connect_wifi[] = "AT+CONNECTWIFI=";

    unsigned int cmd_wifi_size = sizeof(cmd_connect_wifi)-1;
    unsigned int cmd_size = cmd_wifi_size + ssid_size + 1 + pass_size;
    if(_auth_method == ENTERPRISE)
        cmd_size += 1 + username_size;
    char command[cmd_size];
    strncpy(command, cmd_connect_wifi, cmd_wifi_size);
    strncpy(&command[cmd_wifi_size], ssid, ssid_size);
    command[cmd_wifi_size + ssid_size] = ',';
    strncpy(&command[cmd_wifi_size + ssid_size + 1], pass, pass_size);
    if(_auth_method == ENTERPRISE) {
        command[cmd_wifi_size + ssid_size + 1 + pass_size] = ',';
        strncpy(&command[cmd_wifi_size + ssid_size + 1 + pass_size + 1], username, username_size);
    }
    // The ESP can restart sometimes when trying to connect to an WiFi network (particularlly in ENTERPRISE auth mode).
    // The ESP sends a flag message when it restarts 
    if(command_mode()) {
        send_command(command, cmd_size);
        if(!check_response_1(40000000)) {
            return false;
        }
    }
    else
        return false;

    return true;
}

bool ESP8266::wifi_connect()
{
    const char cmd_connect_wifi[] = "AT+CONNECTWIFI=";

    int ssid_size = strlen(_ssid);
    int pass_size = strlen(_password);
    int username_size = strlen(_username);

    unsigned int cmd_wifi_size = sizeof(cmd_connect_wifi)-1;
    unsigned int cmd_size = cmd_wifi_size + ssid_size + 1 + pass_size;
    if(_auth_method == ENTERPRISE)
        cmd_size += 1 + username_size;
    char command[cmd_size];
    strncpy(command, cmd_connect_wifi, cmd_wifi_size);
    strncpy(&command[cmd_wifi_size], _ssid, ssid_size);
    command[cmd_wifi_size + ssid_size] = ',';
    strncpy(&command[cmd_wifi_size + ssid_size + 1], _password, pass_size);
    if(_auth_method == ENTERPRISE) {
        command[cmd_wifi_size + ssid_size + 1 + pass_size] = ',';
        strncpy(&command[cmd_wifi_size + ssid_size + 1 + pass_size + 1], _username, username_size);
    }
    // The ESP can restart sometimes when trying to connect to an WiFi network (particularlly in ENTERPRISE auth mode).
    // The ESP sends a flag message when it restarts 
    if(command_mode()) {
        send_command(command, cmd_size);
        command[cmd_size] = 0; 
        db<ESP8266>(WRN) << "[ESP_CMD]" << command << endl;
        if(!check_response_1(40000000)) {
            return false;
        }
    }
    else
        return false;

    return true;
}

bool ESP8266::wifi_reconnect()
{
    if(!wifi_status())
        return wifi_connect();
    else
        return true;
}

bool ESP8266::command_mode()
{
    flush_serial();
    _uart->put('+');
    _uart->put('+');
    _uart->put('+');
    _uart->put('\r');
    _uart->put('\n');
    db<ESP8266>(WRN) << "[ESP_CMD]+++" << endl;
    return check_response_1();
}

int ESP8266::post(const char * request, unsigned int request_size, char * res, unsigned int res_size)
{
    char res_http_code[] = "+HTTPCODE=";
    if(command_mode()) {
        // db<ESP8266>(WRN) << request << endl;
        send_command(request, request_size);
        char response[20];
        int ret = wait_response(response, sizeof(response), 40000000);
        // OStream cout; cout << response << endl;
        if(ret >= 0) {
            int http_code_pos = strstr(response, res_http_code);
            if(http_code_pos >= 0) {
                int http_code_size = ret - http_code_pos - (sizeof(res_http_code)-1) - (sizeof(end_of_msg)-1) + 1;
                if(http_code_size > 0) {
                    char http_code_c[http_code_size];
                    strncpy(http_code_c, &response[http_code_pos + (sizeof(res_http_code)-1)], http_code_size-1);
                    http_code_c[http_code_size-1] = 0;
                    int http_code = atoi(http_code_c);
                    // if((http_code >= 200) && (http_code < 300))
                    //     return true;
                    return http_code;
                }
            }
        }
    }
    return -100;
}

int ESP8266::api_health_check()
{
    const char cmd_health_check[] = "AT+APIHEALTH";
    return post(cmd_health_check, sizeof(cmd_health_check)-1);
}

int ESP8266::api_attach(const void * data, unsigned int data_size)
{
    const char cmd_attach[] = "AT+APIATTACH=";
    unsigned int request_size = (sizeof(cmd_attach)-1) + data_size;
    char request[request_size]; //-1 for \0 and +2 for \r\n
    strncpy(request, cmd_attach, sizeof(cmd_attach)-1);
    memcpy(&request[sizeof(cmd_attach)-1], data, data_size);
    return post(request, request_size);
}

int ESP8266::api_put(const void * data, unsigned int data_size)
{
    const char cmd_put[] = "AT+APIPUT=";
    unsigned int request_size = (sizeof(cmd_put)-1) + data_size;
    char request[request_size]; //-1 for \0 and +2 for \r\n
    strncpy(request, cmd_put, sizeof(cmd_put)-1);
    memcpy(&request[sizeof(cmd_put)-1], data, data_size);
    return post(request, request_size);
}

bool ESP8266::wifi_status(WiFi_Status * code)
{
    const char cmd_wifi_status[] = "AT+WIFISTATUS";  
    const char res_wifi_status[] = "+WIFISTATUS=";
    if(command_mode()) {
        send_command(cmd_wifi_status, sizeof(cmd_wifi_status)-1);
        db<ESP8266>(WRN) << "[ESP_CMD]" << cmd_wifi_status << endl;
        char response[20];
        int ret = wait_response(response, sizeof(response));
        int msg_pos = strstr(response, res_wifi_status);
        if(msg_pos >= 0) {
            response[ret]=0;
            db<ESP8266>(WRN) << "[ESP_RES]" << response << endl;
            int status = response[msg_pos + (sizeof(res_wifi_status)-1)] - '0';
            if(code)
                *code = static_cast<WiFi_Status>(status);
            if(status == ESP8266::WiFi_Status::WL_CONNECTED)
                return true;
        }
    }
    return false;
}

// int ESP8266::ip(char * ip)
// {
//     const char cmd_get_ip[] = "AT+GETIP";
//     if(command_mode()) {
//         send_command(cmd_get_ip, sizeof(cmd_get_ip)-1);
//         char response[30];
//         int ret = wait_response(response, sizeof(response));
//         if(ret < 0)
//             return -1; //Timeout
//         int ip_res_pos = strstr(response, res_ip);
//         if(ip_res_pos < 0)
//             return -2; //Another response but res_ip
//         int ip_pos = ip_res_pos + (sizeof(res_ip)-1);
//         int ip_size = ret - ip_pos - (sizeof(end_of_msg)-1);
//         for(int i = 0; i < ip_size; i++)
//             _ip[i] = response[ip_pos + i];
//         _ip[ip_size] = 0;
//         memcpy(ip, _ip, ip_size+1);
//         return ip_size;
//     }
//     return -1;
// }

void ESP8266::flush_serial()
{
    // db<ESP8266>(WRN) << "flushing serial: ";
    while(_uart->ready_to_get()) {
        _uart->get();
        // db<ESP8266>(WRN) << c;
        Machine::delay(70);
    }
    // db<ESP8266>(WRN) << endl;
}

void ESP8266::send_command(const char * command, unsigned int size) {
    // db<ESP8266>(WRN) << "Sending command:";
    flush_serial();
    for(unsigned int i = 0; i < size; i++) {
        _uart->put(command[i]);
        // db<ESP8266>(WRN) << command[i];
    }
    // db<ESP8266>(WRN) << endl;
    _uart->put('\r');
    _uart->put('\n');
}

int ESP8266::wait_response(char * response, unsigned int response_size, Microsecond timeout_time)
{
    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    unsigned int i;
    for(i = 0; i < response_size; i++) {
        while(!_uart->ready_to_get())
            if(check_timeout())
                return -1;

        response[i] = _uart->get();
        // db<ESP8266>(WRN) << response[i];
        if(response[i] == '\n' && response[i-1] == '\r')
            return i+1;
    }
    return i;
}

bool ESP8266::check_response(const char * expected, Microsecond timeout_time)
{
    bool ret = true;
    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    unsigned int sz = strlen(expected);
    for(unsigned int i = 0; i < sz; i++) {
        char c;
        while(!_uart->ready_to_get())
            if(check_timeout())
                return false;

        c = _uart->get();
        // db<ESP8266>(WRN) << c;
        if(c != expected[i]) {
            ret = false;
            break;
        }
    }

    return ret;
}

/* Check if the response is OK or ERR */
bool ESP8266::check_response_1(Microsecond timeout_time)
{
    const char res_ok[] = "OK";
    const char res_err[] = "ERR";
    const char res_start[] = "+STARTESP";
    char ok[(sizeof(res_ok)-1) + sizeof(end_of_msg)];
    strncpy(ok, res_ok, sizeof(res_ok)-1);
    strncpy(&ok[sizeof(res_ok)-1], end_of_msg, sizeof(end_of_msg));

    char err[(sizeof(res_err)-1) + sizeof(end_of_msg)];
    strncpy(err, res_err, sizeof(res_err)-1);
    strncpy(&err[sizeof(res_err)-1], end_of_msg, sizeof(end_of_msg));

    bool ret = true;
    unsigned int i = 0;
    unsigned int err_index = 0;
    unsigned int rst_index = 0;
    unsigned int len = strlen(ok);

    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    while(i < len) {

        while(!_uart->ready_to_get())
            if(check_timeout())
                return false;

        char c = _uart->get();
        // db<ESP8266>(WRN) << c;
        if(c == ok[i])
            i++;
        else if(c == ok[0])
            i = 1;
        else
            i = 0;     

        //Check if is error msg
        if(c == err[err_index]) {
            err_index++;
            if(err_index >= strlen(err)) {
                // ret = false;
                // break;
                db<ESP8266>(WRN) << "[ESP_RES]" << res_err << endl;
                return false;
            }
        }
        else if(c == err[0])
            err_index = 1;
        else
            err_index = 0;

        //Check if is reset msg
        if(c == res_start[rst_index]) {
            rst_index++;
            if(rst_index >= strlen(res_start)) {
                // ret = false;
                // break;
                db<ESP8266>(WRN) << "[ESP_RES]" << res_start << endl;
                return false;
            }
        }
        else if(c == res_start[0])
            rst_index = 1;
        else
            rst_index = 0;
    }
    db<ESP8266>(WRN) << "[ESP_RES]" << res_ok << endl;
    return ret;
}

__END_SYS
#endif