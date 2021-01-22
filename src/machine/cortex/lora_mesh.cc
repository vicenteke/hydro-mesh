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

#include <system/config.h>

#ifdef __LORA_MESH_H

#include <machine/cortex/lora_mesh.h>

__BEGIN_SYS

using namespace EPOS;

// Timers static members
unsigned long long GW_Timer::_count;
unsigned long long ED_Timer::_count;

// Lora_Mesh static members
GPIO Lora_Mesh::_interrupt('C', LORA_RX_PIN, GPIO::IN);
OStream Lora_Mesh::cout;
UART Lora_Mesh::_transparent;
Mutex Lora_Mesh::_mutex = Mutex();
int Lora_Mesh::_id;

// Gateway_Lora_Mesh static members
// GPIO Gateway_Lora_Mesh::_interrupt('C', LORA_RX_PIN, GPIO::IN);
void (*Gateway_Lora_Mesh::_handler)(int, char *);
GW_Timer* Gateway_Lora_Mesh::_timer;
// Gateway_Lora_Mesh::nodes_t Gateway_Lora_Mesh::_nodes;

// EndDevice_Lora_Mesh static members
// GPIO EndDevice_Lora_Mesh::_interrupt('C', LORA_RX_PIN, GPIO::IN);
void (*EndDevice_Lora_Mesh::_handler)(char *);
ED_Timer* EndDevice_Lora_Mesh::_timer;

__END_SYS

#endif
