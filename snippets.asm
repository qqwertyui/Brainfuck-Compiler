void Compiler::init_stack_frame(uint frame_size)
	push ebp
	mov ebp, esp
	sub esp, memory_size
	mov eax, esp
	mov ebx, eax
	push memory_size
	push 0
	push eax
	call [iat_memset_addr]
	add esp, 12
	mov ecx, 0
	mov eax, 0


void Compiler::fini_stack_frame()
  mov esp, ebp
  pop ebp
  ret


void Compiler::add_ptr(uint number)
  add ecx, number


void Compiler::sub_ptr(uint number)
  sub ecx, number


void Compiler::add(uint number)
  add [ebx+ecx], number


void Compiler::sub(uint number)
  sub [ebx+ecx], number


void Compiler::write()
  push ecx
  mov eax, [iat_streams]
  add eax, 0x20 ; offset to stdout
  push eax
	mov eax, 0
  mov eax, [ebx+ecx]
  push eax
  call [putc]
	add esp, 8
	pop ecx


void Compiler::read()
  push ecx
  push stdin_addr
  call [iat_getc_addr]
  add esp, 4
  pop ecx
  mov eax, [ebx+ecx]


void Compiler::loop_begin()
  mov eax, 0
  mov al, [ecx+ebx]
  cmp eax, 0
