#ifndef _LORAMESH_H
#define _LORAMESH_H

#include <machine.h>
#include <alarm.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <uart.h>
#include <gpio.h>
// #include <machine/cortex_m/emote3_gptm.h>
// #include <machine/cortex_m/emote3_gprs.h>

#include <cstdint>
// #include <vector>

#include "defines.h"

using namespace EPOS;

class LoraMesh {

public:

	LoraMesh() {
	    // In each child class, properly initializes class
		// Must send feedback about all the steps and notify about errors
		// After that, device should be ready to make any send()

        _transparent = UART(0, 9600, 8, 0, 1); //------------------- 9600
        _command = UART(1, 9600, 8, 0, 1);

        _transparent.loopback(false);
        _command.loopback(false);

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

	uint8_t power() {
		return _power;
	}

	uint8_t cr() {
		return _cr;
	}

    bool checkBit() {
		return _checkBit;
	}

    void checkBit(bool b) {
        _checkBit = b;
    }

	//FUNCTIONS FROM RADIOENGIE DEVICE
	uint16_t CRC (uint8_t* data_in, uint32_t length);

	void localRead (uint16_t id);

	void writeConfig (uint32_t uid, uint16_t id, uint16_t net);

	void setParameters(uint16_t id, uint8_t pot, uint8_t bw, uint8_t sf, uint8_t cr);

	void getParameters();

	int traceRoute(uint16_t slave_id, uint16_t* route); //Returns number of hops

	//virtual void send(uint16_t id, uint8_t* data, int length) = 0;

	//virtual void keepReceiving() = 0; 	// Create parallel thread to keep receiving messages?
										// Only if UART interrupts are not available
										// Maybe there is something like an I/O in Radioenge that could send a signal to eMote


	// OTHER METHODS AVAILABLE
	// void loraRemoteRead (uint16_t id, uint16_t* net, uint32_t* uid, uint8_t* sf);
	// uint8_t loraGetNoise (uint16_t id, uint8_t* min, uint8_t* max);
	// void loraGetRSSI (uint16_t id, uint16_t* id_receiver, uint8_t* rssi_send, uint8_t* rssi_receive);
	// void loraGenericCommand(uint16_t id, uint8_t command, uint8_t* data_send, uint32_t length, uint8_t* response);

protected:

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

// Essential Commands:
// 1) Local Read .................................................................... (get UID, ID and NET)
// 2) Write Config .................................................................. (set ID and NET)
// 		Setting as MASTER: ID = 0
// 3) Leitura e Escrita dos parametros de modulação ................................. (set SF, BW, CR and power)
// 4) Enviar/Receber dados
// 5) Trace Route (quando testar MESH)
// Obs: 1 to 3 are also the steps to config device for the first time

// Default Configs:
// UART: 9600 8N1

//General considerations:
//	Interrupts: may require turning off and on interrupts for receiving UART in LoRa functions if using interrupts
//	Personalized command: values between 0x01 - 0x7F
//		MUST implement function to receive message and SEND A RESPONSE

// #include "../include/loraMesh.h"
// #include "../include/defines.h"

// IMPLEMENTATIONS FOR CLASS LoraMesh

// Provided by datasheet
uint16_t LoraMesh::CRC(uint8_t* data_in, uint32_t length) {

	uint32_t i;
	uint8_t bitbang, j;
	uint16_t crc_calc;
	crc_calc = 0xC181;
	for(i = 0; i < length; i++) {
		crc_calc ^= ((uint16_t)data_in[i]) & 0x00FF;
		for(j=0; j<8; j++) {
			bitbang = crc_calc;
			crc_calc >>= 1;
			if(bitbang & 1) {
				crc_calc ^= 0xA001;
			}
		}
	}
	return crc_calc;
}

void LoraMesh::localRead (uint16_t id) {
	//It is possible to get more information, those are the main and possibly necessary

	db<LoraMesh> (TRC) << "LoraMesh::localRead() called\n";

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = LOCAL_READ;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get());
	while(_command.ready_to_get()) {
		if (i < 31) {
			data[i] = _command.get();
			i++;
		}
	}

	_net = (uint16_t)(data[3] | (data[4] << 8));
	_uid = (uint32_t)(data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24));
	_sf = data[15];
}

void LoraMesh::writeConfig (uint32_t uid, uint16_t id, uint16_t net) {
//Set ID and NET to local or remote device. Must send its Unique ID.

	db<LoraMesh> (TRC) << "LoraMesh::writeConfig() called\n";

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = WRITE_CONFIG;
	data[3] = (net & 0xFF);
	data[4] = ((net & 0xFF00) >> 8);
	data[5] = (uid & 0xFF);
	data[6] = ((uid & 0xFF00) >> 8);
	data[7] = ((uid & 0xFF0000) >> 16);
	data[8] = ((uid & 0xFF000000) >> 24);
	data[9] = data[10] = data[11] = data[12] = data[13] = 0x00;

	uint16_t crc = CRC(data, 14);
	data[14] = (crc & 0xFF);
	data[15] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (i < 31) {
			data[i] = _command.get();
			i++;
		}
	}

	_id  = (data[0] | (data[1] << 8));
	_net = (data[3] | (data[4] << 8));

	if (id == (data[0] | (data[1] << 8)) && net == (data[3] | (data[4] << 8)) && uid == (data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24)))
		checkBit(true);

	else {
		checkBit(false);
		db<LoraMesh> (WRN) << "\nWarning: failed writing configurations\n\n";
	}
}

void LoraMesh::setParameters(uint16_t id, uint8_t pot, uint8_t bw, uint8_t sf, uint8_t cr) {
//Sets the power, bandwidth, spreading factor and check rate

	db<LoraMesh> (TRC) << "LoraMesh::setParameters() called\n";

	int i = 0;
	uint8_t data[] = {(uint8_t)(id & 0xFF), (uint8_t)((id & 0xFF00) >> 8), MOD_PARAM, 0x01, pot, bw, sf, cr, 0x00, 0x00};
	uint16_t crc =  CRC(data, 8);
	data[8] = (crc & 0xFF);
	data[9] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 10 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (i < 10) {
			data[i] = _command.get();
			i++;
		}
	}

	_power = data[4];
	_bw    = data[5];
	_sf    = data[6];
	_cr    = data[7];

	if (id == (data[0] | (data[1] << 8)) && pot == data[4] && bw == data[5] && sf == data[6] && cr == data[7])
		checkBit(true);

	else {
		checkBit(false);
		db<LoraMesh> (WRN) << "\nWarning: failed setting parameters\n\n";
	}
}

void LoraMesh::getParameters() {

	db<LoraMesh> (TRC) << "LoraMesh::getParameters() called\n";

	int i = 0;
	uint16_t id = this->id();
	uint8_t data[] = {(uint8_t)(id & 0xFF), (uint8_t)((id & 0xFF00) >> 8), MOD_PARAM, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint16_t crc =  CRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (i < 10) {
			data[i] = _command.get();
			i++;
		}
	}

	_power = data[4];
	_bw    = data[5];
	_sf    = data[6];
	_cr    = data[7];

	db<LoraMesh> (INF) << "\nParameters got:"
					<< "\nPower            = " << _power
					<< "\nBandwidth        = " << _bw
					<< "\nSpreading Factor = " << _bw
					<< "\nCoding Rate      = " << _bw << '\n';
}

int LoraMesh::traceRoute (uint16_t slave_id, uint16_t* route) {
//Returns number of hops

	db<LoraMesh> (TRC) << "LoraMesh::traceRoute() called\n";

	int i, j = 3;
	uint8_t* data;
	data[0] = (slave_id & 0xFF);
	data[1] = ((slave_id & 0xFF00) >> 8);
	data[2] = TRACE_ROUTE;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (_command.ready_to_get()) {
			data[i] = _command.get();
			i++;
		}
	}

	db<LoraMesh> (INF) << "\nRout traced for ID " << slave_id << ": ";

	while (j < i) {
		if (j % 2) {
			route[j / 2] = data[j];
		} else {
			route[j / 2] |= (data[j] << 8);
		}
		db<LoraMesh> (INF) << " > " << route[j / 2];
		j++;
	}

	db<LoraMesh> (INF) << "\n";

	return (i - 3) / 2;
}

// IMPLEMENTATIONS FOR CLASS GatewayLoraMesh

/*void GatewayLoraMesh::sendData(uint16_t id, uint8_t* data, int length) {

	db<LoraMesh> (TRC) << "GatewayLoraMesh::sendData() called\n";

	int i = 0;

	_transparent.put(id & 0xFF);
	_transparent.put((id & 0xFF00) >> 8);

	for (i = 0 ; i < length ; i++) {
		_transparent.put(data[i]);
	}
}

// IMPLEMENTATIONS FOR CLASS EndDeviceLoraMesh

void EndDeviceLoraMesh::sendData(uint16_t id, uint8_t* data, int length) {

	db<LoraMesh> (TRC) << "EndDeviceLoraMesh::sendData() called\n";

	int i = 0;

	for (i = 0 ; i < length ; i++) {
		_transparent.put(data[i]);
	}
}*/

//Drafts for other functions
/*
void LoraMesh::loraRemoteRead (uint16_t id, uint16_t* net, uint32_t* uid, uint8_t* sf) {
	//It is possible to get more information, those are the main and possibly necessary

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = REMOTE_READ;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (i < 31) {
			data[i] = _command.get();
			i++;
		}
	}

	*net = (uint16_t)(data[3] | (data[4] << 8));
	*uid = (uint32_t)(data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24));
	*sf  = data[15];
}

uint8_t LoraMesh::loraGetNoise (uint16_t id, uint8_t* min, uint8_t* max) {
//Return average noise

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = NOISE;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (i < 8) {
			data[i] = _command.get();
			i++;
		}
	}

	*min = data[3];
	*max = data[5];

	return data[4];
}

void LoraMesh::loraGetRSSI (uint16_t id, uint16_t* id_receiver, uint8_t* rssi_send, uint8_t* rssi_receive) {

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = RSSI;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (i < 9) {
			data[i] = _command.get();
			i++;
		}
	}

	*id_receiver = (data[3] | (data[4] << 8));
	*rssi_send = data[5];
	*rssi_receiver = data[6];
}

void LoraMesh::loraGenericCommand(uint16_t id, uint8_t command, uint8_t* data_send, uint32_t length, uint8_t* response) {
//Personalized command: values between 0x01 - 0x7F
//MUST implement function to receive message in target node and SEND A RESPONSE
//	Response format: ID_L, ID_H, Command, Data0 ... DataN, CRC_L, CRC_H

	int i, j = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = command;
	for (i = 0 ; i < length ; i++)
		data[i + 3] = data_send[i];

	uint16_t crc = CRC(data, length + 3);
	data[length + 3] = (crc & 0xFF);
	data[length + 4] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		_command.put(data[i]);
	}
	i = 0;

	while(!_command.ready_to_get()){}
	while(_command.ready_to_get()) {
		if (_command.ready_to_get()) {
			data[i] = _command.get();
			i++;
		}
	}

	for (j = 0 ; j < i ; j++) {
		response[j] = data[j];
	}
}

void LoraMesh::loraSendData(uint16_t id, uint8_t* data, int length, int isMaster) {
	int i = 0;
	if (isMaster) {
		_transparent.put(id & 0xFF);
		_transparent.put((id & 0xFF00) >> 8);
	}

	for (i = 0 ; i < length ; i++) {
		_transparent.put(data[i]);
	}
}
*/
//https://lisha.ufsc.br/teaching/dos-old/epos_introduction.html


#endif
