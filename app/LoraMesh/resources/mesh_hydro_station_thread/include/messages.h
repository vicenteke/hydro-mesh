/*
*   THIS CLASS CONTAINS METHODS AND ATTRIBUTES TO BUILD THE MESSAGE TO BE SENT OVER THE NETWORK
*   
*
* --> it stores an array with the data to be sent
* --> it builds the message based on the stored data

----------------METHODS
*
* --> build(...): receives a string and build the message to be sent on it
* --> getLength(): returns the length of the stored data array
* --> *getData(): returns the stored data
*/

#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <utility/ostream.h>
#include <machine.h>
#include <machine/cortex_m/emote3_gptm.h>
#include <semaphore.h>

#include "defines.h"

using namespace EPOS;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

struct DBEntry
{
    uint32_t timestamp;
    uint16_t lvl;
    uint16_t tur;
    uint8_t plu;
    uint8_t usr;
    
} __attribute__((packed));

class MessagesHandler
{
public:
    
    MessagesHandler();

    void setLvl(uint16_t lvl) { m_entry.lvl = lvl; }
    void setTur(uint16_t tur) { m_entry.tur = tur; }
    void setPlu(uint8_t plu) { m_entry.plu = plu; }
    void setUsr(uint8_t usr) { m_entry.usr = usr; }
    
    /**
     * builds message at dest for field fieldIdx
     * @returns the number of bytes written
     */
    int build(void * dest);    

    void setTime(long unsigned int time) { m_entry.timestamp = time; }
    void dump();
    
private:
    DBEntry m_entry;
    Semaphore m_semaphore;
};

#endif
