#include <iostream>
#include <string>
#include <array>
#include <fstream>

/* Custom datatypes. */
using u8 = unsigned char;	
using u16 = char16_t; 		
using u32 = char32_t;		

/* Helper-functions for bit-masking. */
const u8 msbyte(const u16 word){ return (word >> 0x8); };	// Get most significant byte.
const u8 lsbyte(const u16 word){ return (word & 0xFF); };	// Get least significant byte.

/* Settings. */
const unsigned MEMORY_SIZE = 0x10000;

/* CPU. */
class CPU
{
	/* Eight general purpose 8-bit registers that can be used together as 16-bit pairs. */
	u16 _af, _bc, _de, _hl; 

	u16 _sp;	// Stack-pointer.
	u16 _pc;	// Program-counter.

private:

	/* Flag-handling. */
	enum{ Flag_Z = 0x80, Flag_N = 0x40, Flag_H = 0x20, Flag_C = 0x10 };
	void set_flags(const u8 flags){ _af |= flags; }
	void reset_flags(const u8 flags){ _af &= ~flags; }

public:
	CPU()
		: _af{ 0 }, _bc{ 0 }, _de{ 0 }, _hl{ 0 }, _sp{ 0 }, _pc{ 0 }
	{
	}

	/* Process an instruction.  http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf */
	void process_instruction(u8(&memory)[MEMORY_SIZE])
	{
		/*
				[prefix byte,]  opcode  [,displacement byte]  [,immediate data]
											- OR -
						two prefix bytes,  displacement byte,  opcode

				Look here: http://www.z80.info/decoding.htm#cb
		*/

		/* Split opcode in its parts. */
		const u8 x = memory[_pc] >> 0x6;			// Bits: 76------
		const u8 y = (memory[_pc] >> 0x3) & 0x7 ;	// Bits: --543---
		const u8 z = memory[_pc] & 0x7;				// Bits: -----210
		const u8 p = y >> 0x1;						// Bits: --54----
		const u8 q = y %  0x2;						// Bits: ----3---

		/*
			The following placeholders for instructions and operands are used:

			d		= displacement byte (8-bit signed integer)
			n		= 8-bit immediate operand (unsigned integer)
			nn		= 16-bit immediate operand (unsigned integer)
			tab[x]	= whatever is contained in the table named tab at index x (analogous for y and z and other table names) 
		*/		

		// Example: 
		// LD r[y], n
		switch (x)
		{
		case 0x00: // X=0
			switch (z)
			{
			case 0x01:	// Z=1
				if (y == 0x6) //LD sp,d16 =>  OPCODE 31 fe ff => LD sp, $fffe
					_sp = memory[++_pc] | (memory[++_pc] << 0x8);
				break;
			default: break;
			}
			break;
		default: break;
		}


		/* Fetch instruction-byte and process. */
		switch (memory[_pc])
		{
			/* 
				LD 
			*/
		case 0x06:	// LD B, n		=> Put value from register B into n.
			memory[++_pc] = msbyte(_bc); 
			break;
		case 0x0E:	// LD C, n		=> Put value from register C into n.
			memory[++_pc] = lsbyte(_bc); 
			break;
		case 0x21:	// LD HL, nn	=>	Put value from nn in (HL).
			_hl = memory[++_pc] | (memory[++_pc] << 0x8); 
			break;
		case 0x31:	// LD SP, nn	=>	Put value from nn in stack-pointer(SP).
			_sp = memory[++_pc] | (memory[++_pc] << 0x8); 
			break;	
		case 0x32:	// LD HL, A	=>	Put A into memory address HL. Decrement HL.
			_hl = msbyte(_af) - 1;
			break;
			/* 
				XOR n  
				Flags: Z if result = 0, N reset, H reset, C reset.
			*/
		case 0xAF:	// XOR n	=>	Logical exclusive OR n with register A, result in A.
			_af = ((msbyte(_af) ^ msbyte(_af)) << 0x8) + (lsbyte(_af));	
			reset_flags(Flag_N | Flag_H | Flag_C);
			if (!msbyte(_af)) set_flags(Flag_Z);
			break;
			/*
				BIT b, r
				Test bit b in register r.
				Flags: Z if bit of r = 0, N reset, H set
			*/
		case 0xCB:
			switch (memory[++_pc])
			{
			case 0x7C:	//BIT 7,H ( how know that should use'BIT'-instruction)
				if (memory[_pc]) set_flags(Flag_Z | Flag_H);
				else set_flags(Flag_H);
				reset_flags(Flag_N);
				break;
			default:	break;
			}
			break;
		// TODO: expand..
		default: 
			std::cerr << "Unknown opcode!" << std::endl;
			break;
		}

		++_pc;
	}
};

class GameBoy
{
	CPU			_cpu;			// Processor.
	std::string _rom_file;		// Rom-file data.
	u8 _memory[MEMORY_SIZE];	// Memory ($0000 - $FFFF). 

private:
	/* Read bytes from a rom-file. */
	std::string read_rom(const std::string path, const unsigned stream_size) 
	{
		char* rom_buffer = new char[stream_size];
		std::ifstream ifs{ path, std::ios::binary | std::ios::in };
		if (ifs.is_open())
		{
			ifs.read(rom_buffer, stream_size);
		}
		else std::cerr << "Unable to open file: " << path << std::endl;
		return std::string{ rom_buffer, stream_size };
	}

public:
	GameBoy()
		: _rom_file("unknown")
	{
		// ...
	}

	void power_on(const std::string bootstrap_path, std::string rom_path)
	{
		_cpu = CPU();							// Reset cpu.
		_rom_file = read_rom(rom_path, 0x8000);	// Read rom-file.	

		/* Copy bootstrap-rom to memory ($0000-$0100). */
		std::string bootstrap = read_rom(bootstrap_path, 0x100);	
		for (size_t i = 0x0; i < bootstrap.size(); ++i)
		{
			_memory[i] = static_cast<u8>(bootstrap[i]);
		}

		/* Copy cartridge-rom to memory. Internal information at $0100 - $014F. */
		for (size_t i = 0x0; i < 0x8000; ++i) 
		{
			_memory[0x100 + i] = _rom_file[i];
		}
	}

	void update()
	{
		_cpu.process_instruction(_memory);
	}
};


int main()
{
	// Test emulator.
	GameBoy gameboy;
	gameboy.power_on("_roms/DMG_ROM.bin", "_roms/Tetris (World).gb");
	for(int i=0; i<10;++i) gameboy.update(); // TODO: process more instructions

	return EXIT_SUCCESS;
}
