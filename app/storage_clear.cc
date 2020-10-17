#include <smart_data.h>
#include <persistent_storage.h>
#include <utility/ostream.h>

using namespace EPOS;

typedef Smart_Data_Common::SI_Record DB_Record;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

int main()
{
    OStream cout;

    Storage::clear(); cout << "storage cleared" << endl;

    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }

    return 0;
}
