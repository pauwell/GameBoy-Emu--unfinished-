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
const unsigned SCREEN_WIDTH			= 160,	// Width of screen  (20 tiles) 
			   SCREEN_HEIGHT		= 144,	// Height of screen (18 tiles)
			   MAX_SPRITES			= 40,	// Maximum number of sprites.
			   MAX_SPRITES_PER_LINE = 10,	// Maximum number of sprites per line.
			   MAX_SPRITE_WIDTH		= 8,	// Maximum sprite-width.
			   MAX_SPRITE_HEIGHT	= 16,	// Maximum sprite-height.
			   MIN_SPRITE_WIDTH		= 8,	// Minimum sprite-width.
			   MIN_SPRITE_HEIGHT	= 8;	// Minimum sprite-height.
const float	   CLOCK_SPEED = 4.194304f;		// Speed of the internal clock.


/* CPU. */
class CPU
{
	/* Eight general purpose 8-bit registers that can be used together as 16-bit pairs. */
	u16 _af, _bc, _de, _hl; 

	u16 _sp;	// Stack-pointer
	u16 _pc;	// Program-counter

public:
	CPU(/*u8 (&memory)[MEMORY_SIZE]*/)
		: _af{ 0 }, _bc{ 0 }, _de{ 0 }, _hl{ 0 }, _sp{ 0 }, _pc{ 0 }
	{
	}

	/* Process an instruction. */
	void process_instruction(u8(&memory)[MEMORY_SIZE])
	{
		u8 opcode = (memory[_pc]);

		switch (opcode)
		{
		case 0x06:
			memory[++_pc] = msbyte(_bc); // LD B, n  => Put value from register B into n.
			break;
		case 0x0E:
			memory[++_pc] = lsbyte(_bc); // LD C, n  => Put value from register C into n.
			break;
		default: 
			std::cerr << "Unknown opcode!" << std::endl;
			break;
		}
	}
};

class GameBoy
{
	CPU			_cpu;
	std::string _rom;
	u8 _memory[MEMORY_SIZE]; // Memory ($0000 - $FFFF). 

private:
	/* Read bytes from a rom-file. */
	std::string read_rom(const std::string path, const unsigned stream_size) // TODO runtime size-check.
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
		: _rom("unknown")
	{
		// ...
	}

	void power_on(const std::string bootstrap_path, std::string rom_path)
	{
		_cpu = CPU();						// Reset cpu.
		_rom = read_rom(rom_path, 0x8000);	// Read rom-file.	

		/* Copy bootstrap-rom to memory ($0000-$0100) */
		auto bootstrap = read_rom(bootstrap_path, 0x100);	
		for (size_t i = 0x0; i < bootstrap.size(); ++i)
		{
			_memory[i] = static_cast<u8>(bootstrap[i]);
		}

		/* Copy cartridge-rom to memory  */
		for (size_t i = 0x0; i < 0x50; ++i) // Internal information($0100 - $014F)
		{
			_memory[0x100 + i] = _rom[i];
		}
		for (size_t i = 0x50; i < 0x8000; ++i) // Data
		{
			_memory[0x100 + i] = _rom[i];
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
	gameboy.update();

	return EXIT_SUCCESS;
}
