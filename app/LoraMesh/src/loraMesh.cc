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

#include "../include/loraMesh.h"

// IMPLEMENTATIONS FOR CLASS GatewayLoraMesh

void GatewayLoraMesh::sendData(uint16_t id, uint8_t* data) {
// Sends data to node defined by "id"

	db<LoraMesh> (TRC) << "GatewayLoraMesh::sendData() called\n";

	_transparent.put(id & 0xFF);
	_transparent.put((id >> 8) & 0xFF);

	for (int i = 0 ; i < strlen(data) ; i++) {
		_transparent.put(data[i]);
	}
}

void GatewayLoraMesh::sendDataToAll(uint8_t* data) {
// Sends data to all nodes

    sendData(BROADCAST_ID, data);
}

// IMPLEMENTATIONS FOR CLASS EndDeviceLoraMesh

void EndDeviceLoraMesh::sendData(uint16_t id, uint8_t* data) {
// Sends data to gateway

	db<LoraMesh> (TRC) << "EndDeviceLoraMesh::sendData() called\n";

	for (int i = 0 ; i < strlen(data) ; i++) {
		_transparent.put(data[i]);
	}
}

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
