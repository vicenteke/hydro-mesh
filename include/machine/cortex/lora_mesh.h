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
 *      Lora_Mesh: abstract class, should not be used. Each device shall use a single object, either a
 *                 Gateway_Lora_Mesh or an EndDevice_Lora_Mesh.
 *
 *      Gateway_Lora_Mesh: used for gateways. It does not send data to cloud, it should be implemented.
 *                         To use it, you should implement a void handler(int id_sender, char* data)
 *                         function. Then, simply use Gateway_Lora_Mesh gateway(&handler).
 *
 *      EndDevice_Lora_Mesh: used for end devices. Use send(char*) to send data to the gateway.
 *                           It can also receive data from the GW, so the handler can be implemented too.
 *
 * TIME SYNCHRONIZATION:
 *      Basically, the GW gets the initial timestamp (epoch) - when connecting to the PC, the script
 *      'loragw' is responsible for that - and uses a timer to count the seconds since epoch.
 *      The ED gets the timestamp from the GW and counts the seconds in the same way. There might appear
 *      a small delay - usually no longer than a few seconds - due to the time-on-air from the messages and some
 *      imprecision from timers.
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
#include <timer.h>
#include <tstp.h>

__BEGIN_SYS

using namespace EPOS;

// Timers
class GW_Timer {
// Uses timer and connects to PC in order to get proper timestamp

public:
    GW_Timer() {
        _count = 0;
        _epoch = 0;
        getEpoch();
        _timer = new User_Timer(0, 1000000, &handler, true);
    }

    ~GW_Timer() {
        delete _timer;
    }

    void getEpoch() {
        // Gets epoch from PC (loragw script should be running)
        unsigned long long epoch = 0;
        unsigned int bytes = 0;
        char c = 0;
        io.put('@');
        do {
            while(!io.ready_to_get());
            c = io.get();
        } while(c != 'X');
        while(bytes < sizeof(unsigned long long)){
            while(!io.ready_to_get());
            c = io.get();
            epoch |= (static_cast<unsigned long long>(c) << ((bytes++)*8));
        }

        _epoch = epoch;
        _count = 0;
    }

    unsigned long long count() { return _count; }
    unsigned long long epoch() { return _epoch; }
    unsigned long long currentTime() { return _count + _epoch;}

private:

    USB io;                           // Connection to PC

    User_Timer* _timer;               // Increments _count every second
    static unsigned long long _count; // Seconds since _epoch
    unsigned long long _epoch;        // Starter time

    static void handler(const unsigned int &) { _count += 1; }
};

class ED_Timer {
// Counts elapsed time since epoch (obtained from GW)

public:
    ED_Timer() {
        _count = 0;
        _epoch = 0;
        _timer = new User_Timer(0, 1000000, &handler, true);
    }
    ~ED_Timer() {
        delete _timer;
    }

    void reset() { _count = 0; }

    void epoch(unsigned long long t) { _epoch = t; }

    unsigned long long count() { return _count; }
    unsigned long long epoch() { return _epoch; }
    unsigned long long currentTime() { return _count + _epoch;}

private:

    User_Timer* _timer;               // Increments _count every second
    static unsigned long long _count; // Seconds since _epoch
    unsigned long long _epoch;        // Starter time

    static void handler(const unsigned int &) { _count += 1; }
};

// LoRa Classes
class Lora_Mesh : public Lora_Mesh_Common {
public:
    // CONSTANTS DECLARATION

    // IMPORTANT: this program does not use default UART pins
    // Make sure that is properly configured in <include/machine/cortex/emote3.h>
    // PC6 = RX & PC7 = TX
    static const int LORA_RX_PIN = 6;
    static const int LORA_TX_PIN = 7;

    // You can change those values if you have to send these chars
    static const char LORA_MESSAGE_FINAL  = '~'; // Marks end of the message
    static const char LORA_SEND_TIMESTAMP = '#'; // Used by getNodesInNet()

    // Values
    static const int LORA_TIMEOUT             = 100000;
    static const int LORA_MAX_NODES           = 15; // Used by GW to create _nodes
    static const int LORA_FINAL_COUNT         =  4; // Number of times LORA_MESSAGE_FINAL is repeated
    static const unsigned int LORA_MAX_LENGTH = 40; // Max length for message

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

        cout << "Configuring Lora...\n";
        _id = MASTER_ID;

        // Initializing timer
        _timer = new GW_Timer();
        Alarm::delay(500000);

        // Initializing interrupts
        _interrupt.handler(uartHandler, GPIO::FALLING);
        receiver(hand);

        cout << "Lora configured\n";
    }

    ~Gateway_Lora_Mesh() {
        db<Lora_Mesh>(WRN) << "~Gateway() destructor called\n";
        stopReceiver();
        delete _timer;
    }

    void send(int id, char* data, int len = 0) {
    // Sends data to node defined by "id"

    	db<Lora_Mesh> (INF) << "Gateway_Lora_Mesh::send() called\n";

        if (len <= 0)
            len = strlen(data);

        // if(!_isOn) turnOn();
        _mutex.lock();
    	_transparent.put(id & 0xFF);
    	_transparent.put((id >> 8) & 0xFF);

    	for (int i = 0 ; i < len ; i++) {
    		_transparent.put(data[i]);
    	}

        for (int i = 0; i < LORA_FINAL_COUNT; i++) {
            _transparent.put(LORA_MESSAGE_FINAL);
        }
        _mutex.unlock();
    }

    static void send(int id, char c) {
    // Sends char to node defined by "id"

        db<Lora_Mesh> (INF) << "Gateway_Lora_Mesh::send(char) called\n";

        // if(!_isOn) turnOn();
        _mutex.lock();

    	_transparent.put(id & 0xFF);
    	_transparent.put((id >> 8) & 0xFF);
        _transparent.put(c);
        for (int i = 0; i < LORA_FINAL_COUNT; i++) {
            _transparent.put(LORA_MESSAGE_FINAL);
        }

        _mutex.unlock();
    }

    void sendToAll(char* data, int len = 0) {
    // Sends data to all nodes

        send(BROADCAST_ID, data, len);
    }

    static void sendTimestamp(int id) {
        char ts[8];
        unsigned long long current = _timer->currentTime();
        for (int i = 0; i < 8; i++) {
            ts[i] = (current >> (i * 8) & 0xFF);
        }

        // Sending
    	_transparent.put(id & 0xFF);
    	_transparent.put((id >> 8) & 0xFF);
    	_transparent.put(LORA_SEND_TIMESTAMP);

    	for (int i = 0 ; i < 8 ; i++) {
    		_transparent.put(ts[i]);
    	}

        for (int i = 0; i < LORA_FINAL_COUNT; i++) {
            _transparent.put(LORA_MESSAGE_FINAL);
        }
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

    GW_Timer* timer() { return _timer; }

private:

    static void uartHandler(const unsigned int &) {
    // Gets data from UART and passes it for _handler to deal with

        // db<Lora_Mesh>(WRN) << "[UART Handler] ";

        _mutex.lock();
        _interrupt.int_disable();

        char str[LORA_MAX_LENGTH];
        str[0] = '\0';

        unsigned int len = 0;
        char buf = '0';
        int id[] = {10, 10};
        int count = 0;
        int i = 0;

        // Getting data
        while (!_transparent.ready_to_get());
        id[1] = _transparent.get();
        id[0] = _transparent.get();

        while (!_transparent.ready_to_get());
        while(_transparent.ready_to_get() && len <= LORA_MAX_LENGTH && count < LORA_FINAL_COUNT) {
            buf = _transparent.get();
            str[len++] = buf;

            // Verify end of message
            if (buf == LORA_MESSAGE_FINAL) {
                count++;
            } else {
                count = 0;
            }

            i = 0;
            while (count < LORA_FINAL_COUNT && !_transparent.ready_to_get() && i++ < LORA_TIMEOUT);
            if (i >= LORA_TIMEOUT)
                db<Lora_Mesh> (ERR) << "-----> timeout achieved for next message\n";
        }
        len -= LORA_FINAL_COUNT;

        str[len] = '\0';

        // Looking for commands
        if (str[1] == '\0') {
            switch(str[0]) {
                case LORA_SEND_TIMESTAMP:
                    sendTimestamp((id[0] << 8) + id[1]);
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

private:
    static void (*_handler)(int, char *);
    static GW_Timer* _timer;
};

class EndDevice_Lora_Mesh : public Lora_Mesh {

public:
    EndDevice_Lora_Mesh(int id, int x = 0, int y = 0, int z = 0, void (*hand)(char *) = &printMessage) {

        // hand: handler for messages from gateway. Receives message as parameter

        cout << "Configuring Lora...\n";
        _id = id;
        _x = x;
        _y = y;
        _z = z;

        // Initializing interrupts
        _interrupt.handler(uartHandler, GPIO::FALLING);
        receiver(hand);

        // Initializing timer
        _timer = new ED_Timer();
        cout << "Awaiting for timestamp from GW...\n";
        while (_timer->epoch() == 0) {
            send(LORA_SEND_TIMESTAMP);
            Alarm::delay(6000000);
        }

        cout << "Timestamp received: " << _timer->epoch() << endl;

        // send(LORA_GET_NODES); // Tells gateway to list this node in net
        cout << "Lora configured\n";
    }

    ~EndDevice_Lora_Mesh() {
        db<Lora_Mesh>(WRN) << "~EndDevice() destructor called\n";
        stopReceiver();
        delete _timer;
    }

    void send(char data[], int len = 0) {
    // Sends data to gateway

    	db<Lora_Mesh> (INF) << "EndDevice_Lora_Mesh::send() called\n";

        if (len <= 0)
            len = strlen(data);

        // if(!_isOn) turnOn();
        _mutex.lock();

    	for (int i = 0 ; i < len ; i++) {
    		_transparent.put(data[i]);
    	}

        for (int i = 0; i < LORA_FINAL_COUNT; i++) {
            _transparent.put(LORA_MESSAGE_FINAL);
        }

        _mutex.unlock();
    }

    static void send(char c) {
    // Sends data to gateway

    	db<Lora_Mesh> (INF) << "EndDevice_Lora_Mesh::send(char) called\n";

        // if(!_isOn) turnOn();
        _mutex.lock();

		_transparent.put(c);
        for (int i = 0; i < LORA_FINAL_COUNT; i++) {
            _transparent.put(LORA_MESSAGE_FINAL);
        }

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

    ED_Timer* timer() { return _timer; }

    int x() { return _x; }
    int y() { return _y; }
    int z() { return _z; }

private:
    static void uartHandler(const unsigned int &) {
    // Gets data from UART and passes it for _handler to deal with

        // db<Lora_Mesh>(INF) << "[UART Handler] ";

        _mutex.lock();
        _interrupt.int_disable();

        char str[LORA_MAX_LENGTH];
        str[0] = '\0';

        unsigned int len = 0;
        char buf = '0';
        int count = 0;
        int i = 0;

        while (!_transparent.ready_to_get());
        while(_transparent.ready_to_get() && len <= LORA_MAX_LENGTH) {
            buf = _transparent.get();
            str[len++] = buf;

            // Verify end of message
            if (buf == LORA_MESSAGE_FINAL) {
                count++;
            } else {
                count = 0;
            }

            i = 0;
            while (count < LORA_FINAL_COUNT && !_transparent.ready_to_get() && i++ < LORA_TIMEOUT);
            if (i >= LORA_TIMEOUT)
                db<Lora_Mesh> (ERR) << "-----> TIMEOUT achieved for next message\n";
        }

        len -= LORA_FINAL_COUNT;

        str[len] = '\0';

        /*if (str[0] >= 0 && str[0] <= 9) {
            db<Lora_Mesh> (WRN) << "Received message intended to node " << (int)((str[0] << 8) + str[1]) << ":\n";
            strncpy(str, str + 2, strlen(str) - 2);
        }*/

        if (str[0] == LORA_SEND_TIMESTAMP && len < (sizeof(unsigned long long) + 2)) {
            unsigned long long ts = 0;
            for (int j = 0; j < 8; j++) {
                ts |= str[j+1] << (8 * j);
            }
            _timer->epoch(ts);
            _timer->reset();
        } else
            _handler(str);

        _transparent.clear_int();
        _interrupt.int_enable();
        _mutex.unlock();
    }

private:
    static void (*_handler)(char *);
    static ED_Timer* _timer;

    int _x, _y, _z;
};

__END_SYS

#endif
