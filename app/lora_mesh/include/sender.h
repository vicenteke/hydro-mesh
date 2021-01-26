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
// #include "loraMesh.hpp"

using namespace EPOS;
//
// // Credentials
// const char DOMAIN[]   = "tutorial";
// const char USERNAME[] = "tutorial";
// const char PASSWORD[] = "tuto2018";
//
// enum {
//     MAX_NODES = 40
// };
//
// /*
//  * @brief Stores and creates series based on received "Usr"
//  */
// class Series_Logger {
// public:
//     Series_Logger() {
//         _length = 0;
//         for (int i = 0; i < MAX_NODES; i++) {
//             _log[i] = -1;
//         }
//         sendCredentials();
//         Alarm::delay(400000);
//     }
//
//     ~Series_Logger() {}
//
//     /*
//      * Checks if series 'usr' has been created; if hasn't, stores 'usr' in _log
//      *
//      * @return false if 'usr' was already in _log, true if it was stored in _log
//      */
//     int add(int usr) {
//
//         for (int i = 0; i < _length; i++) {
//             if (usr == _log[i]) return false;
//         }
//
//         _log[_length++] = usr;
//         return true;
//     }
//
//     /*
//      * Removes 'usr' from _log
//      *
//      * @return false if 'usr' wasn't in _log, true if it was removed
//      */
//     int remove(int usr) {
//         bool found = false;
//         for (int i = 0; i < _length; i++) {
//             if (found) {
//                 _log[i] = _log[i + 1];
//             } else if (usr == _log[i]) {
//                 found = true;
//                 _log[i] = _log[i + 1];
//             }
//         }
//         if (found) {
//             _log[_length--] = -1;
//         }
//
//         return found;
//     }
//
//     int length() { return _length; }
//
// private:
//     int _log[MAX_NODES];
//     unsigned short _length;
// };
//
// /*
//  * @brief Responsible for communication between eMote3 and PC (loragw)
//  */
// class Serial_Link {
// public:
//     Serial_Link(){}
//
//     ~Serial_Link(){}
//
//     /*
//      * @brief Sends credentials (the ones set in the beginning of this file) for loragw
//      * @return 0 if no credentials are available, 1 when sent
//      */
//     int sendCredentials() {
//         if (strlen(DOMAIN) < 2) return 0;
//         if (strlen(USERNAME) < 2) return 0;
//         if (strlen(PASSWORD) < 2) return 0;
//
//         char c = 0;
//         io.put('%');
//         do {
//             while (!io.ready_to_get());
//             c = io.get();
//         } while (c != '%');
//
//         for (int i = 0; i < strlen(DOMAIN); i++) {
//             io.put(DOMAIN[i]);
//         }
//         for (int i = 0; i < 3; i++) {
//             io.put('X');
//         }
//         for (int i = 0; i < strlen(USERNAME); i++) {
//             io.put(USERNAME[i]);
//         }
//         for (int i = 0; i < 3; i++) {
//             io.put('X');
//         }
//         for (int i = 0; i < strlen(PASSWORD); i++) {
//             io.put(PASSWORD[i]);
//         }
//         for (int i = 0; i < 3; i++) {
//             io.put('X');
//         }
//
//         return 1;
//     }
//
//     /*
//      * @brief Keeps looking for loragw signals
//      */
//     void poolingSerial() {
//         char c = 0;
//         while true:
//             c = io.get();
//             switch (c) {
//                 case '%': sendCredentials();
//                     break;
//                 default: break;
//             }
//     }
//
// private:
//     USB io;
//     Series_Logger series;
// }

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

private:
    bool init_network();
    bool init_config();
    int send_data(void * msg, int size);

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

    // static EPOS::eMote3_GPRS *_gprs;
    // EPOS::GPIO *_pwrkey;
    // EPOS::GPIO *_status;
    // EPOS::UART *_uart;
    Interface *_interface;
    MessagesHandler *_msg;
    // const int _id;
    // EndDevice_Lora_Mesh _lora;

    static bool _initialized;
    static int _signal_str;
};

#endif
