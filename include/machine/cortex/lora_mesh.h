#ifndef __cortex_lora_mesh_h
#define __cortex_lora_mesh_h

#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
#include <mutex.h>
// #include <stdint.h>
#include <lora_mesh.h>

__BEGIN_SYS

using namespace EPOS;

class Lora_Mesh : public Lora_Mesh_Common {
public:
    // CONSTANTS DECLARATION

    // IMPORTANT: this program does not use default UART pins
    // Make sure that is properly configured in <include/machine/cortex/emote3.h>
    // PC6 = RX & PC7 = TX
    static const int LORA_RX_PIN = 6;
    static const int LORA_TX_PIN = 7;

    static const int LORA_SUPPLY_PIN_1 = 5; // Provides power to the LoRa MESH module

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
    static const int SEND_PACK	   = 0x28;

    // Commands for Application
    // You can change those values if you need to send those chars
    static const char LORA_MESSAGE_FINAL = '~'; // Marks end of the message
    static const char LORA_GET_NODES = '#'; // Used by getNodesInNet()
    static const char LORA_REMOVE_NODE = '?'; // Used by ~EndDevice

    // Values
    static const int LORA_TIMEOUT        = 1000000;
    static const int LORA_MAX_NODES      = 15; // Used by GW to create _nodes

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

        turnOn();

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

    static void turnOn() {
        _supply.set();
        Alarm::delay(2000000);
        _isOn = true;
        _mutex.unlock();
        cout << "Module is on\n";
    }

    static void turnOff() {
        _mutex.lock();
        _isOn = false;
        _supply.clear();
        Alarm::delay(20000);
        cout << "Module is off\n";
    }

protected:

	// int _uid;			// unique ID
	static int _id;			// device's ID
	int _net;			// network
	int  _sf;			// spreading factor
	int  _bw;			// bandwidth
	int  _cr;			// coding rate
	int  _power;		// power for antenna

	bool	 _checkBit;		// true if the last operation went successful.
							// Not appliable to: localRead, getParameters.

	static UART _transparent;		// transparent UART (C5/C6): send/receive data
	// UART _command;			// command UART (A0/A1): send/receive configuration data and commands

    static OStream cout;
    static GPIO _supply; // GPIO that supplies the LoRa module
    static bool _isOn;
    static Mutex _mutex; // Guarantees that the LoRa module is on

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
        _nodes.size = 0;
        _nodes.id[0] = -1;

        // Initializing interrupts
        _handler = hand;
        _interrupt.handler(uartHandler, GPIO::FALLING);
        receiver();
        getNodesInNet();
    }

    ~Gateway_Lora_Mesh() {
        stopReceiver();
        cout << "~Gateway() destructor called\n";
    }


    void send(int id, char* data, int len = 0) {
    // Sends data to node defined by "id"

    	db<Lora_Mesh> (INF) << "Gateway_Lora_Mesh::send() called\n";

        if (len <= 0)
            len = strlen(data);

    // _interrupt.int_disable();
        // if(!_isOn) turnOn();
        _mutex.lock();
    	_transparent.put(id & 0xFF);
    	_transparent.put((id >> 8) & 0xFF);

    	for (int i = 0 ; i < len ; i++) {
    		_transparent.put(data[i]);
    	}
        _transparent.put(LORA_MESSAGE_FINAL);
        _mutex.unlock();
    // _interrupt.int_enable();
    }

    void send(int id, char c) {
    // Sends char to node defined by "id"

        db<Lora_Mesh> (INF) << "Gateway_Lora_Mesh::send(char) called\n";

        // if(!_isOn) turnOn();
        _mutex.lock();

    	_transparent.put(id & 0xFF);
    	_transparent.put((id >> 8) & 0xFF);
        _transparent.put(c);
        _transparent.put(LORA_MESSAGE_FINAL);

        _mutex.unlock();
    }

    void sendToAll(char* data, int len = 0) {
    // Sends data to all nodes

        send(BROADCAST_ID, data, len);
    }

    void getNodesInNet() {
    // Gets the ID's of the nodes in the network

        db<Lora_Mesh> (INF) << "Gateway_Lora_Mesh::getNodesInNet() called\n";

        for (int i = 0; i < _nodes.size; i++) {
            _nodes.id[i] = -1;
        }
        _nodes.size = 0;

        send(BROADCAST_ID, LORA_GET_NODES);
    }

    static void removeNode(int id) {
        // Removes node from _nodes

        if(_nodes.size == 0) return;

        int i;
        for (i = 0; i <= _nodes.size; i++)
            if (id == _nodes.id[i])
                break;

        while (i <= _nodes.size) {
            _nodes.id[i] = _nodes.id[i+1];
            i++;
        }

        _nodes.size -= 1;
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

        // db<Lora_Mesh>(INF) << "[UART Handler] ";

        _mutex.lock();
        _interrupt.int_disable();

        char str[30];
        str[0] = '\0';

        int len = 0;
        char buf = '0';
        int id[] = {10, 10};
        int i = 0;

        // Getting data
        while (!_transparent.ready_to_get());
        id[1] = _transparent.get();
        id[0] = _transparent.get();

        while (!_transparent.ready_to_get());
        while(_transparent.ready_to_get() && len <= 30) {
            buf = _transparent.get();
            if(buf != '\0' && buf != LORA_MESSAGE_FINAL)
                str[len++] = buf;

            i = 0;
            while (buf != LORA_MESSAGE_FINAL && !_transparent.ready_to_get() && i++ < LORA_TIMEOUT);
            if (i >= LORA_TIMEOUT)
                db<Lora_Mesh> (ERR) << "-----> TIMEOUT achieved for next message\n";
        }

        str[len] = '\0';

        // Looking for commands
        if (str[1] == '\0') {
            switch(str[0]) {
                case LORA_GET_NODES:
                    addNode((id[0] << 8) + id[1]);
                    break;
                case LORA_REMOVE_NODE:
                    removeNode((id[0] << 8) + id[1]);
                    break;
                default:
                    _handler(((id[0] << 8) + id[1]), str);
                    break;
            }
        } else
            _handler(((id[0] << 8) + id[1]), str);

        _transparent.clear_int();
        _interrupt.int_enable();
        _mutex.unlock();
    }

    static void addNode(int id) {

        _nodes.id[_nodes.size++] = id;

        cout << _nodes.size << " nodes in net:";

        for (int i = 0; i < _nodes.size; i++)
            cout << " ["<< _nodes.id[i] << ']';

        cout << '\n';
    }

public:
    typedef struct nodes { // Stores the ID's of nodes in the net
        int size;
        int id[LORA_MAX_NODES];
    } nodes_t;

private:

    static nodes_t _nodes;
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

        send(LORA_GET_NODES); // Tells gateway to list this node in net
    }

    ~EndDevice_Lora_Mesh() {
        send(LORA_REMOVE_NODE);
        stopReceiver();
    }


    void send(char* data, int len = 0) {
    // Sends data to gateway

    	db<Lora_Mesh> (INF) << "EndDevice_Lora_Mesh::send() called\n";

        if (len <= 0)
            len = strlen(data);

        // if(!_isOn) turnOn();
        _mutex.lock();

    	for (int i = 0 ; i < len ; i++) {
    		_transparent.put(data[i]);
    	}

        _transparent.put(LORA_MESSAGE_FINAL);
        _mutex.unlock();
    }

    static void send(char c) {
    // Sends data to gateway

    	db<Lora_Mesh> (INF) << "EndDevice_Lora_Mesh::send(char) called\n";

        // if(!_isOn) turnOn();
        _mutex.lock();

		_transparent.put(c);
        _transparent.put(LORA_MESSAGE_FINAL);

        _mutex.unlock();
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

        // db<Lora_Mesh>(INF) << "[UART Handler] ";

        _mutex.lock();
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

        if (str[1] == '\0') {
            switch(str[0]) {
                case LORA_GET_NODES:
                    // Alarm::delay(_id * 1000);
                    for (int k = 0; k < _id * 1000; k++);
                    _mutex.unlock();
                    send(LORA_GET_NODES);
                    _mutex.lock();
                    break;
                default:
                    _handler(str);
                    break;
            }
        } else
            _handler(str);

        _transparent.clear_int();
        _interrupt.int_enable();
        _mutex.unlock();
    }

private:
    static void (*_handler)(char *);
    static GPIO _interrupt;
};

__END_SYS

#endif
