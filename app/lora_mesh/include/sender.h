/*
*   THIS CLASS CONTAINS METHODS TO SEND THE DATA OVER THE NETWORK OR STORE IT IN THE FLASH IN CASE IT'S NEEDED
*
* --> it uses the board port C4 as the power key to the GPRS module
* --> it uses the board port C1 as to get the status of the GPRS module
* --> it uses the UART 1 with a baud rate of 9600 to communicate with the GPRS module
*
----------------METHODS
*
* --> __init(): called to initiate the GPRS module and connect it to the network
* --> verifyFlashAndSend(): is checks if there is any data stored in the flash memory, if there is tries to send it
* --> sendData(...): receives the message to be sent and tries to send it 3 times
* --> trySendingAndStore(): tries to send the message using [sendData(...)], if trial is not successfull store it on the flash
* --> __initNetwork(): send commands to the GPRS module to initialize
* --> __initConfig(): send commands to the GPRS module to initialize
* --> verifyAndSetCurrentFlashAddress(): check what's the current flash address to be read or written
*/

#ifndef SENDER_H_
#define SENDER_H_

// #include "flash.h"
#include "interface.h"
#include "messages.h"
#include "defines.h"

#include <alarm.h>
#include <machine.h>
#include <gpio.h>
#include <uart.h>
// #include <machine/cortex_m/emote3_gprs.h>
// #include <machine/cortex_m/emote3_gptm.h>

#include "flashfifo.h"
#include "smart.h"
// #include "loraMesh.hpp"

using namespace EPOS;

/*
 * @brief Responsible for sending and storing data using LoRaMESH
 */
class Sender{
    static const auto DATA_SERVER = HYDRO_DATA_SERVER;
    static const auto EMOTEGPTMSHORTDELAY = 2000000u;
    static const auto EMOTEGPTMLONGDELAY = 5000000u;
    static const auto RESPONSETIMEOUT = 3000000u;

	static const int FLASH_PADDING = 2;

public:
    Sender(Interface *x, MessagesHandler *m);

    bool init();
    void try_sending_queue();
    void send_or_store();
    int unsent_messages(){ return _fifo.size(); }
    void query_signal_strength();
    static int signal_strength(){ return _signal_str; }

    Serial_Link* serial() { return _serial; }
    unsigned long long getTimestamp() { return _serial->getTimestamp(); }

private:
    bool init_network();
    bool init_config();

    /*
     * @brief Sends a message received by try_sending_queue. Creates the series necessary
     * @return False in case of error, true otherwise
     */
    int send_data(char * msg, int size);

    /**
     * @brief Enables time sincronization for the GPRS module.
     */
    void enableTimeSincronization();

    /**
     * @brief Queries the GPRS module for current time
     * @returns The number of milliseconds elapsed since the unix epoch
     */
    unsigned long getCurrentTime();

private:
    static Flash_FIFO<sizeof(DBEntry)+FLASH_PADDING> _fifo; // FLASH_PADDING (2) due to the fact that flash only writes a word (4 bytes) and DBEntry has size of 10 bytes

    Interface *_interface;
    MessagesHandler *_msg;
    static Serial_Link *_serial;

    static bool _initialized;
    static int _signal_str;
};

#endif
