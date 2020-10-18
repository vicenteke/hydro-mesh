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


//Select the proper uart base
#define TRANSPARENT_UART 	UARTX_BASE
#define COMMAND_UART 		UARTX_BASE

//Commands list
#define MOD_PARAM	0xD6
#define LOCAL_READ	0xE2
#define REMOTE_READ	0xD4
#define WRITE_CONFIG	0xCA
#define COMMAND_IO	0xC2
#define DIAGNOSE	0xE7
#define NOISE		0xD8
#define RSSI		0xD5
#define TRACE_ROUTE	0xD2
#define SEND_PACK	0x28

//Use define or get/set it through command interface
uint8_t L_ID, M_ID;

//Also L_ID = (ID & 0x0F) and M_ID = (ID & 0xF0)

//Provided by datasheet
uint16_t CalculaCRC(uint8_t* data_in, uint32_t length) {
	uint32_t i;
	uint8_t bitbang, j;
	uint16_t crc_calc;
	crc_calc = 0xC181;
	for(i=0; i<length; i++) {
		crc_calc ^= ((uint16_t)data_in[i]) & 0x00FF;
		for(j=0; j<8; j++) {
			bitbang = crc_calc;
			crc_calc >>= 1;
			if(bitbang & 1) {
				crc_calc ^= CRC_POLY;
			}
		}
	}
	return crc_calc;
}

void loraSendData(uint16_t id, uint8_t* data, int length, int isMaster) {
	int i = 0;
	if (isMaster) {
		UARTCharPutNonBlocking(TRANSPARENT_UART, (id & 0xFF));
		UARTCharPutNonBlocking(TRANSPARENT_UART, ((id & 0xFF00) >> 8));
	}

	for (i = 0 ; i < length ; i++) {
		UARTCharPutNonBlocking(TRANSPARENT_UART, data[i]);
	}
}

void loraGetParam(uint16_t id, uint8_t* pot, uint8_t* bw, uint8_t* sf, uint8_t* cr) {
	int i = 0;
	uint8_t* data = {(id & 0xFF), ((id & 0xFF00) >> 8), MOD_PARAM, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint16_t crc =  CalculaCRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 10) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}

	*pot = data[4];
	*bw  = data[5];
	*sf  = data[6];
	*cr  = data[7];
}

int loraSetParam(uint16_t id, uint8_t pot, uint8_t bw, uint8_t sf, uint8_t cr) {
	int i = 0;
	uint8_t* data = {(id & 0xFF), ((id & 0xFF00) >> 8), MOD_PARAM, 0x01, pot, bw, sf, cr, 0x00, 0x00};
	uint16_t crc =  CalculaCRC(data, 8);
	data[8] = (crc & 0xFF);
	data[9] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 10 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 10) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}

	if (id == (data[0] | (data[1] << 8)) && pot == data[4] && bw == data[5] && sf == data[6] && cr == data[7])
		return 1;

	return 0;
}

void loraLocalRead (uint16_t id, uint16_t* net, uint32_t* uid, uint8_t* sf) {
	//It is possible to get more information, those are the main and possibly necessary

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = LOCAL_READ;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CalculaCRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 31) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}

	*net = (uint16_t)(data[3] | (data[4] << 8));
	*uid = (uint32_t)(data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24));
	*sf  = data[15];
}

void loraRemoteRead (uint16_t id, uint16_t* net, uint32_t* uid, uint8_t* sf) {
	//It is possible to get more information, those are the main and possibly necessary

	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = REMOTE_READ;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CalculaCRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 31) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}

	*net = (uint16_t)(data[3] | (data[4] << 8));
	*uid = (uint32_t)(data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24));
	*sf  = data[15];
}

int loraWriteConfig (uint32_t uid, uint16_t id, uint16_t net) {
//Set ID and NET to local or remote device. Must send its Unique ID.

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

	uint16_t crc = CalculaCRC(data, 14);
	data[14] = (crc & 0xFF);
	data[15] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 31) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}

	if (id == (data[0] | (data[1] << 8)) && net == (data[3] | (data[4] << 8)) && uid == (data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24)))
		return 1;

	return 0;
}

uint8_t loraGetNoise (uint16_t id, uint8_t* min, uint8_t* max) {
//Return average noise
	
	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = NOISE;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CalculaCRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 8) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}
	
	*min = data[3];
	*max = data[5];

	return data[4];
}

void loraGetRSSI (uint16_t id, uint16_t* id_receiver, uint8_t* rssi_send, uint8_t* rssi_receive) {
	
	int i = 0;
	uint8_t* data;
	data[0] = (id & 0xFF);
	data[1] = ((id & 0xFF00) >> 8);
	data[2] = RSSI;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CalculaCRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART) && i < 9) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}
	
	*id_receiver = (data[3] | (data[4] << 8));
	*rssi_send = data[5];
	*rssi_receiver = data[6];
}

int loraTraceRoute (uint16_t slave_id, uint16_t* route) {
//Returns number of hops

	int i, j = 3;
	uint8_t* data;
	data[0] = (slvae_id & 0xFF);
	data[1] = ((slave_id & 0xFF00) >> 8);
	data[2] = TRACE_ROUTE;
	data[3] = data[4] = data[5] = 0x00;

	uint16_t crc = CalculaCRC(data, 6);
	data[6] = (crc & 0xFF);
	data[7] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART)) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}
	
	while (j < i) {
		if (j % 2) {
			route[j / 2] = data[j];
		} else {
			route[j / 2] |= (data[j] << 8);
		}
		j++;
	}

	return (i - 3) / 2;
}

void loraGenericCommand(uint16_t id, uint8_t command, uint8_t* data_send, uint32_t length, uint8_t* response) {
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

	uint16_t crc = CalculaCRC(data, length + 3);
	data[length + 3] = (crc & 0xFF);
	data[length + 4] = ((crc & 0xFF00) >> 8);

	for (i = 0 ; i < 8 ; i++) {
		UARTCharPutNonBlocking(COMMAND_UART, data[i]);
	}
	i = 0;

	while(!CharsAvail(COMMAND_UART)){}
	while(CharsAvail(COMMAND_UART)) {
		if (CharsAvail(COMMAND_UART)) {
			data[i] = UARTCharGet(COMMAND_UART);
			i++;
		}
	}

	for (j = 0 ; j < i ; j++) {
		response[j] = data[j];
	}
}

//https://lisha.ufsc.br/teaching/dos-old/epos_introduction.html
