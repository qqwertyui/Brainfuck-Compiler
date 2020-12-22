#include "PE.h"

#include <imagehlp.h>

#include <cstdio>
#include <ctime>

PE::PE(size_t app_type) {
  this->imports_ready = false;

  /*
          IMAGE_DOS_HEADER
  */
  this->dosHeaderInfo.dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
  this->dosHeaderInfo.dosHeader.e_cblp = 0x90;
  this->dosHeaderInfo.dosHeader.e_cp = 0x3;
  this->dosHeaderInfo.dosHeader.e_cparhdr = 0x4;
  this->dosHeaderInfo.dosHeader.e_maxalloc = 0xFFFF;
  this->dosHeaderInfo.dosHeader.e_sp = 0xB8;
  this->dosHeaderInfo.dosHeader.e_lfarlc = 0x40;
  this->dosHeaderInfo.dosHeader.e_lfanew = 0x80;

  // Standard DOS stub code, the same which appears in most PE's
  unsigned char bytes[] = {
      0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD, 0x21, 0xB8, 0x01,
      0x4C, 0xCD, 0x21, 0x54, 0x68, 0x69, 0x73, 0x02, 0x70, 0x72, 0x6F,
      0x67, 0x72, 0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F, 0x74,
      0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 0x20, 0x69, 0x6E, 0x20,
      0x44, 0x4F, 0x53, 0x20, 0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D,
      0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  this->dosHeaderInfo.code_size = MAX_STUB_SIZE;
  this->dosHeaderInfo.code = new BYTE[MAX_STUB_SIZE];
  memcpy(this->dosHeaderInfo.code, (void*)bytes, sizeof(bytes));

  /*
          signature + IMAGE_FILE_HEADER
  */
  memset(&this->ntHeader, 0, sizeof(IMAGE_NT_HEADERS));
  this->ntHeader.Signature = IMAGE_NT_SIGNATURE;
  this->ntHeader.FileHeader.Machine = IMAGE_FILE_MACHINE_I386;

  // 'add_section' takes care of this one
  this->ntHeader.FileHeader.NumberOfSections = 0;

  // Can be changed to spoof timestamp
  this->ntHeader.FileHeader.TimeDateStamp = std::time(0);

  // todo
  this->ntHeader.FileHeader.PointerToSymbolTable = 0;
  this->ntHeader.FileHeader.NumberOfSymbols = 0;

  // Fixed value (always?)
  this->ntHeader.FileHeader.SizeOfOptionalHeader =
      sizeof(IMAGE_OPTIONAL_HEADER);

  // Some standard flags
  this->ntHeader.FileHeader.Characteristics =
      IMAGE_FILE_RELOCS_STRIPPED | IMAGE_FILE_EXECUTABLE_IMAGE |
      IMAGE_FILE_LINE_NUMS_STRIPPED | IMAGE_FILE_32BIT_MACHINE |
      IMAGE_FILE_DEBUG_STRIPPED;

  /*
          IMAGE_OPTIONAL_HEADER
  */
  this->ntHeader.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;

  this->ntHeader.OptionalHeader.MajorLinkerVersion = 0;  // ???
  this->ntHeader.OptionalHeader.MinorLinkerVersion = 0;  // ???

  // 'add_section' takes care of those
  this->ntHeader.OptionalHeader.SizeOfCode = 0;
  this->ntHeader.OptionalHeader.SizeOfInitializedData = 0;
  this->ntHeader.OptionalHeader.SizeOfUninitializedData = 0;
  this->ntHeader.OptionalHeader.AddressOfEntryPoint = 0;
  this->ntHeader.OptionalHeader.BaseOfCode = 0;
  this->ntHeader.OptionalHeader.BaseOfData = 0;

  // Standard base and alignment
  this->ntHeader.OptionalHeader.ImageBase = 0x400000;
  this->ntHeader.OptionalHeader.SectionAlignment = MEMORY_ALIGNMENT;
  this->ntHeader.OptionalHeader.FileAlignment = FILE_ALIGNMENT;

  // Don't know which values are ok, so using those
  this->ntHeader.OptionalHeader.MajorOperatingSystemVersion = 4;
  this->ntHeader.OptionalHeader.MinorOperatingSystemVersion = 0;
  this->ntHeader.OptionalHeader.MajorImageVersion = 1;
  this->ntHeader.OptionalHeader.MinorImageVersion = 0;
  this->ntHeader.OptionalHeader.MajorSubsystemVersion = 4;
  this->ntHeader.OptionalHeader.MinorSubsystemVersion = 0;
  this->ntHeader.OptionalHeader.Win32VersionValue = 0;

  /*
          'add_section' takes care of those (it is important
          to have appropriate values here, so be careful when
          changing anything here
  */
  this->ntHeader.OptionalHeader.SizeOfImage = 0x1000;  // warning
  this->ntHeader.OptionalHeader.SizeOfHeaders =
      this->dosHeaderInfo.dosHeader.e_lfanew +
      sizeof(IMAGE_NT_HEADERS);  // warning

  // 'calc_checksum' fills this field when dumping
  this->ntHeader.OptionalHeader.CheckSum = 0;

  // See constants defined in 'PE.h'
  this->ntHeader.OptionalHeader.Subsystem = app_type;

  /*
          https://docs.microsoft.com/en-us/windows/win32/\
          debug/pe-format#dll-characteristics
  */
  this->ntHeader.OptionalHeader.DllCharacteristics = 0;

  // Standard values found in other PE, seems good
  this->ntHeader.OptionalHeader.SizeOfStackReserve = 0x200000;
  this->ntHeader.OptionalHeader.SizeOfStackCommit = 0x1000;
  this->ntHeader.OptionalHeader.SizeOfHeapReserve = 0x100000;
  this->ntHeader.OptionalHeader.SizeOfHeapCommit = 0x1000;

  // "Reserved, must be zero."
  this->ntHeader.OptionalHeader.LoaderFlags = 0;

  // Fixed value (in this case), may be changed later
  this->ntHeader.OptionalHeader.NumberOfRvaAndSizes = 0x10;
  memset(this->ntHeader.OptionalHeader.DataDirectory, 0,
         sizeof(IMAGE_DATA_DIRECTORY) * IMAGE_NUMBEROF_DIRECTORY_ENTRIES);

  this->imports_ready = false;
}

PE::~PE() {
  for (size_t i = 0; i < this->sections.size(); i++) {
    delete[] this->sections[i]->data;
    delete this->sections[i];
  }
  delete[] this->dosHeaderInfo.code;
}

void PE::load(std::string name) {
  FILE* f = fopen(name.c_str(), "rb");
  fread(&this->dosHeaderInfo.dosHeader, 1,
        sizeof(this->dosHeaderInfo.dosHeader), f);

  this->dosHeaderInfo.code_size = this->dosHeaderInfo.dosHeader.e_lfanew -
                                  sizeof(this->dosHeaderInfo.dosHeader);
  this->dosHeaderInfo.code = new BYTE[this->dosHeaderInfo.code_size];
  fread(this->dosHeaderInfo.code, 1, this->dosHeaderInfo.code_size, f);

  fseek(f, this->dosHeaderInfo.dosHeader.e_lfanew, 0);
  fread(&this->ntHeader, 1, sizeof(this->ntHeader), f);

  int cur_pos = 0;
  for (size_t i = 0; i < this->ntHeader.FileHeader.NumberOfSections; i++) {
    Section* tmp = new Section;
    fread(&tmp->header, 1, sizeof(tmp->header), f);
    cur_pos = ftell(f);

    // jump to content of section
    fseek(f, tmp->header.PointerToRawData, 0);

    tmp->data = new BYTE[tmp->header.SizeOfRawData];
    fread(tmp->data, 1, tmp->header.SizeOfRawData, f);
    sections.push_back(tmp);

    fseek(f, cur_pos, 0);  // jump back to headers
  }

  fclose(f);
}

size_t PE::align_file(size_t position) {
  if (position == 0) {
    return FILE_ALIGNMENT;
  }
  return (FILE_ALIGNMENT - (position - 1) % FILE_ALIGNMENT) + position - 1;
}

size_t PE::align_memory(size_t position) {
  if (position == 0) {
    return MEMORY_ALIGNMENT;
  }
  return (MEMORY_ALIGNMENT - (position - 1) % MEMORY_ALIGNMENT) + position - 1;
}

void PE::add_section(std::string sectionName, BYTE* data, size_t dataSize) {
  this->ntHeader.FileHeader.NumberOfSections++;
  Section* sec = new Section;
  memset(&sec->header, 0, sizeof(IMAGE_SECTION_HEADER));

  memcpy(sec->header.Name, sectionName.c_str(), sectionName.size() + 1);
  sec->header.Misc.VirtualSize = dataSize;

  if (this->sections.size() == 0) {  // there are no sections yet
    size_t offset = dosHeaderInfo.dosHeader.e_lfanew +
                    sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER);

    sec->header.VirtualAddress = 0x1000;  // warning
    sec->header.SizeOfRawData = this->align_file(dataSize);
    sec->header.PointerToRawData = this->align_file(offset);
  } else {
    // todo
    sec->header.VirtualAddress =
        this->sections[this->sections.size() - 1]->header.VirtualAddress +
        0x1000;
    sec->header.SizeOfRawData = dataSize;
    sec->header.PointerToRawData =
        this->sections[this->sections.size() - 1]->header.PointerToRawData +
        this->sections[this->sections.size() - 1]->header.SizeOfRawData;
  }

  sec->header.Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                IMAGE_SCN_CNT_CODE |
                                IMAGE_SCN_CNT_INITIALIZED_DATA;

  sec->data = new BYTE[sec->header.SizeOfRawData];
  memset(sec->data, 0, sec->header.SizeOfRawData);
  memcpy(sec->data, data, dataSize);

  sections.push_back(sec);

  this->ntHeader.OptionalHeader.SizeOfImage +=
      this->align_memory(sec->header.Misc.VirtualSize);
  this->ntHeader.OptionalHeader.SizeOfHeaders += sizeof(IMAGE_SECTION_HEADER);

  this->fill_section_specific(this->sections.size() - 1);
}

void PE::fill_section_specific(size_t section_id) {
  size_t rva = this->sections[section_id]->header.VirtualAddress;
  size_t size_raw = this->sections[section_id]->header.SizeOfRawData;
  std::string section_name = (char*)this->sections[section_id]->header.Name;

  if (section_name == ".text") {
    this->ntHeader.OptionalHeader.SizeOfCode = this->align_file(size_raw);
    this->ntHeader.OptionalHeader.BaseOfCode = rva;
    this->ntHeader.OptionalHeader.AddressOfEntryPoint = rva;
  } else if (section_name == ".data") {
    this->ntHeader.OptionalHeader.SizeOfUninitializedData =
        this->align_file(size_raw);
    this->ntHeader.OptionalHeader.BaseOfData = rva;
  }
}

void PE::dump(std::string name) {
  this->filename = name;
  this->ntHeader.OptionalHeader.SizeOfHeaders =
      this->align_file(this->ntHeader.OptionalHeader.SizeOfHeaders);
  if (!this->imports_ready) {
    flush_imports();
  }

  FILE* f = fopen(this->filename.c_str(), "wb");

  // Fill IMAGE_DOS_HEADER struct
  fwrite((void*)&this->dosHeaderInfo.dosHeader, 1,
         sizeof(this->dosHeaderInfo.dosHeader), f);

  // Insert custom DOS program
  fwrite((void*)this->dosHeaderInfo.code, 1, this->dosHeaderInfo.code_size, f);

  // Fill IMAGE_NT_HEADERS struct
  fseek(f, this->dosHeaderInfo.dosHeader.e_lfanew, SEEK_SET);
  fwrite((void*)&this->ntHeader, 1, sizeof(this->ntHeader), f);

  // Loop through all secion headers and dump them
  for (size_t i = 0; i < this->ntHeader.FileHeader.NumberOfSections; i++) {
    fwrite((void*)&this->sections[i]->header, 1, sizeof(IMAGE_SECTION_HEADER),
           f);
  }

  for (size_t i = 0; i < this->ntHeader.FileHeader.NumberOfSections; i++) {
    fseek(f, this->sections[i]->header.PointerToRawData, SEEK_SET);
    // printf("%x\n", this->sections[i]->header.SizeOfRawData);
    fwrite((void*)this->sections[i]->data, 1,
           this->sections[i]->header.SizeOfRawData, f);
  }

  fclose(f);

  // fflush?
  // fill checksum field
  f = fopen(this->filename.c_str(), "r+b");

  size_t offset = dosHeaderInfo.dosHeader.e_lfanew + sizeof(IMAGE_NT_HEADERS) -
                  (32 + sizeof(this->ntHeader.OptionalHeader.DataDirectory));
  fseek(f, offset, SEEK_SET);

  size_t checksum = this->calc_checksum();
  fwrite((void*)&checksum, 1, sizeof(DWORD), f);
  fclose(f);
}

size_t PE::calc_checksum() const {
  DWORD headerSum, checkSum;
  MapFileAndCheckSumA((LPSTR)this->filename.c_str(), &headerSum, &checkSum);

  return checkSum;
}

void PE::import(std::string libname, std::string funcname) {
  size_t rva = 0x1000;
  size_t vsize = 0x100;
  size_t section_num = 0;

  if (this->sections.size()) {
    size_t import_section = this->get_section_by_name(".idata");
    if (import_section == 0xffff) {
      rva = this->sections[this->sections.size() - 1]->header.VirtualAddress +
            this->align_memory(this->sections[this->sections.size() - 1]
                                   ->header.Misc.VirtualSize);
      section_num = this->sections.size();
    } else
      rva = this->sections[import_section]->header.VirtualAddress;
  } else {
    section_num = 0;
    rva = 0x1000;
  }

  // Check if it is first time, then create import header
  if (!this->imports.size()) {
    this->ntHeader.FileHeader.NumberOfSections++;
    Section* importSec = new Section();
    strcpy((char*)importSec->header.Name, ".idata");
    importSec->header.VirtualAddress = rva;

    if (section_num)
      importSec->header.PointerToRawData =
          this->sections[this->sections.size() - 1]->header.PointerToRawData +
          this->sections[this->sections.size() - 1]->header.SizeOfRawData;
    else
      importSec->header.PointerToRawData = 0x200;
    importSec->header.Characteristics =
        IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |
        IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_WRITE;

    this->ntHeader.OptionalHeader.SizeOfImage +=
        this->align_memory(importSec->header.Misc.VirtualSize);
    this->ntHeader.OptionalHeader.SizeOfHeaders += sizeof(IMAGE_SECTION_HEADER);

    this->ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
        .VirtualAddress = rva;

    this->sections.push_back(importSec);
  }

  // Check if library was imported previously
  bool imported = false;
  size_t lib_id = 0;
  for (size_t i = 0; i < this->imports.size(); i++)
    if (this->imports[i]->libname == libname) {
      imported = true;
      lib_id = i;
      break;
    }

  if (!imported) {  // libraries
    // get rid of last empty element (but only if there is any)
    if (this->imports.size()) {
      this->imports.pop_back();
    }

    Import_library* tmp_import = new Import_library();
    memset(&tmp_import->importDirectoryTable, 0,
           sizeof(IMAGE_IMPORT_DESCRIPTOR));
    this->imports.push_back(tmp_import);

    // save library name
    memcpy(tmp_import->libname, libname.c_str(), libname.size() + 1);

    // add empty entry (end of list)
    Import_library* end_import = new Import_library();
    memset(&end_import->importDirectoryTable, 0,
           sizeof(IMAGE_IMPORT_DESCRIPTOR));
    this->imports.push_back(end_import);

    // current library index
    lib_id = this->imports.size() - 2;
  }

  // Check if function was imported previously
  imported = false;
  for (size_t i = 0; i < this->imports[lib_id]->importNameTable.size(); i++) {
    if (this->imports[lib_id]->importNameTable[i]->funcName == funcname) {
      imported = true;
      break;
    }
  }
  if (!imported) {  // functions
    // get rid of last empty element (but only if there is any)
    if (this->imports[lib_id]->importNameTable.size())
      this->imports[lib_id]->importNameTable.pop_back();

    Import_function* func = new Import_function();
    memcpy(func->funcName, funcname.c_str(), funcname.size() + 1);
    this->imports[lib_id]->importNameTable.push_back(func);

    // push blank entry
    Import_function* func_end = new Import_function();
    this->imports[lib_id]->importNameTable.push_back(func_end);
  }
}

void PE::alloc_section(std::string section_name, size_t section_size) {
  this->ntHeader.FileHeader.NumberOfSections++;
  Section* sec = new Section;
  memset(&sec->header, 0, sizeof(IMAGE_SECTION_HEADER));

  memcpy(sec->header.Name, section_name.c_str(), section_name.size() + 1);
  sec->header.Misc.VirtualSize = section_size;

  // if there are no sections
  if (this->sections.size() == 0) {
    size_t offset = dosHeaderInfo.dosHeader.e_lfanew +
                    sizeof(IMAGE_NT_HEADERS) + sizeof(IMAGE_SECTION_HEADER);

    sec->header.VirtualAddress = 0x1000;
    sec->header.PointerToRawData = this->align_file(offset);
  } else {
    sec->header.VirtualAddress =
        this->sections[this->sections.size() - 1]->header.VirtualAddress +
        0x1000;
    sec->header.PointerToRawData =
        this->sections[this->sections.size() - 1]->header.PointerToRawData +
        this->sections[this->sections.size() - 1]->header.SizeOfRawData;
  }
  sec->header.SizeOfRawData = this->align_file(section_size);

  sec->header.Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ |
                                IMAGE_SCN_CNT_CODE |
                                IMAGE_SCN_CNT_INITIALIZED_DATA;

  sec->data = new BYTE[sec->header.SizeOfRawData];
  memset(sec->data, 0, sec->header.SizeOfRawData);

  sections.push_back(sec);

  this->ntHeader.OptionalHeader.SizeOfImage +=
      this->align_memory(sec->header.Misc.VirtualSize);
  this->ntHeader.OptionalHeader.SizeOfHeaders += sizeof(IMAGE_SECTION_HEADER);

  this->fill_section_specific(this->sections.size() - 1);
}

size_t PE::write_section(std::string name, BYTE* data, size_t data_size) {
  size_t section_num = get_section_by_name(name);
  if (section_num == 0xffff) {
    return 1;
  }
  memcpy(this->sections[section_num]->data, (void*)data, data_size);
  return 0;
}

size_t PE::flush_imports(size_t address) {
  // If there are any imports, process them
  if (this->imports.size() > 0) {
    size_t import_section = this->get_section_by_name(".idata");
    size_t rva;
    if (address == 0) {
      rva = this->sections[import_section]->header.VirtualAddress;
    } else {
      rva = address;
    }
    // get some basic import informations
    size_t functions = this->imports.size() - 1;  // contains blank entries
    size_t functions_length = 0;
    size_t libraries_length = 0;
    for (size_t i = 0; i < this->imports.size() - 1; i++) {
      libraries_length += strlen(this->imports[i]->libname) + 1;
      for (size_t j = 0; j < this->imports[i]->importNameTable.size() - 1;
           j++) {
        functions_length +=
            sizeof(WORD) +
            strlen(this->imports[i]->importNameTable[j]->funcName) + 1;
        functions++;
      }
    }

    // Import Name Table rva
    const size_t importNameTable =
        rva + this->imports.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR);

    // dll name rva
    const size_t nameRVA =
        rva + this->imports.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR) +
        2 * functions * sizeof(DWORD) + functions_length;

    // IAT rva
    const size_t IAT = rva +
                       this->imports.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR) +
                       functions * sizeof(DWORD);

    // helpful variables used only in incoming loop
    size_t importNameTable_var = importNameTable;
    size_t nameRVA_var = nameRVA;
    size_t IAT_var = IAT;

    // update Import Directory Table entries
    for (size_t i = 0; i < this->imports.size() - 1; i++) {
      this->imports[i]->importDirectoryTable.OriginalFirstThunk =
          importNameTable_var;
      importNameTable_var +=
          this->imports[i]->importNameTable.size() * sizeof(DWORD);
      this->imports[i]->importDirectoryTable.Name = nameRVA_var;
      nameRVA_var += strlen(this->imports[i]->libname) + 1;

      this->imports[i]->importDirectoryTable.FirstThunk = IAT_var;
      IAT_var += this->imports[i]->importNameTable.size() * sizeof(DWORD);
    }

    // Fill directories
    this->ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]
        .VirtualAddress = IAT;
    // at this place, IAT_var point at the end of IAT
    this->ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]
        .Size = IAT_var - IAT;

    // alloc memory and map values into section->data
    size_t complete_size =
        this->imports.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR) +
        2 * functions * sizeof(DWORD) + functions_length + libraries_length;

    // fill fields that required complete import size
    this->sections[import_section]->header.SizeOfRawData =
        this->align_file(complete_size);
    this->sections[import_section]->header.Misc.VirtualSize = complete_size;
    this->ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
        .Size = complete_size;

    this->sections[import_section]->data =
        new BYTE[this->align_file(complete_size)];
    memset(this->sections[import_section]->data, 0,
           this->align_file(complete_size));

    // copy all IDT entries
    size_t ptr = 0;
    for (size_t i = 0; i < this->imports.size(); i++) {
      memcpy(this->sections[import_section]->data + ptr,
             (void*)&this->imports[i]->importDirectoryTable,
             sizeof(IMAGE_IMPORT_DESCRIPTOR));
      ptr += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    // rva of first function name
    const size_t func_names_offset =
        rva + this->imports.size() * sizeof(IMAGE_IMPORT_DESCRIPTOR) +
        2 * functions * sizeof(DWORD);

    // Update (entry field) and Map IAT & INT entries
    for (size_t a = 0; a < 2; a++) {  // 2 means INT and IAT
      size_t names_offset_var = func_names_offset;
      // 'this->imports.size()-1' because we ommit blank IDT
      for (size_t i = 0; i < this->imports.size() - 1; i++) {
        for (size_t j = 0; j < this->imports[i]->importNameTable.size() - 1;
             j++) {
          this->imports[i]->importNameTable[j]->entry = names_offset_var;
          names_offset_var +=
              sizeof(WORD) +
              strlen(this->imports[i]->importNameTable[j]->funcName) + 1;

          memcpy(this->sections[import_section]->data + ptr,
                 (void*)&this->imports[i]->importNameTable[j]->entry,
                 sizeof(DWORD));
          ptr += sizeof(DWORD);
        }
        // add blank entry at the end of every dll's INT
        memcpy(
            this->sections[import_section]->data + ptr,
            (void*)&this->imports[i]
                ->importNameTable[this->imports[i]->importNameTable.size() - 1]
                ->entry,
            sizeof(DWORD));
        ptr += sizeof(DWORD);
      }
    }

    // Import Hints/Names
    for (size_t i = 0; i < this->imports.size() - 1; i++) {
      for (size_t j = 0; j < this->imports[i]->importNameTable.size() - 1;
           j++) {
        memcpy(this->sections[import_section]->data + ptr,
               (void*)&this->imports[i]->importNameTable[j]->hint,
               sizeof(WORD));
        ptr += sizeof(WORD);

        memcpy(this->sections[import_section]->data + ptr,
               this->imports[i]->importNameTable[j]->funcName,
               strlen(this->imports[i]->importNameTable[j]->funcName) + 1);
        ptr += strlen(this->imports[i]->importNameTable[j]->funcName) + 1;
      }
    }

    // DLL Names
    for (size_t i = 0; i < this->imports.size(); i++) {
      memcpy(this->sections[import_section]->data + ptr,
             this->imports[i]->libname, strlen(this->imports[i]->libname) + 1);
      ptr += strlen(this->imports[i]->libname) + 1;
    }
    this->imports_ready = true;
    return 0;  // success
  }
  return 1;  // failure
}

// Use only after PE::flush_imports!!!
size_t PE::get_iat_rv(std::string library, std::string funcname) const {
  size_t lib_index = 0;
  size_t address = 0xffffffff;
  for (size_t i = 0; i < this->imports.size(); i++) {
    if (strcmp(this->imports[i]->libname, library.c_str()) == 0) {
      lib_index = i;
      break;
    }
  }
  for (size_t i = 0; i < this->imports[lib_index]->importNameTable.size();
       i++) {
    if (strcmp(this->imports[lib_index]->importNameTable[i]->funcName,
               funcname.c_str()) == 0) {
      address = this->imports[lib_index]->importDirectoryTable.FirstThunk +
                i * sizeof(DWORD);
      break;
    }
  }

  return (address + this->ntHeader.OptionalHeader.ImageBase);
}

/*
void
PE::resize_section(std::string section_name, size_t new_size)
{
        new_size = this->align_file(new_size);

        size_t section_id = this->get_section_by_name(section_name);
        size_t raw_data_size = this->sections[section_id]->header.SizeOfRawData;
        BYTE *data = new BYTE [raw_data_size];
        memset(data, 0, raw_data_size);
        memcpy(data, this->sections[section_id]->data, raw_data_size);

        delete [] this->sections[section_id]->data;

        this->sections[section_id]->data = new BYTE [new_size];
        memset(this->sections[section_id]->data, 0, new_size);
        memcpy(this->sections[section_id]->data, data, new_size);

        delete [] data;

        // set new header values
        this->ntHeader.OptionalHeader.SizeOfImage -=
this->align_memory(this->sections[section_id]->header.SizeOfRawData);
        this->ntHeader.OptionalHeader.SizeOfImage +=
this->align_memory(new_size); this->sections[section_id]->header.SizeOfRawData =
new_size;

        this->sections[section_id]->header.Misc.VirtualSize = new_size;
        if(section_name == ".text")
                this->ntHeader.OptionalHeader.SizeOfCode =
this->align_memory(new_size);

        for(size_t i=section_id; i<this->sections.size()-1; i++)
        {
                size_t pointer_to_data =
this->sections[i]->header.PointerToRawData +
this->sections[i]->header.SizeOfRawData; size_t rva =
this->align_memory(this->sections[i]->header.VirtualAddress +
this->sections[i]->header.Misc.VirtualSize);

                this->sections[i+1]->header.PointerToRawData = pointer_to_data;
                this->sections[i+1]->header.VirtualAddress = rva;
                if(strcmp( (char*)this->sections[i+1]->header.Name, ".idata") ==
0)
                {
                        this->ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress
= rva; this->flush_imports(rva);
                }
        }
}
*/

WORD PE::RVAtoSection(DWORD rva) {
  size_t i;
  for (i = 0; i < this->sections.size(); i++) {
    if (this->sections[i]->header.VirtualAddress <= rva &&
        this->align_memory(this->sections[i]->header.SizeOfRawData) +
                this->sections[i]->header.VirtualAddress >
            rva)
      return i;
  }

  return 0xffff;
}

WORD PE::get_section_by_name(std::string section_name) const {
  for (size_t i = 0; i < this->sections.size(); i++) {
    if (!strcmp((char*)this->sections[i]->header.Name, section_name.c_str()))
      return i;
  }
  return 0xffff;
}

Import_library::Import_library() { memset(this->libname, 0, 256); }

Import_library::~Import_library() {
  for (size_t i = 0; i < this->importNameTable.size(); i++) {
    delete &this->importNameTable[i];
  }
}

Import_function::Import_function() {
  this->entry = 0;

  // not necessary what value is here because importing by name
  this->hint = 0xff;
  memset(&this->funcName, 0, sizeof(this->funcName));
}
