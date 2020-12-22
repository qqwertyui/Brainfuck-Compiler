/*

        PE file class, which includes some basic methods
        to manage the content of those files.


        To work, must be linked along with 'libimagehlp.a', e.g.
        g++ <your_file>.cpp PE.cpp -limagehlp

*/
#ifndef PE_H
#define PE_H

#include <windows.h>
#include <winnt.h>

#include <string>
#include <vector>

#include "Defs.h"

// IMAGE_NT_HEADERS.OptionalHeader.Subsystem defines
#define UNKNOWN IMAGE_SUBSYSTEM_UNKNOWN
#define NATIVE IMAGE_SUBSYSTEM_NATIVE
#define GUI IMAGE_SUBSYSTEM_WINDOWS_GUI
#define CONSOLE IMAGE_SUBSYSTEM_WINDOWS_CUI

// Alignment
#define FILE_ALIGNMENT 0x200
#define MEMORY_ALIGNMENT 0x1000

#define MAX_STUB_SIZE 0x40

class Import_function {
 public:
  DWORD entry;
  unsigned short hint;  // ordinal number
  char funcName[256];

  Import_function();
};

class Import_library {
 public:
  IMAGE_IMPORT_DESCRIPTOR importDirectoryTable;

  // name may be confusing but it hold all funcion related content
  std::vector<Import_function*> importNameTable;

  size_t iat_rva;
  char libname[256];
  Import_library();
  ~Import_library();
};

class Section {
 public:
  IMAGE_SECTION_HEADER header;
  BYTE* data;
};

class IMAGE_DOS {
 public:
  IMAGE_DOS_HEADER dosHeader;
  BYTE* code;
  size_t code_size;
};

class PE {
  size_t calc_checksum() const;
  void fill_section_specific(size_t section_id);
  std::string filename;  // don't use

 public:
  // Control fields
  bool imports_ready;

  // Header objects
  IMAGE_DOS dosHeaderInfo;
  IMAGE_NT_HEADERS ntHeader;

  // Containers
  std::vector<Section*> sections;
  std::vector<Import_library*> imports;

  // Utils
  WORD RVAtoSection(DWORD rva);
  WORD get_section_by_name(std::string section_name) const;

  // Core functions
  PE(size_t app_type);
  ~PE();
  void add_section(std::string sectionName, BYTE* data, size_t sectionSize);
  // alloc- and write- section are used when importing functions
  void alloc_section(std::string section_name, size_t section_size);
  size_t write_section(std::string name, BYTE* data, size_t data_size);
  // void resize_section(std::string section_name, size_t new_size);
  void import(std::string libname, std::string funcname);
  size_t flush_imports(size_t address = 0);
  size_t get_iat_rv(std::string libname, std::string funcname) const;

  size_t align_file(size_t position);
  size_t align_memory(size_t position);

  void load(std::string name);
  void dump(std::string name);
};

#endif
