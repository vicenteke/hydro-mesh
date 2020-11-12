#ifndef __cortex_lora_mesh_h
#define __cortex_lora_mesh_h

#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
// #include <stdint.h>
#include <lora_mesh.h>

__BEGIN_SYS

using namespace EPOS;

class Lora_Mesh : public Lora_Mesh_Common {
public:
    // CONSTANTS DECLARATION

    static const int LORA_RX_PIN = 5;
    // IMPORTANT: this program does not use default UART pins
    static const int LORA_TX_PIN = 6;
    // PC5 = RX & PC6 = TX
    // Make sure that is properly configured in <include/machine/cortex/emote3.h>

    static const char LORA_MESSAGE_FINAL = '~'; // Marks end of the message
    // You can change it if you need to send this char

    static const int LORA_TIMEOUT        = 1000000;


    //Commands list
    static const int MOD_PARAM	   = 0xD6;
    static const int LOCAL_READ	   = 0xE2;
    static const int REMOTE_READ   = 0xD4;
    static const int WRITE_CONFIG  = 0xCA;
    static const int COMMAND_IO	   = 0xC2;
    static const int DIAGNOSE	   = 0xE7;
    static const int NOISE		   = 0xD8;
    static const int RSSI		   = 0xD5;
    static const int TRACE_ROUTE   = 0xD2;
    static const int SEND_PACK	   =  0x28;

    // ID's
    static const int MASTER_ID     = 0;
    static const int BROADCAST_ID  = 2047;    // Sending to this ID send the command to all the nodes in network
                                            // Can be used only by gateway

    // Configurations for HYDRO_MESH
    static const int HYDRO_NET     = 1562; // Any value from 0 to 2047

    static const int HYDRO_SF      = 12;  // Ranges from 7 to 12 for SF
                                        // or 5 = FSK

    static const int HYDRO_BW      = 2;   // 0 = 125kHz
                                        // 1 = 250kHz
                                        // 2 = 500kHz

    static const int HYDRO_CR      = 4;   // 1 = 4/5
                                        // 2 = 4/6
                                        // 3 = 4/7
                                        // 4 = 4/8

    static const int HYDRO_POWER   = 20; // Ranges from 0 to 20 dBm

public:
	Lora_Mesh() {
	    // In each child class, properly initializes class
		// Must send feedback about all the steps and notify about errors
		// After that, device should be ready to make any send()

        _transparent = UART(1, 9600, 8, 0, 1);
        _transparent.loopback(false);

        // _command = UART(0, 9600, 8, 0, 1);
        // _command.loopback(false);

        _checkBit = true;
	}

	~Lora_Mesh() {}

	//GETTERS
	// int uid() {
	// 	return _uid;
	// }

	int id() {
		return _id;
	}

	int net() {
		return _net;
	}

	int sf() {
		return _sf;
	}

	int bw() {
		return _bw;
	}

	int power() {
		return _power;
	}

	int cr() {
		return _cr;
	}

    bool checkBit() {
		return _checkBit;
	}

    // SETTERS
    // void uid(int uid) {
    //     _uid = uid;
    // }

    void id(int id) {
        _id = id;
    }

    void net(int net) {
        _net = net;
    }

    void sf(int sf) {
        _sf = sf;
    }

    void bw(int bw) {
        _bw = bw;
    }

    void power(int p) {
        _power = p;
    }

    void cr(int cr) {
        _cr = cr;
    }

    void checkBit(bool b) {
        _checkBit = b;
    }

protected:

	// int _uid;			// unique ID
	int _id;			// device's ID
	int _net;			// network
	int  _sf;			// spreading factor
	int  _bw;			// bandwidth
	int  _cr;			// coding rate
	int  _power;		// power for antenna

	bool	 _checkBit;		// true if the last operation went successful.
							// Not appliable to: localRead, getParameters.

	static UART _transparent;		// transparent UART (C5/C6): send/receive data
	UART _command;			// command UART (A0/A1): send/receive configuration data and commands

    static OStream cout;

};

class Gateway_Lora_Mesh : public Lora_Mesh {

public:

    Gateway_Lora_Mesh(void (*hand)(int, char *) = &printMessage,
                    int net = HYDRO_NET,
                    int sf = HYDRO_SF,
                    int bw = HYDRO_BW,
                    int cr = HYDRO_CR,
                    int power = HYDRO_POWER) {

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

    ~Gateway_Lora_Mesh() {
        stopReceiver();
    }


    void send(int id, char* data, int len = 0) {
    // Sends data to node defined by "id"

    	db<Lora_Mesh> (WRN) << "Gateway_Lora_Mesh::sendData() called\n";

        if (len <= 0)
            len = strlen(data);

    // _interrupt.int_disable();
    	_transparent.put(id & 0xFF);
    	_transparent.put((id >> 8) & 0xFF);

    	for (int i = 0 ; i < len ; i++) {
    		_transparent.put(data[i]);
    	}
        _transparent.put(LORA_MESSAGE_FINAL);
    // _interrupt.int_enable();
    }

    void sendToAll(char* data, int len = 0) {
    // Sends data to all nodes

        send(BROADCAST_ID, data, len);
    }

    void receiver(void (*handler)(int, char *) = 0) {
    // Starts receiving messages: enables interrupts for UART
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

    void stopReceiver() {
    // Stops receiving messages: disables UART interrupt

        cout << "Gateway stopped waiting for new messages\n";

        _interrupt.int_disable();
    }

    static void printMessage(int id, char * msg) {
    // Default handler for messages

    // Notice that any function(int, char *) can be passed to constructor to be
    // the handler. Ex: Gateway_Lora_Mesh(&function)

        cout << "Received from " << id << ": " << msg << '\n';
    }

private:

    static void uartHandler(const unsigned int &) {
    // Gets data from UART and passes it for _handler to deal with

        db<Lora_Mesh>(INF) << "[UART Handler] ";

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
            if(buf != '\0' && buf != LORA_MESSAGE_FINAL)
                str[len++] = buf;
            i = 0;
            while (buf != LORA_MESSAGE_FINAL && !_transparent.ready_to_get() && i++ < LORA_TIMEOUT); // 3000: minimum delay tested for sf = 12, bw = 500, cr = 4/8
            if (i >= LORA_TIMEOUT)
                db<Lora_Mesh> (ERR) << "-----> TIMEOUT achieved for next message\n";
        }

        str[len] = '\0';

        _handler(((id[0] << 8) + id[1]), str);

        _transparent.clear_int();
        _interrupt.int_enable();
    }

private:

    static void (*_handler)(int, char *);
    static GPIO _interrupt;
};

class EndDevice_Lora_Mesh : public Lora_Mesh {

public:
    EndDevice_Lora_Mesh(int id,
                    void (*hand)(char *) = &printMessage,
                    int net = HYDRO_NET,
                    int sf = HYDRO_SF,
                    int bw = HYDRO_BW,
                    int cr = HYDRO_CR,
                    int power = HYDRO_POWER) {

        // handler: handler for messages from gateway. Receives message as parameter

        _id = id;
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

    ~EndDevice_Lora_Mesh() {
        stopReceiver();
    }


    void send(char* data, int len = 0) {
    // Sends data to gateway

    	db<Lora_Mesh> (WRN) << "EndDevice_Lora_Mesh::sendData() called\n";

        if (len <= 0)
            len = strlen(data);

    	for (int i = 0 ; i < len ; i++) {
    		_transparent.put(data[i]);
    	}

        _transparent.put(LORA_MESSAGE_FINAL);
    }

    void receiver(void (*handler)(char *) = 0) {
    // Starts receiving messages: enables interrupts for UART

        cout << "End Device " << id() << " started waiting for messages\n";

        if (handler)
            _handler = handler;
        else
            _handler = &printMessage;

        _interrupt.int_disable();
        _transparent.clear_int();
        _interrupt.int_enable();

    }

    void stopReceiver() {
    // Stops receiving messages: disables UART interrupt

        cout << "End Device " << id() << " stopped waiting for new messages\n";

        _interrupt.int_disable();
    }

    static void printMessage(char * msg) {

        cout << "> " << msg << '\n';
    }

private:
    static void uartHandler(const unsigned int &) {
    // Gets data from UART and passes it for _handler to deal with

        db<Lora_Mesh>(INF) << "[UART Handler] ";

        _interrupt.int_disable();

        char str[30];
        str[0] = '\0';

        int len = 0;
        char buf = '0';
        int i = 0;

        while (!_transparent.ready_to_get());
        while(_transparent.ready_to_get() && len <= 30) {
            buf = _transparent.get();
            if(buf != '\0' && buf != LORA_MESSAGE_FINAL)
                str[len++] = buf;
            i = 0;
            while (buf != LORA_MESSAGE_FINAL && !_transparent.ready_to_get() && i++ < LORA_TIMEOUT); // 4000: minimum delay tested for sf = 12, bw = 500, cr = 4/8
            if (i >= LORA_TIMEOUT)
                db<Lora_Mesh> (ERR) << "-----> TIMEOUT achieved for next message\n";
        }

        str[len] = '\0';

        if (str[0] >= 0 && str[0] <= 9) {
            db<Lora_Mesh> (WRN) << "Received message intended to node " << (int)((str[0] << 8) + str[1]) << ":\n";
            strncpy(str, str + 2, strlen(str) - 2);
        }
        _handler(str);

        _transparent.clear_int();
        _interrupt.int_enable();
    }

private:
    static void (*_handler)(char *);
    static GPIO _interrupt;
};

__END_SYS

#endif
