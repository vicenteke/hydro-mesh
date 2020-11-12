// EPOS Hydrology Board Mediator Common Package

#ifndef __lora_mesh_h
#define __lora_mesh_h

#include <system/config.h>

__BEGIN_SYS

class Lora_Mesh_Common
{
protected:
    Lora_Mesh_Common() {}
};

__END_SYS

#ifdef __LORA_MESH_H
#include __LORA_MESH_H
#else
__BEGIN_SYS
class Lora_Mesh: public Dummy {};
__END_SYS
#endif

#endif
