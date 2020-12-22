/*

        Class which maintains command line user input, parses it
        and sets proper flags.

*/
#ifndef PARSER_H
#define PARSER_H

#include <vector>

class Parser {
  static char **findarg(int argc, char **argv, char searched);
  static char *optstr;
  static std::vector<char> flags;

  enum Status { NOT_FOUND = -1, END_OF_ARGS = 0 };

 public:
  static char getopt(int argc, char **argv, const char *optstring);
  static void set(const char flag);
  static bool isset(const char flag);
  static void version(unsigned int version, unsigned int subversion);
  static void help();

  static char *optarg;
};

#endif
