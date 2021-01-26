#ifndef _SMART_H
#define _SMART_H

#include "index.h"
#include <utility/string.h>

using namespace EPOS;

// Credentials
const char DOMAIN[]   = "tutorial";
const char USERNAME[] = "tutorial";
const char PASSWORD[] = "tuto2018";

enum {
    MAX_NODES = 40
};

/*
 * Stores and creates series based on received "Usr"
 */
class Series_Logger {
public:
    Series_Logger() {
        _length = 0;
        for (int i = 0; i < MAX_NODES; i++) {
            _log[i] = -1;
        }
        sendCredentials();
        Alarm::delay(400000);
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
            _log[_length--] = -1;
        }

        return found;
    }

    int length() { return _length; }

    /*
     * Sends credentials (the ones set in the beginning of this file) for loragw
     *
     * @return 0 if no credentials are available, 1 when sent
     */
    int sendCredentials() {
        if (strlen(DOMAIN) < 2) return 0;
        if (strlen(USERNAME) < 2) return 0;
        if (strlen(PASSWORD) < 2) return 0;

        char c = 0;
        io.put('%');
        do {
            while (!io.ready_to_get());
            c = io.get();
        } while (c != '%');

        for (int i = 0; i < strlen(DOMAIN); i++) {
            io.put(DOMAIN[i]);
        }
        for (int i = 0; i < 3; i++) {
            io.put('X');
        }
        for (int i = 0; i < strlen(USERNAME); i++) {
            io.put(USERNAME[i]);
        }
        for (int i = 0; i < 3; i++) {
            io.put('X');
        }
        for (int i = 0; i < strlen(PASSWORD); i++) {
            io.put(PASSWORD[i]);
        }
        for (int i = 0; i < 3; i++) {
            io.put('X');
        }

        return 1;
    }

private:
    USB io;
    int _log[MAX_NODES];
    unsigned short _length;
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
