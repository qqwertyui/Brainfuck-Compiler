#include "Compiler.h"
#include "Parser.h"

int main(int argc, char **argv)
{
	Parser parse(argc, argv);
	if(!parse.is_good() )
	{
		parse.validate();
		return 1;
	}
	
	PE pe(CONSOLE);
	
	#warning write resize function for sections
	pe.alloc_section(".text", 0x2000);
	pe.import("msvcrt.dll",	"fputc");
	pe.import("msvcrt.dll", "fgetc");
	pe.import("msvcrt.dll", "memset");
	pe.import("msvcrt.dll", "_iob");
	pe.flush_imports(); // writes imports into memory
	
	Compiler core(argv[1], &pe);
	core.compile(0x1000); // 0x1000 is memory size
	if(!core.status->warnings.size() || parse.force() )
	{
		if(parse.force() )
			puts("Silencing warnings.");
		pe.write_section(".text", core.code, core.code_length);
		
		pe.dump(argv[2] );
	}
	else
	{
		puts("Following warnings were triggered:");
		uchar buffer[256] = { 0 };
		for(size_t i=0; i<core.status->warnings.size(); i++)
		{
			core.status->warnings[i]->flag_to_string(core.status->warnings[i]->type, buffer);
			printf("%s, Position: %d\n", buffer, core.status->warnings[i]->position);
		}
	}
	return 0;
}