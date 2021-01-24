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
// #include <machine/cortex_m/emote3_gptm.h>
#include <alarm.h>
#include <semaphore.h>
#include <utility/string.h>

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
    int x;
    int y;
    int z;

}__attribute__((packed));

class MessagesHandler
{
public:

    MessagesHandler();

    void setTime(uint32_t time) { m_entry.timestamp = time; }
    void setLvl(uint16_t lvl) { m_entry.lvl = lvl; }
    void setTur(uint16_t tur) { m_entry.tur = tur; }
    void setPlu(uint8_t plu) { m_entry.plu = plu; }
    void setUsr(uint8_t usr) { m_entry.usr = usr; }
    void setX(int x) { m_entry.x = x; }
    void setY(int y) { m_entry.y = y; }
    void setZ(int z) { m_entry.z = z; }

    /**
     * builds message at dest for field fieldIdx
     * @returns the number of bytes written
     */
    int build(void * dest);

    /**
     * builds a string as
     *      dest[3-0]   = timestamp
     *      dest[5-4]   = level
     *      dest[7-6]   = turb
     *      dest[8]     = pluv
     *      dest[9]     = usr
     *      dest[13-10] = x
     *      dest[17-14] = y
     *      dest[21-18] = z
     * @returns the length of dest
     */
    int toString(char dest[]);

    void dump();

private:
    DBEntry m_entry;
    Semaphore m_semaphore;
};

#endif
