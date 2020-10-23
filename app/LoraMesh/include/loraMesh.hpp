#ifndef _LORAMESH_H
#define _LORAMESH_H

#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
// #include <machine/cortex_m/emote3_gptm.h>
// #include <machine/cortex_m/emote3_gprs.h>

#include <cstdint>
// #include <vector>

#include "defines.h"

using namespace EPOS;

class LoraMesh {

public:
	LoraMesh() {
	    // In each child class, properly initializes class
		// Must send feedback about all the steps and notify about errors
		// After that, device should be ready to make any send()

        _transparent = UART(1, 9600, 8, 0, 1);
        _transparent.loopback(false);

        // _command = UART(0, 9600, 8, 0, 1);
        // _command.loopback(false);

        _checkBit = true;
	}

	~LoraMesh() {}

	//GETTERS
	// uint32_t uid() {
	// 	return _uid;
	// }

	uint16_t id() {
		return _id;
	}

	uint16_t net() {
		return _net;
	}

	uint8_t sf() {
		return _sf;
	}

	uint8_t bw() {
		return _bw;
	}

	uint8_t power() {
		return _power;
	}

	uint8_t cr() {
		return _cr;
	}

    bool checkBit() {
		return _checkBit;
	}

    // SETTERS
    // void uid(uint32_t uid) {
    //     _uid = uid;
    // }

    void id(uint16_t id) {
        _id = id;
    }

    void net(uint16_t net) {
        _net = net;
    }

    void sf(uint8_t sf) {
        _sf = sf;
    }

    void bw(uint8_t bw) {
        _bw = bw;
    }

    void power(uint8_t p) {
        _power = p;
    }

    void cr(uint8_t cr) {
        _cr = cr;
    }

    void checkBit(bool b) {
        _checkBit = b;
    }

protected:

	// uint32_t _uid;			// unique ID
	uint16_t _id;			// device's ID
	uint16_t _net;			// network
	uint8_t  _sf;			// spreading factor
	uint8_t  _bw;			// bandwidth
	uint8_t  _cr;			// coding rate
	uint8_t  _power;		// power for antenna

	bool	 _checkBit;		// true if the last operation went successful.
							// Not appliable to: localRead, getParameters.

	static UART _transparent;		// transparent UART (A0/A1): send/receive data
	UART _command;			// command UART (C3/C4): send/receive configuration data and commands

    static OStream cout;

};

class GatewayLoraMesh : public LoraMesh {

public:

    GatewayLoraMesh(uint16_t net = HYDRO_NET,
                    uint8_t sf = HYDRO_SF,
                    uint8_t bw = HYDRO_BW,
                    uint8_t cr = HYDRO_CR,
                    uint8_t power = HYDRO_POWER) {

        _id = MASTER_ID;

        _net    = net;
        _sf     = sf;
        _bw     = bw;
        _cr     = cr;
        _power  = power;

        receiver();
    }

    ~GatewayLoraMesh() {
        stopReceiver();
    }

    void send(uint16_t id, char* data);
    void sendToAll(char* data);
    void receiver();
    void stopReceiver();

private:

    Thread * _receiver;
    static int keepReceiving();
};

class EndDeviceLoraMesh : public LoraMesh {

public:
    EndDeviceLoraMesh(uint16_t id,
                    uint16_t net = HYDRO_NET,
                    uint8_t sf = HYDRO_SF,
                    uint8_t bw = HYDRO_BW,
                    uint8_t cr = HYDRO_CR,
                    uint8_t power = HYDRO_POWER) {

        _id = id;
        _net    = net;
        _sf     = sf;
        _bw     = bw;
        _cr     = cr;
        _power  = power;

        receiver();
    }

    ~EndDeviceLoraMesh() {
        stopReceiver();
    }

    void send(char* data);
    void receiver();
    void stopReceiver();

private:

    Thread * _receiver;
    static int keepReceiving();
};

// IMPLEMENTATIONS FOR CLASS GatewayLoraMesh

void GatewayLoraMesh::send(uint16_t id, char* data) {
// Sends data to node defined by "id"

	db<LoraMesh> (TRC) << "GatewayLoraMesh::sendData() called\n";

	_transparent.put(id & 0xFF);
	_transparent.put((id >> 8) & 0xFF);

	for (int i = 0 ; i < strlen(data) ; i++) {
		_transparent.put(data[i]);
	}
}

void GatewayLoraMesh::sendToAll(char* data) {
// Sends data to all nodes

    send(BROADCAST_ID, data);
}

void GatewayLoraMesh::receiver() {
// Creates receiver thread

    cout << "Gateway started waiting for messages\n";

    _receiver = new Thread(&keepReceiving);
    int status_receiver = _receiver->join();

}
void GatewayLoraMesh::stopReceiver() {
// Kills receiver thread

    cout << "Gateway stopped waiting for new messages\n";

    if (_receiver)
        delete _receiver;
}

int GatewayLoraMesh::keepReceiving() {
// Creates a loop to check UART

    db<LoraMesh> (TRC) << "GatewayLoraMesh::keepReceiving() called\n";

    char str[30];
    str[0] = '\0';

    int len = 0;
    char buf = '0';
    int id[] = {10, 10};

    while (1) {
        while (!_transparent.ready_to_get());
        id[1] = _transparent.get();
        id[0] = _transparent.get();
        Alarm::delay((int)(6000));
        while(_transparent.ready_to_get() && len <= 30) {
            buf = _transparent.get();
            str[len++] = buf;
            Alarm::delay((int)(500)); // 500 for 115200
        }

        str[len] = '\0';

        cout << "Received from " << (char)(id[0]/10 + '0') << (char)(id[0]%10 + '0')
            << (char)(id[1]/10 + '0') << (char)(id[1]%10 + '0') << ": " << str << '\n';

        str[0] = '\0';
        len = 0;
        id[0] = id[1] = 10;
    }

    return 0;
}

// IMPLEMENTATIONS FOR CLASS EndDeviceLoraMesh

void EndDeviceLoraMesh::send(char* data) {
// Sends data to gateway

	db<LoraMesh> (TRC) << "EndDeviceLoraMesh::sendData() called\n";

	for (int i = 0 ; i < strlen(data) ; i++) {
		_transparent.put(data[i]);
	}
}

void EndDeviceLoraMesh::receiver() {
// Creates receiver thread

    cout << "End Device " << id() << " started waiting for messages\n";

    _receiver = new Thread(&keepReceiving);
    // int status_receiver = _receiver->join();

}
void EndDeviceLoraMesh::stopReceiver() {
// Kills receiver thread

    cout << "End Device " << id() << " stopped waiting for new messages\n";

    if (_receiver)
        delete _receiver;
}

int EndDeviceLoraMesh::keepReceiving() {
// Creates a loop to check UART

    db<LoraMesh> (TRC) << "EndDeviceLoraMesh::keepReceiving() called\n";

    char str[30];
    str[0] = '\0';

    int len = 0;
    char buf = '0';

    while (1) {
        while (!_transparent.ready_to_get());
        while(_transparent.ready_to_get() && len <= 30) {
            buf = _transparent.get();
            str[len++] = buf;
            Alarm::delay((int)(500)); // 500 for 115200
        }

        str[len] = '\0';

        cout << "> " << str << '\n';

        str[0] = '\0';
        len = 0;
    }

    return 0;
}

#endif
