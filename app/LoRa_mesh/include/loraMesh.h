#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
#include <machine/cortex_m/emote3_gptm.h>
#include <machine/cortex_m/emote3_gprs.h>

#include <cstdint>
#include <vector>

using namespace EPOS;

class LoraMesh {

public:

	LoraMesh() {
	    // In each child class, properly initializes class
		// Must send feedback about all the steps and notify about errors
		// After that, device should be ready to make any send()

        _transparent = UART(0, 115200, 8, 0, 1); //------------------- 9600
        _command = UART(1, 115200, 8, 0, 1);
        _checkBit = true;

	}

	~LoraMesh() {}	// Destruction of the network must be deeply analysed
					//
					// Consider:
					//		1) Removing leaf End Device
					//		2) Removing bridge End Device
					//		3) Removing gateway with no End Devices
					//		4) Removing gateway with devices
					//		5) Simply disabling all the network
					//
					// Should it be a way to advise gateway/end devices that node is going to be deleted?
					// Maybe something like a button interrupt that calls delete device;

	//GETTERS
	uint32_t uid() {
		return _uid;
	}

	uint16_t id() {
		return _id;
	}

	uint16_t net() {
		return _net;
	}

	uint8_t sf() {
		return _sf;
	}

	uint8_t bw() {
		return _bw;
	}

	uint8_t potencia() {
		return _potencia;
	}

	uint8_t cr() {
		return _cr;
	}

    bool checkBit() {
		return _checkBit;
	}

	//FUNCTIONS FROM RADIOENGIE DEVICE
	uint16_t CRC (uint8_t* data_in, uint32_t length);

	void localRead (uint16_t id);

	void writeConfig (uint32_t uid, uint16_t id, uint16_t net);

	void setParameters(uint16_t id, uint8_t pot, uint8_t bw, uint8_t sf, uint8_t cr);

	void getParameters();

	int traceRoute(uint16_t slave_id, uint16_t* route); //Returns number of hops

	virtual void send(uint16_t id, uint8_t* data, int length) = 0;

	//virtual void keepReceiving() = 0; 	// Create parallel thread to keep receiving messages?
										// Only if UART interrupts are not available
										// Maybe there is something like an I/O in Radioenge that could send a signal to eMote


	// OTHER METHODS AVAILABLE
	// void loraRemoteRead (uint16_t id, uint16_t* net, uint32_t* uid, uint8_t* sf);
	// uint8_t loraGetNoise (uint16_t id, uint8_t* min, uint8_t* max);
	// void loraGetRSSI (uint16_t id, uint16_t* id_receiver, uint8_t* rssi_send, uint8_t* rssi_receive);
	// void loraGenericCommand(uint16_t id, uint8_t command, uint8_t* data_send, uint32_t length, uint8_t* response);

private:

	uint32_t _uid;			// unique ID
	uint16_t _id;			// device's ID
	uint16_t _net;			// network
	uint8_t  _sf;			// spreading factor
	uint8_t  _bw;			// bandwidth
	uint8_t  _cr;			// coding rate
	uint8_t  _power;		// power for antenna

	bool	 _checkBit;		// true if the last operation went successful.
							// Not appliable to: localRead, getParameters.

	UART _transparent;		// transparent UART (A0/A1): send/receive data
	UART _command;			// command UART (C3/C4): send/receive configuration data and commands

};

class GatewayLoraMesh : public LoraMesh {

public:

    GatewayLoraMesh(uint16_t net = HYDRO_NET,
                    uint8_t sf = HYDRO_SF,
                    uint8_t bw = HYDRO_BW,
                    uint8_t cr = HYDRO_CR,
                    uint8_t power = HYDRO_POWER) {

        localRead(MASTER_ID);
        writeConfig(_uid, MASTER_ID, net);
        setParameters(MASTER_ID, power, bw, sf, cr);
    }

    ~GatewayLoraMesh() {}

private:

	// vector end devices? Maybe use something from EPOS communication abstractions

	// OBS:
		// 1) For each message received, send ACK (all zeros)

};

class EndDeviceLoraMesh : public LoraMesh {

public:
    EndDeviceLoraMesh(uint16_t id,
                    uint16_t net = HYDRO_NET,
                    uint8_t sf = HYDRO_SF,
                    uint8_t bw = HYDRO_BW,
                    uint8_t cr = HYDRO_CR,
                    uint8_t power = HYDRO_POWER) {

        localRead(id);
        writeConfig(_uid, id, net);
        setParameters(id, power, bw, sf, cr);
    }

    ~EndDeviceLoraMesh() {}

private:

	// vector route?

};
