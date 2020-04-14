#include "Bf_status.h"
#include <cstring>


Bf_status::Bf_status(uint memory_size)
{
	this->memp = 0;
	this->memory = new short [memory_size];
	memset(this->memory, 0, sizeof(short) * memory_size);
}


Bf_status::~Bf_status()
{
	for(size_t i=0; i<warnings.size(); i++)
	{
		delete warnings[i];
		warnings.pop_back();
	}
}


Warning::Warning(Bf_status::Flag _type, uint _position)
{
	this->type = _type;
	this->position = _position;
}


void 
Warning::flag_to_string(Bf_status::Flag flag, uchar *buffer)
{	
	switch(flag)
	{
		case Bf_status::Flag::COUNTER_UNDERFLOW: strcpy( (char*)buffer, "COUNTER_UNDERFLOW"); break;
		case Bf_status::Flag::COUNTER_OVERFLOW: strcpy( (char*)buffer, "COUNTER_OVERFLOW"); break;
		case Bf_status::Flag::MEMORY_POSITION_OVERRUN: strcpy( (char*)buffer, "MEMORY_POSITION_OVERRUN"); break;
		case Bf_status::Flag::MEMORY_POSITION_UNDERRUN: strcpy( (char*)buffer, "MEMORY_POSITION_UNDERRUN"); break;
	}
}