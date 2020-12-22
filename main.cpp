#include "Compiler.h"
#include "Parser.h"

#define VERSION 0
#define SUBVERSION 1

enum Status {
  BF_SUCCESS = 0,
  BF_INSUFFICENT_ARGS = 1,
  BF_MISSING_INPUT = 2,
  BF_COMPILER_ERROR = 3
};

void help() {
  puts(
      "Usage: ./bfc.exe -i <src> -o <dest> -[fvh]\n"
      "\t-i, specify input file\n"
      "\t-o, specify output file (default is a.exe)\n"
      "\t-m, custom stack size in bytes (default is 4096)\n"
      "\t-f, ignore all warnings and compile\n"
      "\t-v, prints current compiler version\n"
      "\t-h, prints this message\n");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    help();
    return Status::BF_INSUFFICENT_ARGS;
  }
  const char *input_file = nullptr;
  const char *output_file = nullptr;
  unsigned int memory_size = 0x1000;
  char c;
  while ((c = Parser::getopt(argc, argv, "hvi:fo:m:")) != 0) {
    switch (c) {
      case 'h': {
        help();
        return Status::BF_INSUFFICENT_ARGS;
      }
      case 'v': {
        printf("Brainfuck language compiler (bfc) %d.%d\n", VERSION,
               SUBVERSION);
        return Status::BF_SUCCESS;
      }
      case 'i': {
        input_file = Parser::optarg;
        break;
      }
      case 'o': {
        output_file = Parser::optarg;
        break;
      }
      case 'f': {
        Parser::set('f');
        break;
      }
      case 'm': {
        memory_size = std::atoi(Parser::optarg);
        break;
      }
      default: {
        break;
      }
    }
  }
  if (input_file == nullptr) {
    fputs("You need to pass input file first (-i <filename>)\n", stderr);
    return Status::BF_MISSING_INPUT;
  }
  if (output_file == nullptr) {
    output_file = "a.exe";
  }

  PE pe(CONSOLE);

#warning write resize function for sections
  pe.alloc_section(".text", 0x2000);
  pe.import("msvcrt.dll", "fputc");
  pe.import("msvcrt.dll", "fgetc");
  pe.import("msvcrt.dll", "memset");
  pe.import("msvcrt.dll", "_iob");
  pe.flush_imports();  // writes imports into memory

  Compiler engine(input_file, &pe);
  if (engine.is_good() == false) {
    puts("Failed to run compiler module");
    return Status::BF_COMPILER_ERROR;
  }
  engine.compile(memory_size);
  pe.write_section(".text", engine.i386_code, engine.i386_code_length);
  pe.dump(output_file);

  printf("Output code has %d bytes size\n", engine.i386_code_length);
  return Status::BF_SUCCESS;
}
