#ifndef _SMART_H
#define _SMART_H

#include "index.h"
// #include <smart_data.h> // Smart_Data not even compiling

#include <tstp.h>

struct DB_Series {
    unsigned char version;
    unsigned long unit;
    long x;
    long y;
    long z;
    unsigned long r;
    unsigned long long t0;
    unsigned long long t1;
    unsigned long dev;
    friend OStream & operator<<(OStream & os, const DB_Series & d) {
        os << "{ve=" << d.version << ",u=" << d.unit << ",dst=(" << d.x << "," << d.y << "," << d.z << ")+" << d.r << ",t=[" << d.t0 << "," << d.t1 << "]}";
        return os;
    }
}__attribute__((packed));

struct DB_Record {
    unsigned char version;
    unsigned long unit;
    double value;
    unsigned char error;
    unsigned char confidence;
    long x;
    long y;
    long z;
    unsigned long long t;
    unsigned long dev;
    /*friend OStream & operator<<(OStream & os, const SI_Record & d) {
        unsigned long long ll = *const_cast<unsigned long long*>(reinterpret_cast<const unsigned long long*>(&d.value));
        ll = ((ll&0xFFFFFFFF)<<32) + (ll>>32);
        double val_to_print = *reinterpret_cast<double*>(&ll);
        os << "{ve=" << d.version << ",u=" << d.unit << ",va=" << val_to_print << ",e=" << d.error << ",src=(" << d.x << "," << d.y << "," << d.z << "),t=" << d.t << ",d=" << d.dev << "}";
        return os;
    }*/
}__attribute__((packed));

enum {
    STATIC_VERSION = (1 << 4) + (1 << 0),
    MOBILE_VERSION = (1 << 4) + (2 << 0),
};

using namespace EPOS;

// Credentials
const char DOMAIN[]   = "tutorial";
const char USERNAME[] = "tutorial";
const char PASSWORD[] = "tuto20182";

enum {
    MAX_NODES = 40
};

/*
 * @brief Stores and creates series based on received "Usr"
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
    bool add(int usr) {

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
    bool remove(int usr) {
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
            _log[_length--] = -1;
        }

        return found;
    }

    int length() { return _length; }

private:
    int _log[MAX_NODES];
    unsigned short _length;
};

/*
 * @brief Responsible for communication between eMote3 and PC (loragw)
 */
class Serial_Link {
public:
    Serial_Link(){
        sendCredentials();
        Alarm::delay(400000);
    }

    ~Serial_Link(){}

    /*
     * Checks if series 'usr' has been created; if hasn't, stores 'usr' in _log
     *
     * @return false if 'usr' was already in _log, true if it was stored in _log
     */
    bool add(int usr) { return series.add(usr); }

    /*
     * Removes 'usr' from _log
     *
     * @return false if 'usr' wasn't in _log, true if it was removed
     */
    bool remove(int usr) { return series.remove(usr); }

    /*
     * @brief Sends credentials (the ones set in the beginning of this file) for loragw
     * @return 0 if no credentials are available, 1 when sent
     */
    int sendCredentials() {
        if (strlen(DOMAIN) < 2) return 0;
        if (strlen(USERNAME) < 2) return 0;
        if (strlen(PASSWORD) < 2) return 0;

        char c = 0;
        int timeout = 1000000;
        io.put('%');
        do {
            while (!io.ready_to_get() && timeout-- > 0);
            if (timeout > 0)
                c = io.get();
            else {
                io.put('%');
                timeout = 1000000;
            }
        } while (c != '%');

        for (unsigned int i = 0; i < strlen(DOMAIN); i++) {
            io.put(DOMAIN[i]);
        }
        for (int i = 0; i < 3; i++) {
            io.put('X');
        }
        for (unsigned int i = 0; i < strlen(USERNAME); i++) {
            io.put(USERNAME[i]);
        }
        for (int i = 0; i < 3; i++) {
            io.put('X');
        }
        for (unsigned int i = 0; i < strlen(PASSWORD); i++) {
            io.put(PASSWORD[i]);
        }
        for (int i = 0; i < 3; i++) {
            io.put('X');
        }

        return 1;
    }

    /*
     * @brief Sends data to loragw to create a new series
     * @return the char it receives from loragw ('S' = success, 'F' = fail)
     */
    char createSeries(DB_Series & db_series) {
        char* data = reinterpret_cast<char*>(&db_series);
        io.put('S');
        for (unsigned int i = 0; i < sizeof(DB_Series); i++)
        {
            io.put(data[i]);
        }
        for (int i = 0; i < 3; ++i)
        {
            io.put('X');
        }
        while (!io.ready_to_get());
        return io.get();
    }

    /*
     * @brief Finishes a series
     * @return the char it receives from loragw ('S' = success, 'F' = fail)
     * Error: no https://iot.lisha.ufsc.br/api/finish.php (error 404)
     */
    char finishSeries(DB_Series & db_series) {
        // if (!series.remove((int)db_series.dev)) return;
        char* data = reinterpret_cast<char*>(&db_series);
        io.put('F');
        for (unsigned int i = 0; i < sizeof(DB_Series); i++)
        {
            io.put(data[i]);
        }
        for (int i = 0; i < 3; ++i)
        {
            io.put('X');
        }
        while (!io.ready_to_get());
        return io.get();
    }

    /*
     * @brief Sends a record to loragw to be stored
     * @return the char it receives from loragw ('S' = success, 'F' = fail)
     */
    char sendRecord(DB_Record & db_record) {
        char* data = reinterpret_cast<char*>(&db_record);
        io.put('R');
        for (unsigned int i = 0; i < sizeof(DB_Record); i++)
        {
            io.put(data[i]);
        }
        for (int i = 0; i < 3; ++i)
        {
            io.put('X');
        }
        while (!io.ready_to_get());
        return io.get();
    }

private:
    USB io;
    Series_Logger series;
};

/*class Smart_Data_Hydro_Mesh {
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
};*/

#endif
