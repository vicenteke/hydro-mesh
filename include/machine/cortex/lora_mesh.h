/*
 * These classes create an interface for using the Radioenge's LoRaMESH module
 * (https://www.radioenge.com.br/solucoes/iot/34-modulo-loramesh.html) along with EPOS.
 * Notice that there is a shield for using it as well, but you can still just connect jumpers.
 * Also, it does not use default UART pins, so make sure it is properly connected.
 *
 * First of all, it is necessary to configure the devices using the software - for Windows - available on
 * Radioenge's LoRaMESH page ("Software de configuração LoRaMESH") and an FTDI. The configuration is as follows:
 *      1) Connect to it with the FTDI, really straightforward
 *      2) LOCAL READ: it gets the UID (Unique IDentifier) of the device
 *      3) Configure the network characteristics for the device
 *          ID: for the gateway it must be zero. For the end devices it must be any value from 1 to 2046.
 *          SENHA/NET: the network number has to be the same for each device.
 *
 *          Then click on WRITE CONFIG to store it. You can check if it worked with a LOCAL READ (values
 *          should remain the same).
 *      4) Configure the LoRa attributes. Spreading Factor (SF) and Bandwidth (BW) must be the same for
 *         each node, however it's very likely to set Coding Rate (CR) and Power as the same as well.
 *         Again, you can check if the values were configured.
 *
 *      As an example, here are the values we used:
 *      NET     = 1562 // Any value from 0 to 2047
 *      SF      = 12   // Ranges from 7 to 12, or 5 for FSK
 *      BW      = 125  // 125kHz, 250kHz or 500kHz
 *      CR      = 4/8  // 4/5, 4/6, 4/7 or 4/8
 *      POWER   = 20   // Ranges from 0 to 20 dBm
 *
 *
 * CLASSES:
 *      Lora_Mesh: abstract class, should not be used.
 *
 *      Gateway_Lora_Mesh: used for gateways. It does not send data to cloud, it should be implemented.
 *                         To use it, you should implement a void handler(int id_sender, char* data)
 *                         function. Then, simply use Gateway_Lora_Mesh gateway(&handler).

 *      EndDevice_Lora_Mesh: used for end devices. Use send(char*) to send data to the gateway.
 *                           It can also receive data from the GW, so the handler can be implemented too.
 */

#ifndef __cortex_lora_mesh_h
#define __cortex_lora_mesh_h

#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
#include <mutex.h>
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
public:
	Lora_Mesh() {

        _transparent = UART(1, 9600, 8, 0, 1);
        _transparent.loopback(false);
	}

	~Lora_Mesh() {}

	int id() {
		return _id;
	}

    void id(int id) {
        _id = id;
    }

protected:

    static int _id;			   // device's ID
    static UART _transparent;  // transparent UART (C5/C6): send/receive data
    static GPIO _interrupt;
    static OStream cout;
    static Mutex _mutex;       // Guarantees that the LoRa module is on
};

class Gateway_Lora_Mesh : public Lora_Mesh {

public:

    Gateway_Lora_Mesh(void (*hand)(int, char *) = &printMessage) {

    // hand: handler for messages. Receives ID and the message as parameters

        cout << "Configuring LoRa...\n";
        _id = MASTER_ID;

        _nodes.size = 0;
        _nodes.id[0] = -1;

        // Initializing interrupts
        _interrupt.handler(uartHandler, GPIO::FALLING);
        receiver(hand);
        // getNodesInNet();
        cout << "LoRa configured\n";
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

    static void send(int id, char c) {
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
        OStream kout;
        kout << "Received from " << id << ": " << msg << '\n';
    }

private:

    static void uartHandler(const unsigned int &) {
    // Gets data from UART and passes it for _handler to deal with

        // db<Lora_Mesh>(WRN) << "[UART Handler] ";

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
};

class EndDevice_Lora_Mesh : public Lora_Mesh {

public:
    EndDevice_Lora_Mesh(int id,
                    void (*hand)(char *) = &printMessage) {

        // hand: handler for messages from gateway. Receives message as parameter

        cout << "Configuring LoRa...\n";
        _id = id;

        // Initializing interrupts
        _interrupt.handler(uartHandler, GPIO::FALLING);
        receiver(hand);

        // send(LORA_GET_NODES); // Tells gateway to list this node in net
        cout << "LoRa configured\n";
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
        OStream kout;
        kout << "> " << msg << '\n';
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
};

__END_SYS

#endif
