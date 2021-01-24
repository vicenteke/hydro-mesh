#ifndef _SMART_H
#define _SMART_H

#include "index.h"
#include <smart_data.h>

using namespace EPOS;

enum {
    MAX_NODES = 40
};

// Timeout variable
TSC::Time_Stamp _init_timeout;
const RTC::Microsecond SEND_DB_SERIES_TIMEOUT = 5ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_RECORD_TIMEOUT = 5ull * 60 * 1000000;

// Test time
// const RTC::Microsecond INTEREST_EXPIRY = 1ull * 60 * 1000000;
// const RTC::Microsecond HTTP_SEND_PERIOD = 2ull * 60 * 1000000;
// const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY / 2;
// const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

// Production time
const RTC::Microsecond INTEREST_EXPIRY = 5ull * 60 * 1000000;
const RTC::Microsecond HTTP_SEND_PERIOD = 30ull * 60 * 1000000;
const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY / 2;
const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 4;//2 * 12;

typedef Smart_Data_Common::SI_Record DB_Record;
typedef Smart_Data_Common::DB_Series DB_Series;

// Credentials
const char DOMAIN[]   = "tutorial";
const char USERNAME[] = "tutorial";
const char PASSWORD[] = "tuto2018";

struct Credentials
{
    Credentials() {
        _size_domain = sizeof(DOMAIN) - 1;
        memcpy(_domain,DOMAIN,_size_domain);
        _size_username = sizeof(USERNAME) - 1;
        memcpy(_username,USERNAME,_size_username);
        _size_password = sizeof(PASSWORD) - 1;
        memcpy(_password,PASSWORD,_size_password);
    }
    char _size_domain;
    char _domain[sizeof(DOMAIN) - 1];
    char _size_username;
    char _username[sizeof(USERNAME) - 1];
    char _size_password;
    char _password[sizeof(PASSWORD) - 1];
}__attribute__((packed));

struct Attach_Payload
{
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Series series){ _series = series; }
public:
    Credentials _credentials;
    DB_Series _series;
}__attribute__((packed));

struct Put_Payload
{
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Record smartdata){ _smartdata = smartdata; }
public:
    Credentials _credentials;
    DB_Record _smartdata;
}__attribute__((packed));

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
    USB io;
};

/*
 * Stores series already created based on received "Usr"
 * Obs: creating the series is up to the application
 */
class Series_Logger {
public:
    Series_Logger() {
        _length = 0;
        for (int i = 0; i < MAX_NODES; i++) {
            _log[i] = -1;
        }
    }

    ~Series_Logger() {}

    /*
     * Checks if series 'usr' has been created; if hasn't, stores 'usr' in _log
     *
     * @return false if 'usr' was already in _log, true if it was stored in _log
     */
    int add(int usr) {

        for (int i = 0; i < _length; i++) {
            if (usr == _log[i]) return false;
        }

        _log[_length++] = usr;
        return true;
    }

    /*
     * Removes 'usr' from _log
     *
     * @return false if 'usr' wasn't in _log, true if it was removed
     */
    int remove(int usr) {
        bool found = false;
        for (int i = 0; i < _length; i++) {
            if (found) {
                _log[i] = _log[i + 1];
            } else if (usr == _log[i]) {
                found = true;
                _log[i] = _log[i + 1];
            }
        }
        if (found) {
            _log[_length--] = -1
        }

        return found;
    }

    int length() { return _length; }

private:
    int _log[MAX_NODES];
    unsigned short _length;
};

class Smart_Data_Hydro_Mesh {
public:
    Smart_Data_Hydro_Mesh() {
        level = new Water_level();
        turb  = new Water_Turbidity();
        pluv  = new Rain();
    }

    ~Smart_Data_Hydro_Mesh {
        delete level;
        delete turb;
        delete pluv;
    }

    Water_Level * level() { return level; };
    Water_Turbidity * turbidity() { return turb; }
    Rain * pluviometry() { return pluv; }

private:
    // My_Temperature t(0, 1500000, My_Temperature::PRIVATE, 5000000); // dev=0, expiry=15s, mode=PRIVATE, period=5s
    // Printer<My_Temperature> p(&t);

    Water_Level * level;
    Water_Turbidity * turb;
    Rain * pluv;
};

#endif
