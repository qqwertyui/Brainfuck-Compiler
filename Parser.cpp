#include "Parser.h"

#include <ctype.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

char *Parser::optarg = nullptr;
char *Parser::optstr = nullptr;
std::vector<char> Parser::flags = {};

char Parser::getopt(int argc, char **argv, const char *optstring) {
  if (Parser::optstr == nullptr) {
    Parser::optarg = nullptr;
    Parser::optstr = (char *)optstring;
  }
  char result = 0;
  char **arg = nullptr;
  if (Parser::optstr[0] == ':') {
    Parser::optstr++;
  }
  if (isalpha(Parser::optstr[0])) {
    arg = Parser::findarg(argc, argv, Parser::optstr[0]);
    if (arg == nullptr) {
      result = Parser::Status::NOT_FOUND;  // argument not found, keep looping
    } else if (Parser::optstr[1] == ':') {
      if (arg[1] && arg[1][0] != '-') {
        Parser::optarg = arg[1];
        result = arg[0][1];
      } else {
        fprintf(stderr, "%c: %s\n", arg[0][1], "Requires an argument");
        exit(1);
      }
    } else {
      if (arg[0][0] == '-' && arg[0][1]) {
        Parser::optarg = nullptr;
        result = arg[0][1];
      }
    }
  } else {
    result = Parser::Status::END_OF_ARGS;
  }
  Parser::optstr++;
  return result;
}

char **Parser::findarg(int argc, char **argv, char searched) {
  for (int i = 0; i < argc; i++) {
    if (argv[i][0] == '-' &&
        argv[i][1] == searched) {  // nothing happens if we hit null character
      return &argv[i];
    }
  }
  return nullptr;
}

void Parser::set(const char flag) { Parser::flags.push_back(flag); }

bool Parser::isset(const char flag) {
  for (const char i : Parser::flags) {
    if (i == flag) {
      return true;
    }
  }
  return false;
}
