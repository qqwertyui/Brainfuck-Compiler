#include "Parser.h"
#include <cstdio>
#include <cstring>

Parser::Parser(int argc, char **argv)
{
	this->flags = 0;
	if(argc < 3)
	{
		flags |= Parser::Flags::NOT_ENOUGH_ARGS;
	}
	while(*(++argv) != NULL)
	{
		if(!strcmp(*argv, "-f") || !strcmp(*argv, "--force") )
			flags |= Parser::Flags::FORCE;
		if(!strcmp(*argv, "-h") || !strcmp(*argv, "--help") )
			flags |= Parser::Flags::HELP;
		if(!strcmp(*argv, "-v") || !strcmp(*argv, "--version") )
		{
			flags |= Parser::Flags::_VERSION;
			flags &= ~(flags<<Parser::Flags::HELP);
		}
	}
}

void 
Parser::help() const
{
	puts(
	"Usage: bfc <source> <destination> [options]\n"
	"\t-f, --force	ignore all warnings and compile\n"
	"\t-v, --version	prints current compiler version\n"
	"\t-h, --help	prints this message\n"
	);
}

void 
Parser::version(unsigned int version, unsigned int subversion) const
{
	printf("brainfuck compiler (bfc) %d.%d\n", version, subversion);
}

bool 
Parser::is_good() const
{
	/*
		E.g. bfc code.bf out.exe --help
		is going to pass following ones
	*/
	if(this->flags & Parser::Flags::NOT_ENOUGH_ARGS)
		return false;
	else if(this->flags & Parser::Flags::HELP)
		return false;
	else if(this->flags & Parser::Flags::_VERSION)
		return false;
	return true;
}

void 
Parser::validate() const
{
	if(this->flags & Parser::Flags::HELP)
		this->help();
	else if(this->flags & Parser::Flags::_VERSION)
		this->version(this->VERSION, this->SUBVERSION);
	else if(this->flags & Parser::Flags::NOT_ENOUGH_ARGS)
		this->help();
}

bool 
Parser::force() const
{
	return (this->flags & Parser::Flags::FORCE);
}