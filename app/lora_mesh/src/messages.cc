#include "../include/messages.h"

MessagesHandler::MessagesHandler()
{
    //OStream cout;

    memset(&m_entry, 0, sizeof(DBEntry));

}

int MessagesHandler::build(void * dest)
{
    m_semaphore.p();
    memcpy(dest, &m_entry, sizeof(DBEntry));
    m_semaphore.v();

    return sizeof(DBEntry);
}

int MessagesHandler::toString(char dest[])
{
    m_semaphore.p();
    uint32_t timestamp = m_entry.timestamp;
    uint16_t lvl = m_entry.lvl;
    uint16_t tur = m_entry.tur;
    uint8_t plu = m_entry.plu;
    uint8_t usr = m_entry.usr;
    m_semaphore.v();

    // char buf[10];

    for (int i = 0; i < 4; i++) {
        dest[i] = (timestamp >> (i * 8)) & 0xFF;
    }

    dest[4] = lvl & 0xFF;
    dest[5] = (lvl >> 8) & 0xFF;
    dest[6] = tur & 0xFF;
    dest[7] = (tur >> 8) & 0xFF;
    dest[8] = plu;
    dest[9] = usr;

    // strcpy(dest, buf);

    return 10;
}


void MessagesHandler::dump()
{
    OStream cout;

    cout << "\nMESSAGE"<< endl;
    cout << "lvl " << (m_entry.lvl) << endl;
    cout << "tur " << (m_entry.tur) << endl;
    cout << "plu " << (m_entry.plu) << endl;
	cout << "usr " << (m_entry.usr) << endl;
    cout << "timestamp " << (m_entry.timestamp) << endl;

    cout << endl;
}
