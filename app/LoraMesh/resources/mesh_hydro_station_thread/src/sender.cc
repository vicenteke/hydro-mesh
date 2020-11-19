#include "../include/sender.h"
#include "../include/defines.h"

bool Sender::_initialized = false;
// EPOS::eMote3_GPRS * Sender::_gprs = 0;
int Sender::_signal_str = 0;

Sender::Sender(Interface *x, MessagesHandler *m) : _interface(x), _msg(m) //---
{

    // _pwrkey = new EPOS::GPIO{'C', 4, EPOS::GPIO::OUTPUT};
    // _status = new EPOS::GPIO{'C', 1, EPOS::GPIO::INPUT};
    // _uart = new EPOS::UART{9600, 8, 0, 1, 1};
    //
    // _gprs = new EPOS::eMote3_GPRS{*_pwrkey, *_status, *_uart};
    // _interface->print_message(Interface::MESSAGE::GPRSCREATED, _status->get());

    _lora = EndDevice_Lora_Mesh(_id);

    _interface->print_message(Interface::MESSAGE::GPRSCREATED, true);

    kout << "Unsent msgs: " << unsent_messages() << "\n";
}


int Sender::send_data(void * msg, int size)//---
{
    // auto sent = _gprs->send_http_post(DATA_SERVER, (const char*) msg, (unsigned int) size);
    //
    // if(!sent) {
    //
    //     for(unsigned int i = 0; i < 2; i++) {
    //         _interface->blink_error(Interface::ERROR::TRYINGSENDAGAIN);
    //
    //         _gprs->off();
    //         eMote3_GPTM::delay(200000);
    //         _gprs->on();
    //         _gprs->use_dns(); // This parameter is (or should be) reset when the module resets.
    //
    //         eMote3_GPTM::delay(EMOTEGPTMLONGDELAY); // Hope for god network is up.
    //
    //         sent = _gprs->send_http_post(DATA_SERVER, (const char *) msg, (unsigned int) size);
    //         if(sent){
    //             _interface->blink_success(Interface::SUCCESS::MESSAGESENT);
    //             break;
    //         }
    //     }
    //
    // }

    // return sent;

    _lora.send((const char*) msg);
    // May require: ack messages and 3 attempts as before
    return true;
}

void Sender::send_or_store() //---!
{
    EPOS::OStream x;
    x << "Sending data....\n";

    long unsigned int timestamp = 0;
    // timestamp = getCurrentTime(); ---------------- Must be implemented

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
    _msg->setTime(timestamp);

	/*
	BIG WARNING HERE: flash only writes a word (4 bytes) and DBEntry has 10 bytes. We write to the flash always 12 bytes, but we use only 10 bytes in fact
	*/
    char buf[sizeof(DBEntry)+FLASH_PADDING];

    _msg->build(buf);
    _msg->dump();

	/*unsigned int t = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	unsigned short l = buf[4] | (buf[5] << 8);
	unsigned short tur = buf[6] | (buf[7] << 8);
	unsigned char p = (unsigned char) buf[8];
	unsigned char s = (unsigned char) buf[9];
	buf[10] = 0;
	buf[11] = 0;
	x << "Writing to flash timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;*/

    if(!_fifo.push(buf)) {
        x << "STORAGE FULL! Message discarded.\n";
    }

}

void Sender::try_sending_queue() //---
{
	EPOS::OStream cout;
	cout << "Sender::try_sending_queue()\n";
    if( unsent_messages() >= SENDING_BATCH_SIZE ) {
	    int idOffset = strlen(HYDRO_STATION_ID) + 1;
	    int bufSize = SENDING_BATCH_SIZE_MAX*(sizeof(DBEntry) + FLASH_PADDING) + idOffset; //plus 2 bytes due to the flash
	    char buf[bufSize];

        int toSend = (unsent_messages() > SENDING_BATCH_SIZE_MAX)? SENDING_BATCH_SIZE_MAX : unsent_messages();
        // strcpy(buf, _lora.id());
        // strcpy(buf, HYDRO_STATION_ID);

        // for(int i = 0; i < toSend; i++) {
		// 	unsigned int t;
		// 	unsigned short l, tur;
		// 	unsigned char p, s;
		// 	if(i == 0) {
		// 		_fifo.peek(buf + idOffset + i*(sizeof(DBEntry)+FLASH_PADDING), i); //plus 2 bytes due to the flash
		// 		char name[5];
		// 		strncpy(name, buf, 4);
		// 		name[4] = '\0';
		// 		t = buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24);
		// 		l = buf[9] | (buf[10] << 8);
		// 		tur = buf[11] | (buf[12] << 8);
		// 		p = (unsigned char) buf[13];
		// 		s = (unsigned char) buf[14];
		// 		cout << "Station name = " << name << " timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;
		// 	} else {
		// 		_fifo.peek((buf - (FLASH_PADDING * i)) + idOffset + i*(sizeof(DBEntry)+FLASH_PADDING), i); //plus 2 bytes due to the flash
		// 		int tmp = sizeof(DBEntry) * i + idOffset;
		// 		t = buf[tmp] | (buf[tmp+1] << 8) | (buf[tmp+2] << 16) | (buf[tmp+3] << 24);
		// 		l = buf[tmp+4] | (buf[tmp+5] << 8);
		// 		tur = buf[tmp+6] | (buf[tmp+7] << 8);
		// 		p = (unsigned char) buf[tmp+8];
		// 		s = (unsigned char) buf[tmp+9];
		// 		cout << "timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;
		// 	}
			/*cout << "\n%%%%%%%%%% Printing message before sending:\n";
			char name[5];
			strncpy(name, buf, 4);
			unsigned int t = buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24);
			unsigned short l = buf[9] | (buf[10] << 8);
			unsigned short tur = buf[11] | (buf[12] << 8);
			unsigned char p = (unsigned char) buf[13];
			unsigned char s = (unsigned char) buf[14];
			cout << "Station name = " << name << " timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;*/
		// }

        // bool sent = send_data(buf, (toSend * (sizeof(DBEntry) + FLASH_PADDING) + idOffset) - (toSend * FLASH_PADDING));  //bufSize-(FLASH_PADDING*toSend)); //less 2 bytes due to flash

        // if(sent) {
        //     cout << "SENT from fifo\n";
        //     for(int i = 0; i < toSend; i++)
        //         _fifo.pop();
        // } else {
        //     cout << "SEND FAILED from fifo\n";
        //     return;
        // }

        _fifo.peek(buf + idOffset + i*(sizeof(DBEntry)+FLASH_PADDING), i); //plus 2 bytes due to the flash

        unsigned int t;
        unsigned short l, tur;
        unsigned char p, s;
        // char name[5];
        //
        // strncpy(name, buf, 4);
        // name[4] = '\0';

        // t = buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24);
        // l = buf[9] | (buf[10] << 8);
        // tur = buf[11] | (buf[12] << 8);
        // p = (unsigned char) buf[13];
        // s = (unsigned char) buf[14];
        // cout << "Station ID = " << _lora.id() << " timestamp = " << t << " level = " << l << " tur = " << tur << " plu = " << p << " signal = " << s << endl;

        char message[10];
        for (int j = 0; j < 10; j++) {
            message[j] = buf[j + 5];
        }
        _lora.send(message);

        for(int i = 1; i < toSend; i++) {
            _fifo.peek((buf - (FLASH_PADDING * i)) + idOffset + i*(sizeof(DBEntry)+FLASH_PADDING), i); //plus 2 bytes due to the flash
            int tmp = sizeof(DBEntry) * i + idOffset;
            // t = buf[tmp] | (buf[tmp+1] << 8) | (buf[tmp+2] << 16) | (buf[tmp+3] << 24);
            // l = buf[tmp+4] | (buf[tmp+5] << 8);
            // tur = buf[tmp+6] | (buf[tmp+7] << 8);
            // p = (unsigned char) buf[tmp+8];
            // s = (unsigned char) buf[tmp+9];

            for (int j = 0; j < 10; j++) {
                message[j] = buf[j + tmp];
            }

            _lora.send(message);
		}
    }
}

bool Sender::init() //---?
{

    // _gprs->power_on();
    // _gprs->set_baudrate();
    //
    // eMote3_GPTM::delay(EMOTEGPTMSHORTDELAY / 2);
    // if(init_network()) {
    //     if(init_config()) {
    //
    //         _gprs->use_dns();
    //         enableTimeSincronization();
    //
    //         _initialized = true;
    //
    //     } else{
    //         return false;
    //     }
    // } else{
    //     return false;
    // }
    return false;
}

void Sender::query_signal_strength() //---!
{
    // if(_initialized)
    //     _signal_str = _gprs->get_signal_quality();
    return; //---
}

bool Sender::init_config() //---?
{

    kout << "[Sender::init_config]\n";

    bool res;
    res = false;
    _interface->print_message(Interface::MESSAGE::GPRSSETUP,0);

    //---
    // Alarm::delay(EMOTEGPTMSHORTDELAY);
    return true;
    //---

//     eMote3_GPTM::delay(EMOTEGPTMSHORTDELAY);
//
// #if GPRS_USE_AUTH
//
//     while(!res) {
//         _gprs->send_command("AT+QIACT");
//         res = _gprs->await_response("OK", RESPONSETIMEOUT);
//
//         if(_status->get()==0)
//             _gprs->on();
//
//         eMote3_GPTM::delay(EMOTEGPTMSHORTDELAY);
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
//         eMote3_GPTM::delay(EMOTEGPTMSHORTDELAY);
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
//     if(res)
//         kout << "success\n";
//     else
//         kout << "failure\n";
//
//     return res;
}

bool Sender::init_network(){ //---?

    return true; //---

//     kout << "[Sender::init_network]\n";
//
//     bool res = false;//_gprs->sim_card_ready();
//     unsigned int tmp = 1;
//
//     eMote3_GPTM::delay(EMOTEGPTMSHORTDELAY);
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
//         eMote3_GPTM::delay(EMOTEGPTMSHORTDELAY);
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
}

void Sender::enableTimeSincronization() //---!
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

    return; //---

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

long unsigned int Sender::getCurrentTime() //---!
{
    // Example str, includes quotes
    // "16/11/18,19:45:35-08"

    // DateTime dt;
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

    return 0; // we suck

}
