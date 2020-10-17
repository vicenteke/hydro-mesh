// EPOS ARM Cortex INA219 Current Shunt and Power Monitor Mediator Declarations

#ifndef __cortex_ina2019_h_
#define __cortex_ina2019_h_

#include <alarm.h>
#include <i2c.h>

__BEGIN_SYS

class OPT3001
{
private:
    enum { ADDRESS = 0x44 };

    enum Register {
        RESULT           = 0X00,
        CONFIGURATION    = 0x01,
        LOW_LIMIT        = 0x02,
        HIGH_LIMIT       = 0x03,
        MANUFACTURER_ID  = 0x7E,
        DEVICE_ID        = 0x7F,
    };

public:

    // Valid values for field E3 to E0 (Full-Scale Range)
    enum {
        FULL_SCALE_0  = 0 << 12,
        FULL_SCALE_1  = 1 << 12,
        FULL_SCALE_2  = 2 << 12,
        FULL_SCALE_3  = 3 << 12,
        FULL_SCALE_4  = 4 << 12,
        FULL_SCALE_5  = 5 << 12,
        FULL_SCALE_6  = 6 << 12,
        FULL_SCALE_7  = 7 << 12,
        FULL_SCALE_8  = 8 << 12,
        FULL_SCALE_9  = 9 << 12,
        FULL_SCALE_10 = 10 << 12,
        FULL_SCALE_11 = 11 << 12,
        FULL_SCALE_12 = 12 << 12 // default
    };    
    
    // Valid values for field CT (Conversion Time)
    enum {
        CT_100ms   = 0 << 11,
        CT_800ms   = 1 << 11 // default
    };

    // Valid values for field M1 to M0 (Mode)
    enum {
        SHUT_DOWN   = 0 << 9, //default
        SING_SHOT   = 1 << 9,
        CONTINUOUS_0  = 2 << 9,
        CONTINUOUS_1  = 3 << 9 
    };

    // Valid values for field L (Latch)
    enum {
        TRANSPARENT   = 0 << 4, //default
        LATCH   = 1 << 4
    };

    // Valid values for field FC1 to FC0 (Mode)
    enum {
        ONE_FAULT   = 0 << 0, // default
        TWO_FAULT   = 1 << 0,
        FOUR_FAULT  = 2 << 0, 
        EIGHT_FAULT = 3 << 0 
    };

public:
    OPT3001(char port_sda = 'B', unsigned int pin_sda = 1, char port_scl = 'B', unsigned int pin_scl = 0) : _i2c(I2C_Common::MASTER, port_sda, pin_sda, port_scl, pin_scl){
        configure(FULL_SCALE_12 | CT_800ms | CONTINUOUS_0 | TRANSPARENT | ONE_FAULT);      
        _i2c.put(ADDRESS, RESULT , false); 
    }

    bool configure(unsigned int c) {
        char configuration[3];
        configuration[0] = CONFIGURATION;
        configuration[1] = (c >> 8) & 0xFF;
        configuration[2] = (c >> 0) & 0xFF;
        return !_i2c.put(ADDRESS, configuration, 3, true);
    }

    int get() { while(!_i2c.ready_to_get()); return OPT3001::lux(); }

private:
    float lux(){
        char data[2];
        if (_i2c.get(ADDRESS, data, 2, true))
            return -1; // this is an error
        unsigned short raw = ((data[0] << 8) | data[1]);
        int exponent = (raw >> 12) & 0x000F;
        float fractional_result = raw & 0x0FFF;
        float result = 0;

        switch(exponent){

    		case 0: //*0.015625
    			result = fractional_result*0.01;
    			break;
    		case 1: //*0.03125
    			result = fractional_result*0.02;
    			break;
    		case 2: //*0.0625
    			result = fractional_result*0.04;
    			break;
    		case 3: //*0.125
    			result = fractional_result*0.08;
    			break;
    		case 4: //*0.25
    			result = fractional_result*0.16;
    			break;
    		case 5: //*0.5
    			result = fractional_result*0.32;
    			break;
    		case 6:
    			result = fractional_result*0.64;
    			break;
    		case 7: //*2
    			result = fractional_result*1.28;
    			break;
    		case 8: //*4
    			result = fractional_result*2.56;
    			break;
    		case 9: //*8
    			result = fractional_result*5.12;
    			break;
    		case 10: //*16
    			result = fractional_result*10.24;
    			break;
    		case 11: //*32
    			result = fractional_result*20.48;
    			break;
    	}
                
	    return raw;
	
    }

private:
    I2C _i2c;
};

__END_SYS

#endif
