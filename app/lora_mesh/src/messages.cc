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


void MessagesHandler::dump()
{
    OStream cout;
    
    cout << "\n\nMESSAGE"<< endl;
    cout << "lvl " << (m_entry.lvl) << endl;
    cout << "tur " << (m_entry.tur) << endl;
    cout << "plu " << (m_entry.plu) << endl;
	cout << "usr " << (m_entry.usr) << endl;
    cout << "timestamp " << m_entry.timestamp << endl;
    
    cout << endl;
}
