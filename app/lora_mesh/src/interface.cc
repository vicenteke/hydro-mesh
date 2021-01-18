#include "../include/interface.h"

bool Interface::_production = false;

void Interface::show_life(){
    if(!_production){
        blink(30,100000);
    }else{
        blink(3,300000);
    }
}

void Interface::blink_success(SUCCESS type){
    if(!_production){
        switch(type){
            case SIMCARDINITIALIZED:
                blink(5,200000);
                print_message(MESSAGE::GPRSINITIALIZED,0);
                led->set();
                break;
            case MESSAGESENT:
                blink(2,200000);
                print_message(MESSAGE::SENDDATARESULT,1);
                break;
        }
    }
}

void Interface::blink_error(ERROR type){
    switch(type){
        case NOGPRSCARD:
            blink(2,100000);
            print_message(MESSAGE::GPRSFAIL,0);
            break;
        case NONETWORK:
            blink(4,100000);
            print_message(MESSAGE::NETWORKFAIL,0);
            break;
        case TRYINGSENDAGAIN:
            blink(8,200000);
            print_message(MESSAGE::GPRSSENDERROR,0);
            break;
    }

}

void Interface::blink(unsigned int x, unsigned int period){
    for (auto i = 0u; i < x; ++i) {
        led->set();
        Alarm::delay(period);
        led->clear();
        Alarm::delay(period);
    }
}

void Interface::print_message(MESSAGE m, int data){
    if(!_production){
        EPOS::OStream a;
        switch(m){
            case GPRSCREATED:
                a << "GPRS created. Actual status: "<<data<<"\n";
                break;
            case GPRSSTATUS:
                a << "GPRS status: "<<data<<"\n";
                break;
            case GPRSSETUP:
                a << "Setting up GPRS...\n";
                break;
            case GPRSFAIL:
                a << "Fail to load GPRS module (check the hardware and connections).\n";
                break;
            case NETWORKFAIL:
                a << "Fail to find the network (check if sim card is active).\n";
                break;
            case STARTINGNETWORK:
                a << "Setting up network...\n";
                break;
            case SENDCOMMAND:
                a << "\n["<<data<<" trial] Sending command through GPRS.\n";
                break;
            case WAITINGRESPONSE:
                a << "[" << data << " trial] Waiting response for sent command through GPRS.\n";
                break;
            case SENDDATARESULT:
                a << "Data Send Result: " << data << "\n";
                break;
            case GPRSINITIALIZED:
                a << "GPRS Module started...";
                break;
            case GPRSSENDERROR:
                a << "Error to send data, trying again...\n";
                break;
            case CHECKINGFLASH:
                a << "Checking for stored messages on the flash...\n";
                break;
            case GETTINGFLASHDATA:
                a << "Getting data stored on the flash...\n";
                break;
            case MESSAGESNOTSENT:
                a << data << " messages were not sent. Rebooting the system...\n";
                break;
            case ALLMESSAGESSENT:
                a << "All messages were sent. Sleep for 5 minutes...\n";
                break;
        }
    }
}

void Interface::print_sending_data(unsigned int data[]){
    if(!_production){
        EPOS::OStream a;
        a << "Sending [0]=" << data[0] << " [1]=" << data[1] << " [2]=" << data[2] << "\n";
    }
}

void Interface::print_sent_message(const char msg[]){
    if(!_production){
        EPOS::OStream a;
        a << "Message: " << msg << "\n\n";
    }
}
