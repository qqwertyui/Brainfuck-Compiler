/*

	Class which maintains command line user input, parses it
	and sets proper flags.

*/
#ifndef PARSER_H
#define PARSER_H


class Parser
{
	unsigned char flags;
	
	void version(unsigned int version, unsigned int subversion) const;
	void help() const;
	
	public:
		static const unsigned int SUBVERSION = 1;
		static const unsigned int VERSION = 0;
		enum Flags
		{
			NOT_ENOUGH_ARGS = (1<<0),
			FORCE = (1<<1),
			HELP = (1<<2),
			_VERSION = (1<<3)
		};
	
		Parser(int argc, char **argv);

		void validate() const;
		bool is_good() const;
		bool force() const;
};

#endif