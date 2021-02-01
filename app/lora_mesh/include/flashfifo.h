
#ifndef FLASHFIFO_H_
#define FLASHFIFO_H_

#include <utility/string.h>
// #include <machine/cortex_m/emote3_flash.h>
#include <persistent_storage.h>
#include <mutex.h>
__BEGIN_SYS

/**
 * @brief A persistent fifo implemented as a circular buffer in flash memory
 * @param S: number of bytes of the stored element
 */

template <int S>
class Flash_FIFO{
private:
    typedef Persistent_Storage Flash;

public:

    struct Header
    {
        Header() : pos(0), s(0)
        {
            strcpy(magic, Flash_FIFO::getMagic());
        }

        char magic[16];
        unsigned int pos; // position in circular buffer
        unsigned int s; // elements in buffer
    };

    static const unsigned int FLASH_BASE = 128 * 1024; //flash start address is 128k. This address will store the current flash address that is being written
    static const unsigned int FLASH_DATA_SIZE = 128 * 1024;  //128k writable after 128k start address (256k total)
    static const unsigned int ELEMENT_SIZE = S;
    static const unsigned int HEADER_SIZE = sizeof(Header);
    static const unsigned int CAPACITY = (FLASH_DATA_SIZE - HEADER_SIZE) / ELEMENT_SIZE;

    static unsigned int get_slot_addr(int slot) { return FLASH_BASE + HEADER_SIZE + ELEMENT_SIZE*slot; }


    Flash_FIFO()
    {
        init();
    }

    void init()
    {
        // read header
        // kout << "[Flash_FIFO::init]\n";
        Flash::read(FLASH_BASE, reinterpret_cast<unsigned int*>(&m_head), HEADER_SIZE); // reads header in memory

        // try to validate whats in it
        if(strcmp(getMagic(), m_head.magic))
        {
            // kout << "[Flash_FIFO::init] magic mismatch\n";
            clear();
            return;
        }

        if(m_head.pos > CAPACITY || m_head.s > CAPACITY)
        {
            // kout << "[Flash_FIFO::init] header corrupt\n";
            clear();
            return;
        }

        // header assumed valid now, we are done
        // kout << "[Flash_FIFO::init] header valid\n";
    }

    void clear()
    {
        // kout << "[Flash_FIFO::clear]\n";
        // write zero to everything

        strcpy( m_head.magic, getMagic());
        m_head.pos = 0;
        m_head.s = 0;

        flushHeader();
    }

    /**
     * @returns true if successfull, false otherwise
     */
    bool push(void * src)
    {
        if(!src)
            return false;

        if(size() == capacity())
            return false;

		/*char *buf = (char *) src;
		unsigned int t = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
		unsigned short l = buf[4] | (buf[5] << 8);
		unsigned short tur = buf[6] | (buf[7] << 8);
		unsigned char p = (unsigned char) buf[8];
		unsigned char s = (unsigned char) buf[9];*/

        int slot = (m_head.pos + m_head.s) % CAPACITY;
		//kout << "[FLASH IS WRITING to " << get_slot_addr(slot) << " ] timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;
        _mutex.lock();
        Flash::write(get_slot_addr(slot), static_cast<unsigned int*>(src), S);

        m_head.s++;

        flushHeader();
        _mutex.unlock();
        return true;
    }

    /**
     * @returns true if successfull, false otherwise
     */
    bool peek(void * dest, unsigned int advance = 0)
    {
        if(size() <= advance)
            return false;

        int slot = (m_head.pos + advance) % CAPACITY;
		//kout << "Reading from flash address = " << get_slot_addr(slot) << endl;
        _mutex.lock();
        Flash::read(get_slot_addr(slot), static_cast<unsigned int*>(dest), S);
        _mutex.unlock();

		/*char *buf = (char *) dest;
		unsigned int t = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
		unsigned short l = buf[4] | (buf[5] << 8);
		unsigned short tur = buf[6] | (buf[7] << 8);
		unsigned char p = (unsigned char) buf[8];
		unsigned char s = (unsigned char) buf[9];

		kout << "[FLASH READ FROM " << get_slot_addr(slot) << " ] timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;
		*/
        return true;
    }

    /**
     * @returns true if successfull, false otherwise
     */
    bool pop(void * dest = 0)
    {
        if(dest)
            if(!peek(dest))
                return false;

        m_head.pos = (m_head.pos + 1) % CAPACITY;
        m_head.s --;
        _mutex.lock();
        flushHeader();
        _mutex.unlock();
        return true;
    }

    unsigned int size()
    {
        return m_head.s;
    }

    unsigned int capacity()
    {
        return CAPACITY;
    }

private:

    static const char * getMagic()
    {
        return "FLASH_FIFO_HEAD"; // len is 16, if the zero terminator is included
    }

    void flushHeader()
    {
        // kout << "[Flash_FIFO::flushHeader]\n";
        Flash::write(FLASH_BASE, reinterpret_cast<unsigned int*>(&m_head), HEADER_SIZE); // writes header to flash
    }

    Mutex _mutex;
    Header m_head;
};

__END_SYS

#endif
