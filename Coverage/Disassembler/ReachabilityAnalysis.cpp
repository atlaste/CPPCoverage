#include "ReachabilityAnalysis.h"

#include "../Util.h"
#include <iostream>
#include "X86DisassemblerDecoder.h"
#include <cstdint>

#define GET_INSTRINFO_ENUM
#include "X86GenInstrInfo.inc"

struct Helper
{
	static int ByteReader(const struct reader_info *arg, uint8_t* byte, uint64_t address)
	{
		if (address > arg->size) { return -1; }

		*byte = arg->code[address];
		return 0;
	}

	static int ProcessReader(const struct reader_info *arg, uint8_t* byte, uint64_t address)
	{
		if (address > 16) { return -1; } // oops

		auto ptr = reinterpret_cast<HANDLE>(arg->code);
		
		BYTE result;
		SIZE_T numberBytesRead;
		if (!ReadProcessMemory(ptr, reinterpret_cast<LPVOID>(address + arg->offset), &result, 1, &numberBytesRead))
		{
			auto err = Util::GetLastErrorAsString();
			std::cout << "Error reading memory from target process: " << err << std::endl;
			return -1;
		}

		*byte = result;
		return 0;
	}

	static void HandleJump(InternalInstruction& instr, uint64_t address, uint8_t* state, size_t numberBytes)
	{
		auto immediate = instr.immediates[0];
		auto type = instr.operands[0].type;
		bool valid = true;

		auto enc = instr.operands[0].encoding;
		if (enc == ENCODING_IB || enc == ENCODING_IW || enc == ENCODING_ID || enc == ENCODING_IO || enc == ENCODING_Iv || enc == ENCODING_Ia)
		{
			switch (type)
			{
				case TYPE_RELv:
				{
					//isBranch = true;
					//pcrel = insn->startLocation + insn->immediateOffset + insn->immediateSize;
					switch (instr.displacementSize)
					{
						case 1:
							if (immediate & 0x80)
								immediate |= ~(0xffull);
							break;
						case 2:
							if (immediate & 0x8000)
								immediate |= ~(0xffffull);
							break;
						case 4:
							if (immediate & 0x80000000)
								immediate |= ~(0xffffffffull);
							break;
						case 8:
							break;
						default:
							break;
					}
				}
				break;

				case TYPE_IMM8:
				case TYPE_IMM16:
				case TYPE_IMM32:
				case TYPE_IMM64:
				case TYPE_IMMv:
				{
					switch (instr.operands[0].encoding)
					{
						case ENCODING_IB:
							if (immediate & 0x80)
							{
								immediate |= ~(0xffull);
							}
							break;
						case ENCODING_IW:
							if (immediate & 0x8000)
								immediate |= ~(0xffffull);
							break;
						case ENCODING_ID:
							if (immediate & 0x80000000)
								immediate |= ~(0xffffffffull);
							break;

					}
				}
				break;

				case TYPE_REL8:
				{
					if (immediate & 0x80)
						immediate |= ~(0xffull);
				}
				break;

				case TYPE_REL32:
				case TYPE_REL64:
				{
					if (immediate & 0x80000000)
						immediate |= ~(0xffffffffull);
				}
				break;
			}

			// Great, we have the immediate argument. Calculate target address:
			auto targetAddr = static_cast<int64_t>(immediate) + address;
			if (targetAddr < numberBytes)
			{
				state[targetAddr] |= 1;
			}
		}
		// Otherwise we cannot handle it...
	}

	static void HandleTerminator(InternalInstruction& instr, uint64_t address, uint8_t* state, size_t numberBytes)
	{
		if (address < numberBytes)
		{
			state[address] |= 2;
		}
	}

	static void MarkReachable(uint8_t* state, size_t numberBytes)
	{
		auto current = 0x10; // reachable

		for (size_t i = 0; i < numberBytes; ++i)
		{
			switch (state[i])
			{
				case 0:
					state[i] |= current;
					break;
				case 1: // branch target
					current = 0x10;
					state[i] |= 0x10;
					break;
				case 2: // terminator
					state[i] |= current;
					current = 0;
					break;
				case 3: // branch target | terminator
					state[i] |= 0x10;
					current = 0;
					break;
			}
		}
	}
};

ReachabilityAnalysis::ReachabilityAnalysis(HANDLE processHandle, DWORD64 methodStart, SIZE_T numberBytes) :
	state(0),
	methodStart(methodStart),
	numberBytes(numberBytes)
{
	if (numberBytes == 0) { return; }

	auto data = new uint8_t[numberBytes];
	SIZE_T numberBytesRead;
	if (!ReadProcessMemory(processHandle, reinterpret_cast<LPVOID>(methodStart), data, numberBytes, &numberBytesRead))
	{
		auto err = Util::GetLastErrorAsString();
		std::cout << "Error while reading symbol: " << err << std::endl;

		delete[] data;
		return;
	}

	// Initialize the disassembler reader
	reader_info reader;
	reader.code = data;
	reader.offset = 0;
	reader.size = numberBytes;

	// Address is the start offset to read the instruction:
	uint64_t address = 0;
	DWORD64 baseAddress = methodStart;

	// Initialize state to 0
	state = new uint8_t[numberBytes];
	memset(state, 0, numberBytes);

	InternalInstruction instr;
	memset(&instr, 0, offsetof(InternalInstruction, reader));

	while (address < numberBytes)
	{
#if _WIN64
		auto ret = decodeInstruction(&instr, Helper::ByteReader, &reader, address, DisassemblerMode::MODE_64BIT);
#else
		auto ret = decodeInstruction(&instr, Helper::ByteReader, &reader, address, DisassemblerMode::MODE_32BIT);
#endif

		size_t size;
		if (ret)
		{
			size = (uint16_t)(instr.readerCursor - address);
		}
		else
		{
			size = (uint16_t)instr.length;
		}

		if (instr.operandSize > 0)
		{
			// Handle instructions:
			// - If it's a branch, we basically want to know the destination address, and mark the target with a 1.
			// - If it's a terminator (e.g. ret, unconditional branch), we want to mark the source with a 2.
			// - If it's something else, we don't care.

			switch (instr.instructionID)
			{
				// FarJmp, SjLj omitted.
				// j*
				// TAILJMP
				// Ret, iret

				case X86_FARJMP16i:
				case X86_FARJMP16m:
				case X86_FARJMP32i:
				case X86_FARJMP32m:
				case X86_FARJMP64:
				case X86_JMP16m:
				case X86_JMP16r:
				case X86_JMP32m:
				case X86_JMP32r:
				case X86_JMP64m:
				case X86_JMP64r:
					Helper::HandleJump(instr, baseAddress + address, state, numberBytes);
					Helper::HandleTerminator(instr, baseAddress + address, state, numberBytes);
					break;

				case X86_JAE_1:
				case X86_JA_1:
				case X86_JBE_1:
				case X86_JB_1:
				case X86_JE_1:
				case X86_JGE_1:
				case X86_JG_1:
				case X86_JLE_1:
				case X86_JL_1:
				case X86_JMP_1:
				case X86_JNE_1:
				case X86_JNO_1:
				case X86_JNP_1:
				case X86_JNS_1:
				case X86_JO_1:
				case X86_JP_1:
				case X86_JS_1:
				case X86_JCXZ:
				case X86_JRCXZ:
				case X86_JAE_2:
				case X86_JA_2:
				case X86_JB_2:
				case X86_JBE_2:
				case X86_JE_2:
				case X86_JGE_2:
				case X86_JG_2:
				case X86_JLE_2:
				case X86_JL_2:
				case X86_JMP_2:
				case X86_JNE_2:
				case X86_JNO_2:
				case X86_JNP_2:
				case X86_JNS_2:
				case X86_JO_2:
				case X86_JP_2:
				case X86_JS_2:
				case X86_JAE_4:
				case X86_JA_4:
				case X86_JBE_4:
				case X86_JB_4:
				case X86_JE_4:
				case X86_JGE_4:
				case X86_JG_4:
				case X86_JLE_4:
				case X86_JL_4:
				case X86_JMP_4:
				case X86_JNE_4:
				case X86_JNO_4:
				case X86_JNP_4:
				case X86_JNS_4:
				case X86_JO_4:
				case X86_JP_4:
				case X86_JS_4:
				case X86_JECXZ_32:
				case X86_JECXZ_64:
					Helper::HandleJump(instr, baseAddress + address, state, numberBytes);
					break;

				case X86_RETIL:
				case X86_RETIQ:
				case X86_RETIW:
				case X86_RETL:
				case X86_RETQ:
				case X86_RETW:
					Helper::HandleTerminator(instr, baseAddress + address, state, numberBytes);
					break;
			}
		}

		address += size;
	}

	Helper::MarkReachable(state, numberBytes);

	delete[] data;
}

size_t ReachabilityAnalysis::FirstInstructionSize(HANDLE processHandle, DWORD64 methodStart)
{
	// Initialize the disassembler reader
	reader_info reader;
	reader.code = reinterpret_cast<uint8_t*>(processHandle);
	reader.offset = methodStart;
	reader.size = 16; // not used

	// Address is the start offset to read the instruction:
	uint64_t address = 0;

	// Initialize state to 0
	InternalInstruction instr;
	memset(&instr, 0, offsetof(InternalInstruction, reader));

#if _WIN64
	auto ret = decodeInstruction(&instr, Helper::ProcessReader, &reader, address, DisassemblerMode::MODE_64BIT);
#else
	auto ret = decodeInstruction(&instr, Helper::ProcessReader, &reader, address, DisassemblerMode::MODE_32BIT);
#endif

	size_t size;
	if (ret)
	{
		size = (uint16_t)(instr.readerCursor - address);
	}
	else
	{
		size = (uint16_t)instr.length;
	}
	return size;
}