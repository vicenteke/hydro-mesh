#include "../include/sender.h"
#include "../include/defines.h"

bool Sender::_initialized = false;
// EPOS::eMote3_GPRS * Sender::_gprs = 0;
int Sender::_signal_str = 0;
Flash_FIFO<sizeof(DBEntry)+Sender::FLASH_PADDING> Sender::_fifo;
Serial_Link * Sender::_serial;

Sender::Sender(Interface *x, MessagesHandler *m) : _interface(x), _msg(m)
{

    // _pwrkey = new EPOS::GPIO{'C', 4, EPOS::GPIO::OUTPUT};
    // _status = new EPOS::GPIO{'C', 1, EPOS::GPIO::INPUT};
    // _uart = new EPOS::UART{9600, 8, 0, 1, 1};

    // _gprs = new EPOS::eMote3_GPRS{*_pwrkey, *_status, *_uart};
    // _interface->print_message(Interface::MESSAGE::GPRSCREATED, _status->get());
    if (!_initialized) {
        _serial = new Serial_Link();
        kout << "Messages already in flash: " << unsent_messages() << "\n";
        while(unsent_messages() > 0) _fifo.pop();
    }

    _initialized = true;
}


int Sender::send_data(char * msg, int size)
{
    // auto sent = _gprs->send_http_post(DATA_SERVER, (const char*) msg, (unsigned int) size);
    // Change for anything that sends through HTTP

    /*if(!sent) {
        // Try two more times to send
        for(unsigned int i = 0; i < 2; i++) {
            _interface->blink_error(Interface::ERROR::TRYINGSENDAGAIN);

            // _gprs->off();
            Alarm::delay(200000);
            // _gprs->on();
            // _gprs->use_dns(); // This parameter is (or should be) reset when the module resets.

            Alarm::delay(EMOTEGPTMLONGDELAY); // Hope for god network is up.

            // sent = _gprs->send_http_post(DATA_SERVER, (const char *) msg, (unsigned int) size);
            if(sent){
                _interface->blink_success(Interface::SUCCESS::MESSAGESENT);
                break;
            }
        }

    }
    return sent;*/
    //---

    // USB io;
    // char* message = (char*) msg;
    // bool was_locked = CPU::int_disabled();
    // if(!was_locked)
    //     CPU::int_disable();
    //
    // io.put('@');
    // for (int i = 0; i < size; i++)
    //     io.put(message[i]);
    //
    // io.put('@');
    // if(!was_locked)
    //     CPU::int_enable();

    // kout << "[send_data] ";
    int nameOffset = 5; // name[5]
    int offset = 0;

    DB_Record record;

    for (int i = 0; i < size; ++i)
    {
        offset = (i * sizeof(DBEntry)) + nameOffset;

        record.version  = STATIC_VERSION;
        record.x        = msg[offset+10] | (msg[offset+11] << 8) | (msg[offset+12] << 16) | (msg[offset+13] << 24);
        record.y        = msg[offset+14] | (msg[offset+15] << 8) | (msg[offset+16] << 16) | (msg[offset+17] << 24);
        record.z        = msg[offset+18] | (msg[offset+19] << 8) | (msg[offset+20] << 16) | (msg[offset+21] << 24);
        record.t        = msg[offset] | (msg[offset+1] << 8) | (msg[offset+2] << 16) | (msg[offset+3] << 24);
        record.dev      = msg[offset + 9];

        // Sending pluviometer
        record.unit         = TSTP::Unit::Length;
        record.value        = msg[offset+6] | (msg[offset+7] << 8);
        record.error        = HYDRO_ERROR_PLU; // Set in lora_mesh/include/defines.h
        record.confidence   = HYDRO_CONF_PLU;

        // If can't send record, then series has to be created
        if (_serial->sendRecord(record) != 'S') {

            // Checks if series for that device weren't created and creates them
            if (!(_serial->add((int)msg[offset + 9]))) {
                kout << "\n    it seems that the series for this device has already been created.\n"
                     << "    creating new ones, this should merge previous series or each data is split in two\n\n";
            }
            DB_Series series_data;
            series_data.version = STATIC_VERSION;
            series_data.x = record.x;
            series_data.y = record.y;
            series_data.z = record.z;
            series_data.r = HYDRO_RADIUS; // Set in lora_mesh/include/defines.h
            series_data.t0 = record.t - 1000;
            series_data.t1 = -1;
            series_data.dev = record.dev;

            // Pluviometer
            series_data.unit = TSTP::Unit::Length;
            if(_serial->createSeries(series_data) == 'F') {
                kout << "!!!! > unable to create series. Operation aborted and data discarded\n";
                _serial->remove((int)msg[offset + 9]);
                return false;
            }

            // Turbidity
            series_data.unit = TSTP::Unit::Amount_of_Substance;
            _serial->createSeries(series_data);

            // Water Level
            series_data.unit = TSTP::Unit::Length;
            series_data.dev += 2047; // Used to differ pluvio and water series. 2047 = max id for LoRaMESH
            _serial->createSeries(series_data);

            if (_serial->sendRecord(record) != 'S') {
                kout << "!!!! > failed to send record. Operation aborted and data discarded\n";
                _serial->remove((int)msg[offset + 9]);
                return false;
            } else {
                kout << "    pluviometer sent...\n";
            }
        } else {
            kout << "    pluviometer sent...\n";
        }
        
        // Sending turbidity
        record.unit         = TSTP::Unit::Amount_of_Substance;
        record.value        = msg[offset] | (msg[offset+1] << 8) | (msg[offset+2] << 16) | (msg[offset+3] << 24);
        record.error        = HYDRO_ERROR_TURB; // Set in lora_mesh/include/defines.h
        record.confidence   = HYDRO_CONF_TURB;

        if (_serial->sendRecord(record) != 'S') {
            kout << "!!!! > failed to send turbidity\n";
            return false;
        } else {
            kout << "    turbidity sent...\n";
        }

        // Sending water level
        record.unit         = TSTP::Unit::Length;
        record.value        = msg[offset+4] | (msg[offset+5] << 8);
        record.error        = HYDRO_ERROR_LEVEL; // Set in lora_mesh/include/defines.h
        record.confidence   = HYDRO_CONF_LEVEL;
        record.dev          += 2047; // Used to differ pluvio and water series. 2047 = max id for LoRaMESH

        if (_serial->sendRecord(record) != 'S') {
            kout << "!!!! > failed to send water level\n";
            return false;
        } else {
            kout << "    water level sent...\n";
        }

        _serial->add(record.dev - 2047);
    }

    return true;
}

void Sender::send_or_store()
{
    EPOS::OStream x;
    // x << "[send_or_store] ";

    // long unsigned int timestamp = 0;
    // timestamp = getCurrentTime();

    /*
    // block until time is valid (2016 onwards)
    while( (timestamp < 1451606400) )
    {
        if(timestamp != 0 ) // we got a wrong value
            eMote3_GPTM::delay(5000000ul);

        timestamp = getCurrentTime();
    }
    */
    //x << "Setting message timestamp = " << timestamp << endl;
    // _msg->setTime(timestamp);

	/*
	BIG WARNING HERE: flash only writes a word (4 bytes) and DBEntry has 10 bytes. We write to the flash always 12 bytes, but we use only 10 bytes in fact
	*/
    char buf[sizeof(DBEntry)+FLASH_PADDING];

    _msg->build(buf);
    // _msg->dump();

	/*unsigned int t = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	unsigned short l = buf[4] | (buf[5] << 8);
	unsigned short tur = buf[6] | (buf[7] << 8);
	unsigned char p = (unsigned char) buf[8];
	unsigned char s = (unsigned char) buf[9];
	buf[10] = 0;
	buf[11] = 0;
	x << "Writing to flash timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;*/

    if(!_fifo.push(buf)) {
        x << "storage full! Message discarded.\n";
    }

}

void Sender::try_sending_queue()
{
	EPOS::OStream cout;
	// cout << "[try_sending_queue] ";
    if( unsent_messages() >= SENDING_BATCH_SIZE ) {
	    int idOffset = strlen(HYDRO_STATION_ID) + 1;
	    int bufSize = SENDING_BATCH_SIZE_MAX*(sizeof(DBEntry) + FLASH_PADDING) + idOffset; //plus 2 bytes due to the flash
	    char buf[bufSize];

        int toSend = (unsent_messages() > SENDING_BATCH_SIZE_MAX)? SENDING_BATCH_SIZE_MAX : unsent_messages();
        strcpy(buf, HYDRO_STATION_ID);

        for(int i = 0; i < toSend; i++) {
			unsigned int t;
			unsigned short l, tur;
			unsigned char p, s;
            int x, y, z;
			if(i == 0) {
				_fifo.peek(buf + idOffset + i*(sizeof(DBEntry)+FLASH_PADDING), i); //plus 2 bytes due to the flash
				char name[5];
				strncpy(name, buf, 4);
				name[4] = '\0';
				t = buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24);
				l = buf[9] | (buf[10] << 8);
				tur = buf[11] | (buf[12] << 8);
				p = (unsigned char) buf[13];
				s = (unsigned char) buf[14];
                x = buf[15] | (buf[16] << 8) | (buf[17] << 16) | (buf[18] << 24);
                y = buf[19] | (buf[20] << 8) | (buf[21] << 16) | (buf[22] << 24);
                z = buf[23] | (buf[24] << 8) | (buf[25] << 16) | (buf[26] << 24);
				cout << "name = " << name << " timestamp = " << t << " level = " << l
                    << " tur = " << tur << " plu = " << p << " device = " << s;
                cout << " coord = " << x << ", " << y << ", " << z << endl;
			} else {
				_fifo.peek((buf - (FLASH_PADDING * i)) + idOffset + i*(sizeof(DBEntry)+FLASH_PADDING), i); //plus 2 bytes due to the flash
				int tmp = sizeof(DBEntry) * i + idOffset;
				t = buf[tmp] | (buf[tmp+1] << 8) | (buf[tmp+2] << 16) | (buf[tmp+3] << 24);
				l = buf[tmp+4] | (buf[tmp+5] << 8);
				tur = buf[tmp+6] | (buf[tmp+7] << 8);
				p = (unsigned char) buf[tmp+8];
				s = (unsigned char) buf[tmp+9];
                x = buf[tmp+10] | (buf[tmp+11] << 8) | (buf[tmp+12] << 16) | (buf[tmp+13] << 24);
                y = buf[tmp+14] | (buf[tmp+15] << 8) | (buf[tmp+16] << 16) | (buf[tmp+17] << 24);
                z = buf[tmp+18] | (buf[tmp+19] << 8) | (buf[tmp+20] << 16) | (buf[tmp+21] << 24);

				cout << "timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " device = " << s;
                cout << "coord = " << x << ", " << y << ", " << z  << endl;
			}
			/*cout << "\n%%%%%%%%%% Printing message before sending:\n";
			char name[5];
			strncpy(name, buf, 4);
			unsigned int t = buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24);
			unsigned short l = buf[9] | (buf[10] << 8);
			unsigned short tur = buf[11] | (buf[12] << 8);
			unsigned char p = (unsigned char) buf[13];
			unsigned char s = (unsigned char) buf[14];
			cout << "Station name = " << name << " timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;*/
		}

        bool sent = send_data(buf, toSend);  // In this case, size = number of messages
        // bool sent = send_data(buf, (toSend * (sizeof(DBEntry) + FLASH_PADDING) + idOffset) - (toSend * FLASH_PADDING));  //bufSize-(FLASH_PADDING*toSend)); //less 2 bytes due to flash

        if(sent) {
            cout << "sent " << toSend << " from fifo\n";
            for(int i = 0; i < toSend; i++)
                _fifo.pop();
        } else {
            cout << "sent failed from fifo\n";
            // for(int i = 0; i < toSend; i++)
            //     _fifo.pop();
        }
    }
}

bool Sender::init()
{

    // _gprs->power_on();
    // _gprs->set_baudrate();

    Alarm::delay(EMOTEGPTMSHORTDELAY / 2);
    if(init_network()) {
        if(init_config()) {

            // _gprs->use_dns();
            enableTimeSincronization();

            _initialized = true;

        } else{
            return false;
        }
    } else{
        return false;
    }
    return false;
}

void Sender::query_signal_strength()
{
    // if(_initialized)
        // _signal_str = _gprs->get_signal_quality();
}

bool Sender::init_config()
{

    // kout << "[Sender::init_config]\n";

    bool res;
    res = false;
    _interface->print_message(Interface::MESSAGE::GPRSSETUP,0);

    Alarm::delay(EMOTEGPTMSHORTDELAY);

// #if GPRS_USE_AUTH
//
//     while(!res) {
//         _gprs->send_command("AT+QIACT");
//         res = _gprs->await_response("OK", RESPONSETIMEOUT);
//
//         if(_status->get()==0)
//             _gprs->on();
//
//         Alarm::delay(EMOTEGPTMSHORTDELAY);
//     }
//
// #else
//
//     while(!res) {
//         _gprs->send_command("AT+CGATT=1");
//         res = _gprs->await_response("OK", RESPONSETIMEOUT);
//
//         _interface->print_message(Interface::MESSAGE::GPRSSTATUS, res);
//
//         if(_status->get()==0)
//             _gprs->on();
//
//         Alarm::delay(EMOTEGPTMSHORTDELAY);
//     }
//
//
//     _gprs->send_command("AT+CGACT?");
//     res = _gprs->await_response("OK", RESPONSETIMEOUT*10);
//
//
//     res = false;
//     while(!res) {
//         _gprs->send_command("AT+CGACT=1,1");
//         res = _gprs->await_response("OK", RESPONSETIMEOUT*10);
//         //_interface->print_message(Interface::MESSAGE::GPRSSTATUS, res);
//         if(_status->get()==0)
//         {
//             kout << "[Sender::init_config] module is off\n";
//             _gprs->on();
//         }
//     }
//
// #endif
//
//    if(res)
//        kout << "success\n";
//    else
//        kout << "failure\n";
//
//    return res;

    return true;
}

bool Sender::init_network(){

//     kout << "[Sender::init_network]\n";
//
//     bool res = false;//_gprs->sim_card_ready();
//     unsigned int tmp = 1;
//
//     Alarm::delay(EMOTEGPTMSHORTDELAY);
//
//     while(!res) {
// #if GPRS_USE_AUTH
//         _gprs->send_command("AT+QIREGAPP=\""GPRS_OPERATOR_APN"\",\""GPRS_AUTH_USER"\",\""GPRS_AUTH_PASSWD"\"");
//         res = _gprs->await_response("OK", EMOTEGPTMSHORTDELAY);
//
//         _gprs->send_command("AT+QIREGAPP?");
//         res = res && _gprs->await_response("OK", RESPONSETIMEOUT);
// #else
//         _gprs->send_command("AT+CGDCONT=1,\"IP\",\""GPRS_OPERATOR_APN"\",,0,0");
//         res = _gprs->await_response("OK", EMOTEGPTMSHORTDELAY);
//
//         _gprs->send_command("AT+CGDCONT?");
//         res = res && _gprs->await_response("OK", RESPONSETIMEOUT);
// #endif
//         if(!_status->get())
//             _gprs->on();
//         Alarm::delay(EMOTEGPTMSHORTDELAY);
//
//         tmp++;
//     }
//
//     if(res)
//         kout << "success\n";
//     else
//         kout << "failure\n";
//
//     return res;
    return true;
}

void Sender::enableTimeSincronization()
{
    // bool res = false;
    // while(!res) {
    //     _gprs->send_command("AT+CTZR=0");
    //     res = _gprs->await_response("OK", RESPONSETIMEOUT);
    // }
    //
    // res = false;
    // while(!res) {
    //     _gprs->send_command("AT+QNITZ=1");
    //     res = _gprs->await_response("OK", RESPONSETIMEOUT);
    // }
    //
    // res = false;
    // while(!res) {
    //     _gprs->send_command("AT+CTZU=1");
    //     res = _gprs->await_response("OK", RESPONSETIMEOUT);
    // }

    // get current time now
    /*
    char buf[128];
    memset(buf, 0, 128);

    res = _gprs->await_response("+QNITZ:", 10000000, buf);

    kout << "TIME IS " << buf;
    kout << "\nEND\n";
    */

}

#include "../include/unixtime.h"

#define DECIMAL_AT(X) ( (buf[(X)] - '0')*10 + (buf[(X)+1] - '0') )

long unsigned int Sender::getCurrentTime()
{
    // Example str, includes quotes
    // "16/11/18,19:45:35-08"

    DateTime dt;
    // char buf[64];
    // memset(buf, 0, 64);
    //
    // if(_gprs->queryTime( buf )) {
    //     dt.year = 2000 + DECIMAL_AT(2);
    //     dt.month = DECIMAL_AT(5);
    //     dt.date = DECIMAL_AT(8);
    //
    //     dt.hr = DECIMAL_AT(11);
    //     dt.min = DECIMAL_AT(14);
    //     dt.sec = DECIMAL_AT(17);
    //
    //     kout << "Decoded time is: " << dt.year << "-" << dt.month << "-" << dt.date << " " << dt.hr << ":" << dt.min << ":" << dt.sec << "\n";
    //
	// 	return unixtime(dt);
    //     //return unixtime(dt, 0, 0);
    // } else {
    //     kout << "TIME QUERY FAILED\n";
    // }
    //
    // return 0; // we suck

    dt.year = 2021;
    dt.month = 1;
    dt.date = 1;

    dt.hr = 0;
    dt.min = 11;
    dt.sec = 22;

    return unixtime(dt);

}
