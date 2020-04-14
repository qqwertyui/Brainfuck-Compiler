/* 
	
	Very simplified structure of bf* program which controls compile-time
	program memory values and set flags if something goes wrong.
	
*/
#ifndef BF_STATUS_H
#define BF_STATUS_H


#include "Defs.h"
#include <vector>

class Warning;

class Bf_status
{
	public:
		std::vector <Warning*> warnings;
		
		int memp;
		short *memory; // need to have buffer larger than one byte in this case
		
		enum Flag
		{
			COUNTER_UNDERFLOW = 1,
			COUNTER_OVERFLOW = 2,
			MEMORY_POSITION_OVERRUN = 3,
			MEMORY_POSITION_UNDERRUN = 4
		};
		
		Bf_status(uint memory_size);
		~Bf_status();
};

class Warning
{
	public:
		uint position;
		Bf_status::Flag type;
	
	Warning(Bf_status::Flag _type, uint _position);
	void flag_to_string(Bf_status::Flag flag, uchar *buffer);
};

#endif