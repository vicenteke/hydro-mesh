#ifndef HS_SENSORS_H_
#define HS_SENSORS_H_

#include <gpio.h>
#include "defines.h"
#include <adc.h>
#include <system/meta.h>
#include <machine/cortex_m/emote3_gptm.h>

using namespace EPOS;






class Sensor_Base {
public:
    Sensor_Base(GPIO & relay):
        relay(relay)
    {}

    int enable()
    {
        relay.set();
	return 0;
    }

    int disable()
    {
        relay.clear();
	return 0;
    }

protected:
    GPIO & relay;
};

class Analog_Sensor_Base: public Sensor_Base{
public:
    Analog_Sensor_Base(ADC & adc, GPIO & relay):
        Sensor_Base(relay),
        adc(adc)
        
    {}
    
    int read(){return adc.read();}
protected:
    ADC & adc;
};


class Level_Sensor: public Analog_Sensor_Base {
public:
    Level_Sensor(ADC & adc, GPIO & relay):
        Analog_Sensor_Base{adc, relay}
    {}

    int raw_sample()
    {
        return adc.read();
    }

    int sample()
    {
        // y = Ax + B
        // 0 = 1876 * A + B (1876 is zero)
        // 5000 = 8192 * A + B (8192 is ADC 13 bits = 5 meters)
        // A = 0.792
        // B = -1485
        return adc.read(); //(int) (0.792f * adc.read() - 1485);
    }
};






class Turb_Solar_Sensor: public Analog_Sensor_Base {

public://adc = tensao | adc2 = corrente
    Turb_Solar_Sensor(ADC & adc, ADC & adc2, GPIO & relay, GPIO & infrared):
        Analog_Sensor_Base{adc, relay},
        infrared(infrared)
    {}
    
    
    
    void init(){}
    
    
    int sample()
    {
        short turb_data;
        // Filter daylight as directed by sensor manufacturer
        eMote3_GPTM::delay(250000); // Wait 250 ms before reading
        auto daylight = adc.read();
		//kout << "daylight = " << daylight << "\n";
        eMote3_GPTM::delay(250000); // Wait more 250 ms because we've been told to
        infrared.set();
        eMote3_GPTM::delay(450000); // Wait 200+250 ms before reading again
        
        auto mixed = adc.read();
		
        //kout << "IR = " << mixed << "\n";
        eMote3_GPTM::delay(250000); // Wait 200+250 ms before reading again
        infrared.clear();
        //kout << "daylight= " << daylight << endl;
        //kout << "IR: = " << mixed << endl;
        //kout << "difference: = " << (mixed-daylight) << endl;
        //return mixed;
        turb_data = mixed - daylight;
        turb_data = (turb_data < 0) ? 0 : turb_data;
        return turb_data;
    }
    
    
    unsigned char signal(){return 2;}
        

private:
    GPIO & infrared;
    //unsigned char _signal;
};


class Turb_OBS_Sensor {    //public Analog_Sensor_Base {

public://adc = tensao | adc2 = corrente
    //Turbidity_Sensor(ADC & adc, ADC & adc2, GPIO & relay, GPIO & infrared):
    Turb_OBS_Sensor(ADC & adc1, ADC & adc2, GPIO & relay, GPIO & infrared):
        _analog1(adc1, relay),
        _analog2(adc2, relay),
        //adc2(adc2),
        _infrared(infrared)
    {}
    
    
    
    void init(){}
    
    void enable(){_analog1.enable();}
    
    void disable(){_analog1.disable();}
    
    int sample()
    {
        // Filter daylight as directed by sensor manufacturer
        eMote3_GPTM::delay(250000); // Wait 250 ms before reading

        eMote3_GPTM::delay(250000); // Wait more 250 ms because we've been told to
        //_infrared.set();
        eMote3_GPTM::delay(450000); // Wait 200+250 ms before reading again
        
        auto current = _analog1.read();
        auto volt = _analog2.read();
        short turb_data;
        
        kout<<"\ncorrente: " << current<<endl;
        kout<<"tensao: " << volt<<endl;
        
        if (volt < 1450){
            turb_data = current;
            _signal = 0;
        }else{
            turb_data = volt;
            _signal = 1;
        }
		turb_data = (turb_data < 0) ? 0 : turb_data;
        
      
        eMote3_GPTM::delay(250000); // Wait 200+250 ms before reading again
        //_infrared.clear();
        //eMote3_GPTM::delay(10000000);

        return turb_data;
    }
    
    
    unsigned char signal(){return _signal;}
        

private:
    Analog_Sensor_Base _analog1;
    Analog_Sensor_Base _analog2;
    //ADC & adc2;
    GPIO & _infrared;
    unsigned char _signal;

};

template <typename T>
class Base_Turbidity_Sensor : public T{
public:
    Base_Turbidity_Sensor(ADC & adc1, ADC & adc2, GPIO & relay, GPIO & infrared):T(adc1, adc2, relay, infrared){}
};


typedef IF<TURB_SENSOR_TYPE, Base_Turbidity_Sensor<Turb_OBS_Sensor>, Base_Turbidity_Sensor<Turb_Solar_Sensor> >::Result Turbidity_Sensor;






class Pluviometric_Sensor: public Sensor_Base {
public:
    Pluviometric_Sensor(GPIO & input, GPIO & relay):
        Sensor_Base(relay),
        input(input),
        _count(0)
    {
        _instance = this;

        input.input();
        input.pull_up();

        input.handler(&handler);
        input.int_enable(GPIO::FALLING, false);
    }
  
    static void handler(const unsigned int & i)
    {
        if(_instance && !_debouncing){
          _instance->_count++;
          _instance->_debouncing = true;
          for(volatile unsigned int j = 0; j < 600050; j++) ; //was 50, we added 600000, to prevent some physical errors
          //eMote3_GPTM timer(0);
          //timer.set(1200000);
          //timer.enable();
          //while(timer.running());
          _instance->_debouncing = false;
        }
    }
  
    int count()
    {
        return _count;
    }
  
    void resetCount()
    {
        _count = 0;
    }
  
    int countAndReset()
    {
        int ret = _count;
        _count -= ret;
        return ret;
    }
  
private:
    GPIO &input;
    int _count;
    static bool _debouncing;
  
    // this is needed because we treat the interrupt in a static function
    static Pluviometric_Sensor * _instance;
};

class Voltages_ADC
{
    static const unsigned char CFG_RANGE = 0b111; // positive up to 3*Vref
    static const unsigned int SPEED = 400000; // 400 kHz

public:
    Voltages_ADC():
    _spi(0, Traits<CPU>::CLOCK, SPI::FORMAT_MOTO_3, SPI::MASTER, SPEED, 8)
    {
        send_byte(0b10001000); // set mode to external clock

        send_byte(0b10001000 | CFG_RANGE); // channels 0 and 1 to differential mode
        send_byte(0b10101000 | CFG_RANGE); // channels 2 and 3 to differential mode

        clear_state();
    }

    void sample()
    {
        _bat   = sample_ch(0x00);
        _panel = sample_ch(0x02);
     }

    void clear_state()
    {
        while(_spi.is_busy()) ; // wait for stuff to finish
        while(_spi.data_available())
            _spi.get_data_non_blocking(); // clear input buffer
    }

    int sample_ch(unsigned char ch)
    {
        clear_state();

        send_byte(0x80 | ch << 4); // start conversion of ch
        send_byte(0); // generate clock for conversion

        eMote3_GPTM::delay(10); // 10us
        send_byte(0); // generate clock for input
        send_byte(0); // generate clock

        while(_spi.is_busy()) ; // wait till done

        _spi.get_data();
        _spi.get_data(); //throwaway two bytes

        unsigned char hh = _spi.get_data();
        unsigned char ll = _spi.get_data();

        return (((int)hh << 8 | ll) - 32768) *  24576 / 32768; // returns in millivolts
    }

    void send_byte(unsigned char b)
    {
        _spi.put_data(b);
    }


    unsigned int battery_voltage() const { return _bat; }
    unsigned int panel_voltage()   const { return _panel; }

private:
    SPI _spi;
    volatile unsigned int _bat, _panel;
};


#endif
