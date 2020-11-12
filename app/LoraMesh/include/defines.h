#ifndef _DEFINES_H
#define _DEFINES_H

// IMPORTANT: this program does not use default UART pins
// PC5 = RX & PC6 = TX
// Make sure that is properly configured in <include/machine/cortex/emote3.h>

#define LORA_RX_PIN 5
#define LORA_TX_PIN 6

//Commands list
#define MOD_PARAM	     0xD6
#define LOCAL_READ	     0xE2
#define REMOTE_READ	     0xD4
#define WRITE_CONFIG	 0xCA
#define COMMAND_IO	     0xC2
#define DIAGNOSE	     0xE7
#define NOISE		     0xD8
#define RSSI		     0xD5
#define TRACE_ROUTE	     0xD2
#define SEND_PACK	     0x28

#define MASTER_ID        0
#define BROADCAST_ID     2047   // Sending to this ID send the command to all the nodes in network
                                // Can be used only by gateway

// Configurations for HYDRO_MESH
#define HYDRO_NET        1562  // Any value from 0 to 2047

#define HYDRO_SF         12  // Ranges from 7 to 12 for SF
                            // or 5 = FSK

#define HYDRO_BW         2  // 0 = 125kHz
                            // 1 = 250kHz
                            // 2 = 500kHz

#define HYDRO_CR         4  // 1 = 4/5
                            // 2 = 4/6
                            // 3 = 4/7
                            // 4 = 4/8

#define HYDRO_POWER      20 // Ranges from 0 to 20 dBm

#endif
