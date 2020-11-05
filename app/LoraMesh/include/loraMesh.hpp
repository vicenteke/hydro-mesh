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

// Declaring LoraMesh static members
OStream LoraMesh::cout;
UART LoraMesh::_transparent;

class GatewayLoraMesh : public LoraMesh {

public:

    GatewayLoraMesh(void (*hand)(int, char *) = &printMessage,
                    uint16_t net = HYDRO_NET,
                    uint8_t sf = HYDRO_SF,
                    uint8_t bw = HYDRO_BW,
                    uint8_t cr = HYDRO_CR,
                    uint8_t power = HYDRO_POWER) {

    // handler: handler for messages. Receives ID and the message as parameters

        _id = MASTER_ID;

        _net    = net;
        _sf     = sf;
        _bw     = bw;
        _cr     = cr;
        _power  = power;

        // Initializing interrupts
        _handler = hand;
        _interrupt.handler(uartHandler, GPIO::FALLING);
        receiver();
    }

    ~GatewayLoraMesh() {
        stopReceiver();
    }

    void send(uint16_t id, char* data);
    void sendToAll(char* data);
    void receiver(void (*hand)(int, char *) = _handler ? _handler : &printMessage);
    void stopReceiver();
    static void printMessage(int, char *);

private:
    static void uartHandler(const unsigned int &);
    static void (*_handler)(int, char *);
    static GPIO _interrupt;
};

// Declaring GatewayLoraMesh static members
GPIO GatewayLoraMesh::_interrupt('C', 3, GPIO::IN);
void (*GatewayLoraMesh::_handler)(int, char *);

class EndDeviceLoraMesh : public LoraMesh {

public:
    EndDeviceLoraMesh(uint16_t id,
                    void (*handler)(char *) = &printMessage,
                    uint16_t net = HYDRO_NET,
                    uint8_t sf = HYDRO_SF,
                    uint8_t bw = HYDRO_BW,
                    uint8_t cr = HYDRO_CR,
                    uint8_t power = HYDRO_POWER) {

        // handler: handler for messages from gateway. Receives message as parameter

        _id = id;
        _net    = net;
        _sf     = sf;
        _bw     = bw;
        _cr     = cr;
        _power  = power;

        receiver(handler);
    }

    ~EndDeviceLoraMesh() {
        stopReceiver();
    }

    void send(char* data);
    void receiver(void (*handler)(char *) = &printMessage);
    void stopReceiver();
    static void printMessage(char *);

private:

    Thread * _receiver;
    static int keepReceiving(void (*handler)(char *));
};

// IMPLEMENTATIONS FOR CLASS GatewayLoraMesh

void GatewayLoraMesh::send(uint16_t id, char* data) {
// Sends data to node defined by "id"

	db<Lora> (WRN) << "GatewayLoraMesh::sendData() called\n";

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

void GatewayLoraMesh::uartHandler(const unsigned int &) {
// Gets data from UART and passes it for _handler to deal with

    db<Lora>(INF) << "[UART Handler] ";

    _interrupt.int_disable();

    char str[30];
    str[0] = '\0';

    int len = 0;
    char buf = '0';
    int id[] = {10, 10};
    int i = 0;

    while (!_transparent.ready_to_get());
    id[1] = _transparent.get();
    id[0] = _transparent.get();

    while (!_transparent.ready_to_get());
    while(_transparent.ready_to_get() && len <= 30) {
        buf = _transparent.get();
        str[len++] = buf;
        i = 0;
        while (!_transparent.ready_to_get() && i++ < 3000); // 3000: minimum delay tested for sf = 12, bw = 500, cr = 4/8
    }

    str[len] = '\0';

    _handler(((id[0] << 8) + id[1]), str);

    _transparent.clear_int();
    _interrupt.int_enable();
}

void GatewayLoraMesh::receiver(void (*handler)(int, char *)) {
// Enables interrupts for UART
// handler: handler for messages. Receives ID and the message as parameters

    cout << "Gateway started waiting for messages\n";

    if (handler)
        _handler = handler;
    else
        _handler = &printMessage;

    _interrupt.int_disable();
    _transparent.clear_int();
    _interrupt.int_enable();
}

void GatewayLoraMesh::stopReceiver() {
// Stops receiving messages: disables UART interrupt

    cout << "Gateway stopped waiting for new messages\n";

    _interrupt.int_disable();
}

void GatewayLoraMesh::printMessage(int id, char * msg) {
// Default handler for messages

    cout << "Received from " << id << ": " << msg << '\n';
}

// IMPLEMENTATIONS FOR CLASS EndDeviceLoraMesh

void EndDeviceLoraMesh::send(char* data) {
// Sends data to gateway

	db<Lora> (WRN) << "EndDeviceLoraMesh::sendData() called\n";

	for (int i = 0 ; i < strlen(data) ; i++) {
		_transparent.put(data[i]);
	}
}

void EndDeviceLoraMesh::receiver(void (*handler)(char *)) {
// Creates receiver thread

    cout << "End Device " << id() << " started waiting for messages\n";

    _receiver = new Thread(&keepReceiving, handler);
    // int status_receiver = _receiver->join();

}
void EndDeviceLoraMesh::stopReceiver() {
// Kills receiver thread

    cout << "End Device " << id() << " stopped waiting for new messages\n";

    if (_receiver)
        delete _receiver;
}

int EndDeviceLoraMesh::keepReceiving(void (*handler)(char *)) {
// Creates a loop to check UART

    db<Lora> (WRN) << "EndDeviceLoraMesh::keepReceiving() called\n";

    char str[30];
    str[0] = '\0';

    int len = 0;
    char buf = '0';

    while (1) {
        while (!_transparent.ready_to_get());
        while(_transparent.ready_to_get() && len <= 30) {
            buf = _transparent.get();
            str[len++] = buf;
            Alarm::delay((int)(3000));
        }

        str[len] = '\0';

        handler(str);

        str[0] = '\0';
        len = 0;
    }

    return 0;
}

void EndDeviceLoraMesh::printMessage(char * msg) {

    cout << "> " << msg << '\n';
}

#endif
