// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
#include "libc-main.h"
#include "screen.h"
#include "x86.h"
#include "vm.h"
#include "fs.h"

#define MAX_LABELS 256
#define MAX_CODE 4096
#define MAX_LINE 256

// Array of names for simple 32-bit x86 registers (EAX=0, ECX=1, EDX=2, EBX=3, ESP=4, EBP=5, ESI=6, EDI=7)
uint8_t *registerNames[] = {(uint8_t *)"eax", (uint8_t *)"ecx", (uint8_t *)"edx", (uint8_t *)"ebx", (uint8_t *)"esp", (uint8_t *)"ebp", (uint8_t *)"esi", (uint8_t *)"edi"};

// Structure to hold label information (name and address)
typedef struct {
    uint8_t name[32];
    int address;
    int section;
} LabelEntry;

// Structure for jump table entry
typedef struct {
    uint8_t *name;
    uint32_t func;
} JumpTableEntry;

// Global variables for managing labels, machine code, and assembly state
LabelEntry labels[MAX_LABELS];
int numLabels = 0;
unsigned char codeSectionBuffer[MAX_CODE];
int codeSectionLength = 0;
unsigned char dataSectionBuffer[MAX_CODE];
int dataSectionLength = 0;
int currentCodeAddress = 0;
uint32_t currentPrintRow = 0;
bool hasExternalLabel = false;
int currentSection = 0;
int codeSectionOffset = 0;
int dataSectionOffset = 0;

/**
 * Searches through the array of labels to find the index of a label with the given name.
 * This function iterates over all recorded labels and compares their names using string comparison.
 * If a match is found, the index is returned; otherwise, it indicates the label does not exist.
 * @param labelName The name of the label to search for.
 * @return The index of the label if found, or -1 if not found.
 */
int findLabelIndex(uint8_t *labelName)
{
    for (int index = 0; index < numLabels; index++)
    {
        if (strcmp(labels[index].name, labelName) == 0)
        {
            return index;
        }
    }
    return -1;
}

/**
 * Looks up the address of a library function from the shared jump table.
 * This function scans the jump table entries starting from a predefined memory location.
 * It compares the provided name with each entry's name until a match is found or the end is reached.
 * @param name The name of the library function to find.
 * @return The function address if found, or 0 if not found.
 */
uint32_t findLibFunctionAddr(uint8_t *name)
{
    JumpTableEntry *entry = (JumpTableEntry *)SHARED_LIBRARIES_START_LOC;
    while (entry->name != NULL)
    {
        if (strcmp((uint8_t *)entry->name, name) == 0)
        {
            return entry->func;
        }
        entry++;
    }
    return 0;
}

/**
 * Adds a new label entry to the labels array with the specified name, address, and section.
 * This function checks if there is space in the array before adding the label.
 * It copies the name, sets the address and section, increments the label count, and prints debug information.
 * @param labelName The name of the label to add.
 * @param labelAddress The address associated with the label.
 * @param labelSection The section (text or data) where the label is defined.
 */
void addLabel(uint8_t *labelName, int labelAddress, int labelSection)
{
    if (numLabels < MAX_LABELS)
    {
        strcpy(labels[numLabels].name, labelName);
        labels[numLabels].address = labelAddress;
        labels[numLabels].section = labelSection;
        numLabels++;
        currentPrintRow++;
        printString(COLOR_GREEN, currentPrintRow++, 10, (uint8_t*)"Label:");
        printString(COLOR_LIGHT_BLUE, currentPrintRow-1, 17, labelName);
    }
}

/**
 * Parses a register name string and returns its corresponding index in the registerNames array.
 * This function compares the input string against known register names (eax to edi).
 * @param registerString The string representing the register name.
 * @return The index of the register (0-7) if matched, or -1 if not a valid register.
 */
int parseRegisterIndex(uint8_t *registerString)
{
    for (int index = 0; index < 8; index++)
    {
        if (strcmp(registerString, registerNames[index]) == 0)
        {
            return index;
        }
    }
    return -1;
}

/**
 * Parses an immediate value from a string, supporting decimal and hexadecimal formats with optional negative sign.
 * This function handles signs, bases (10 or 16), and converts characters to digits accordingly.
 * It processes the string until a non-digit character is encountered.
 * @param valueString The string containing the immediate value.
 * @return The parsed integer value, including sign.
 */
int parseImmediateValue(uint8_t *valueString)
{
    int value = 0;
    int sign = 1;
    if (*valueString == '-')
    {
        sign = -1;
        valueString++;
    }
    int base = 10;
    if (*valueString == '0')
    {
        valueString++;
        if (*valueString == 'x' || *valueString == 'X')
        {
            base = 16;
            valueString++;
        }
        else
        {
            valueString--; // Revert if not hex
        }
    }
    while (*valueString)
    {
        unsigned char character = toLower(*valueString);
        int digit;
        if (character >= '0' && character <= '9')
        {
            digit = character - '0';
        }
        else if (base == 16 && character >= 'a' && character <= 'f')
        {
            digit = character - 'a' + 10;
        }
        else
        {
            break;
        }
        if (digit >= base) break;
        value = value * base + digit;
        valueString++;
    }
    return sign * value;
}

/**
 * Emits a single byte to the appropriate section buffer (code or data) based on the current section.
 * This function appends the byte to the buffer and increments the length or address accordingly.
 * It checks for buffer overflow before emitting.
 * @param byteValue The byte to emit.
 */
void emitByte(unsigned char byteValue)
{
    if (currentSection == 0) {
        if (codeSectionLength < MAX_CODE) {
            codeSectionBuffer[codeSectionLength++] = byteValue;
            currentCodeAddress++;
        }
    } else if (currentSection == 1) {
        if (dataSectionLength < MAX_CODE) {
            dataSectionBuffer[dataSectionLength++] = byteValue;
        }
    }
}

/**
 * Emits a 32-bit integer value to the buffer in little-endian byte order.
 * This function breaks down the dword into four bytes and emits each using emitByte.
 * @param dwordValue The 32-bit value to emit.
 */
void emitDword(int dwordValue)
{
    emitByte(dwordValue & 0xFF);
    emitByte((dwordValue >> 8) & 0xFF);
    emitByte((dwordValue >> 16) & 0xFF);
    emitByte((dwordValue >> 24) & 0xFF);
}

/**
 * Checks if the given argument string represents a memory operand, such as [reg+offset].
 * This function verifies if the string starts with '[' and ends with ']'.
 * @param arg The argument string to check.
 * @return 1 if it is a memory operand, 0 otherwise.
 */
int isMemoryOperand(uint8_t *arg)
{
    if (*arg != '[') return 0;
    uint8_t *p = arg;
    while (*p) p++;
    return *(p - 1) == ']';
}

/**
 * Parses a memory operand string like [ebp-4] to extract the base register index and offset.
 * This function skips the opening '[', identifies the base register (ebp or eax), parses the sign and offset.
 * @param arg The memory operand string.
 * @param base Pointer to store the base register index.
 * @param offset Pointer to store the parsed offset.
 */
void parseMemoryOperand(uint8_t *arg, int *base, int *offset)
{
    arg++; // Skip '['
    *base = -1;
    *offset = 0;
    if (toLower(*arg) == 'e' && toLower(*(arg+1)) == 'b' && toLower(*(arg+2)) == 'p')
    {
        *base = 5; // EBP index
        arg += 3;
    }
    else if (toLower(*arg) == 'e' && toLower(*(arg+1)) == 'a' && toLower(*(arg+2)) == 'x')
    {
        *base = 0; // EAX index
        arg += 3;
    }
    // Assume next is + or - or digit
    int sign = 1;
    if (*arg == '-') { sign = -1; arg++; }
    else if (*arg == '+') { arg++; }
    *offset = sign * parseImmediateValue(arg); // Parses until non-digit, assuming ] follows
}

/**
 * Determines the number of bytes required to represent the displacement in a memory operand.
 * This function decides based on the offset value and base register whether 0, 1, or 4 bytes are needed.
 * @param offset The displacement offset.
 * @param base The base register index.
 * @return The number of bytes for the displacement (0, 1, or 4).
 */
int getDispBytes(int offset, int base)
{
    if (offset == 0 && base != 5) return 0;
    if (offset >= -128 && offset <= 127) return 1;
    return 4;
}

/**
 * Determines the number of bytes required to represent an immediate value.
 * This function checks if the value fits in 1 byte (signed) or requires 4 bytes.
 * @param val The immediate value.
 * @return The number of bytes for the immediate (1 or 4).
 */
int getImmBytes(int val)
{
    if (val >= -128 && val <= 127) return 1;
    return 4;
}

/**
 * Parses a single line of assembly code to extract the instruction, arguments, and detect if it's a label.
 * This function handles labels (ending with ':'), instructions, and up to two arguments, supporting quoted strings.
 * It skips whitespace, copies substrings into buffers, and sets flags accordingly.
 * @param linePointer Pointer to the line string.
 * @param instructionBuffer Buffer to store the instruction.
 * @param arg1Buffer Buffer to store the first argument.
 * @param arg2Buffer Buffer to store the second argument.
 * @param isLabelFlag Pointer to flag indicating if a label was found.
 * @param labelNameBuffer Buffer to store the label name if present.
 */
void parseLine(uint8_t *linePointer, uint8_t *instructionBuffer, uint8_t *arg1Buffer, uint8_t *arg2Buffer, int *isLabelFlag, uint8_t *labelNameBuffer)
{
    *isLabelFlag = 0;
    instructionBuffer[0] = 0;
    arg1Buffer[0] = 0;
    arg2Buffer[0] = 0;
    labelNameBuffer[0] = 0;

    linePointer = skipWhitespace(linePointer);
    if (!*linePointer) return;

    uint8_t *startPointer = linePointer;
    while (*linePointer && *linePointer != ':' && !isSpace(*linePointer)) linePointer++;
    int length = linePointer - startPointer;

    if (*linePointer == ':')
    {
        *isLabelFlag = 1;
        if (length > 0 && length < 32)
        {
            bytecpy(labelNameBuffer, startPointer, length);
            labelNameBuffer[length] = 0;
        }
        linePointer = skipWhitespace(linePointer + 1);
        if (!*linePointer) return;

        // Parse instruction after the label
        startPointer = linePointer;
        while (*linePointer && !isSpace(*linePointer)) linePointer++;
        length = linePointer - startPointer;
        if (length > 0 && length < 16)
        {
            bytecpy(instructionBuffer, startPointer, length);
            instructionBuffer[length] = 0;
        }
    }
    else
    {
        // No label, so the first word is the instruction
        if (length > 0 && length < 16)
        {
            bytecpy(instructionBuffer, startPointer, length);
            instructionBuffer[length] = 0;
        }
        linePointer = skipWhitespace(linePointer);
    }

    if (!instructionBuffer[0]) return;

    linePointer = skipWhitespace(linePointer);
    if (!*linePointer) return;

    // Parse first argument
    startPointer = linePointer;
    bool quoted = (*linePointer == '\'');
    char quoteChar = quoted ? *linePointer : 0;
    if (quoted) linePointer++;
    while (*linePointer) {
        if (quoted) {
            if (*linePointer == quoteChar) {
                linePointer++;  // Move past closing quote
                break;
            }
        } else {
            if (*linePointer == ',' || isSpace(*linePointer)) break;
        }
        linePointer++;
    }
    length = linePointer - startPointer;
    if (length > 0 && length < 64) {
        bytecpy(arg1Buffer, startPointer, length);
        arg1Buffer[length] = 0;
    }
    if (!arg1Buffer[0]) return;

    // Parse second argument (after comma)
    linePointer = skipWhitespace(linePointer);
    if (*linePointer != ',') return;

    linePointer = skipWhitespace(linePointer + 1);
    if (!*linePointer) return;

    startPointer = linePointer;
    quoted = (*linePointer == '\'');
    quoteChar = quoted ? *linePointer : 0;
    if (quoted) linePointer++;
    while (*linePointer) {
        if (quoted) {
            if (*linePointer == quoteChar) {
                linePointer++;  // Move past closing quote
                break;
            }
        } else {
            if (isSpace(*linePointer)) break;
        }
        linePointer++;
    }
    length = linePointer - startPointer;
    if (length > 0 && length < 64) {
        bytecpy(arg2Buffer, startPointer, length);
        arg2Buffer[length] = 0;
    }
}

/**
 * Extracts the next line from the input buffer, skipping whitespace and handling line endings.
 * This function advances the input pointer to the next line and copies the current line into the buffer.
 * It limits the line length to prevent overflow.
 * @param inputPointer Pointer to the current position in the input buffer.
 * @param lineBuffer Buffer to store the extracted line.
 * @return 1 if a line was extracted, 0 if end of input or empty line.
 */
int getNextLine(uint8_t **inputPointer, uint8_t *lineBuffer)
{
    if (!*inputPointer || !**inputPointer) return 0;
    uint8_t *startPointer = *inputPointer;
    startPointer = skipWhitespace(startPointer);
    if (*startPointer == 0 || *startPointer == '\n')
    {
        *inputPointer = startPointer + (*startPointer == '\n' ? 1 : 0);
        return 0;
    }
    uint8_t *endPointer = startPointer;
    while (*endPointer && *endPointer != '\n') endPointer++;
    int length = endPointer - startPointer;
    if (length > MAX_LINE - 1) length = MAX_LINE - 1;
    bytecpy(lineBuffer, startPointer, length);
    lineBuffer[length] = 0;
    *inputPointer = endPointer + (*endPointer == '\n' ? 1 : 0);
    return 1;
}

/**
 * Calculates the byte size of an assembly instruction based on its opcode and operands.
 * This function handles various instructions like mov, add, sub, cmp, jumps, etc., 
 * determining size based on register, memory, or immediate operands.
 * @param instruction The instruction opcode string.
 * @param arg1 The first argument string.
 * @param arg2 The second argument string.
 * @return The size in bytes of the encoded instruction.
 */
int getInstructionSize(uint8_t *instruction, uint8_t *arg1, uint8_t *arg2)
{
    int size = 0;
    int reg1 = parseRegisterIndex(arg1);
    int reg2 = parseRegisterIndex(arg2);
    if (strcmp(instruction, (uint8_t *)"db") == 0)
    {
        int len = strlen(arg1) - 1;
        if (len < 0) len = 0;
        size = len;
    }
    else if (strcmp(instruction, (uint8_t *)"mov") == 0 ||
        strcmp(instruction, (uint8_t *)"add") == 0 ||
        strcmp(instruction, (uint8_t *)"sub") == 0)
    {
        if (reg1 != -1)
        {
            if (reg2 != -1)
            {
                size = 2; // reg, reg
            }
            else if (isMemoryOperand(arg2))
            {
                int base, offset;
                parseMemoryOperand(arg2, &base, &offset);
                int disp_bytes = getDispBytes(offset, base);
                size = 2 + disp_bytes; // mov reg, mem
            }
            else
            {
                int imm = parseImmediateValue(arg2);
                int imm_bytes = getImmBytes(imm);
                if (strcmp(instruction, (uint8_t *)"mov") == 0) {
                    size = 5; // mov reg, imm32 always 5 bytes
                } else {
                    size = (imm_bytes == 1 ? 3 : 6);
                }
            }
        }
        else if (isMemoryOperand(arg1))
        {
            int base, offset;
            parseMemoryOperand(arg1, &base, &offset);
            int disp_bytes = getDispBytes(offset, base);
            if (reg2 != -1)
            {
                size = 2 + disp_bytes; // mov mem, reg
            }
            else
            {
                int imm = parseImmediateValue(arg2);
                int imm_bytes = (strcmp(instruction, (uint8_t *)"mov") == 0) ? 4 : getImmBytes(imm);
                size = 2 + disp_bytes + imm_bytes; // mov mem, imm32: 2 + disp + 4
            }
        }
    }
    else if (strcmp(instruction, (uint8_t *)"cmp") == 0)
    {
        if (reg1 != -1 && reg2 != -1)
        {
            size = 2;
        }
        else if (reg1 != -1)
        {
            int imm = parseImmediateValue(arg2);
            int imm_bytes = getImmBytes(imm);
            size = (imm_bytes == 1 ? 3 : 6);
        }
        else if (isMemoryOperand(arg1))
        {
            int base, offset;
            parseMemoryOperand(arg1, &base, &offset);
            int disp_bytes = getDispBytes(offset, base);
            int imm = parseImmediateValue(arg2);
            int imm_bytes = getImmBytes(imm);
            size = 2 + disp_bytes + imm_bytes;
        }
    }
    else if (strcmp(instruction, (uint8_t *)"jmp") == 0)
    {
        if (arg1[0]) size = 5;
    }
    else if (strcmp(instruction, (uint8_t *)"call") == 0)
    {
        if (arg1[0])
        {
            int regIndex = parseRegisterIndex(arg1);
            if (regIndex != -1)
            {
                size = 2;
            }
            else
            {
                size = 5;
            }
        }
    }
    else if (strcmp(instruction, (uint8_t *)"jne") == 0 || strcmp(instruction, (uint8_t *)"jle") == 0 || strcmp(instruction, (uint8_t *)"jge") == 0)
    {
        if (arg1[0]) size = 6;
    }
    else if (strcmp(instruction, (uint8_t *)"int") == 0)
    {
        if (arg1[0]) size = 2;
    }
    else if (strcmp(instruction, (uint8_t *)"pusha") == 0 || strcmp(instruction, (uint8_t *)"popa") == 0)
    {
        size = 1;
    }
    else if (strcmp(instruction, (uint8_t *)"push") == 0 || strcmp(instruction, (uint8_t *)"pop") == 0)
    {
        size = 1;
    }
    else if (strcmp(instruction, (uint8_t *)"ret") == 0)
    {
        size = 1;
    }
    return size;
}

/**
 * Performs the first pass over the assembly input to collect labels and calculate section offsets.
 * This function processes each line, identifies labels and instructions, computes instruction sizes,
 * and updates offsets for code and data sections without generating machine code.
 * @param inputStartPointer Pointer to the start of the input assembly code.
 */
void firstPass(uint8_t *inputStartPointer)
{
    uint8_t *inputPointer = inputStartPointer;
    uint8_t lineBuffer[MAX_LINE];
    uint8_t instruction[16];
    uint8_t arg1[64];
    uint8_t arg2[64];
    uint8_t labelName[32];
    int isLabel;
    currentSection = 0;
    codeSectionOffset = 0;
    dataSectionOffset = 0;

    while (getNextLine(&inputPointer, lineBuffer))
    {
        uint8_t *linePointer = lineBuffer;
        if (*linePointer == ';' || *linePointer == '#') continue; // Skip comments

        parseLine(linePointer, instruction, arg1, arg2, &isLabel, labelName);
        if (isLabel)
        {
            int labelIndex = findLabelIndex(labelName);
            if (labelIndex == -1)
            {
                int labelAddress = (currentSection == 0) ? codeSectionOffset : dataSectionOffset;
                addLabel(labelName, labelAddress, currentSection);
            }
            if (instruction[0] == 0) continue;
        }
        if (instruction[0] == 0) continue;

        // Convert instruction to lowercase for comparison
        for (uint8_t *charPointer = instruction; *charPointer; charPointer++) *charPointer = toLower(*charPointer);

        if (strcmp(instruction, (uint8_t *)"section") == 0)
        {
            if (strcmp(arg1, (uint8_t *)".data") == 0)
            {
                currentSection = 1;
            }
            else if (strcmp(arg1, (uint8_t *)".text") == 0 || strcmp(arg1, (uint8_t *)".txt") == 0)
            {
                currentSection = 0;
            }
            continue;
        }

        int instructionSize = getInstructionSize(instruction, arg1, arg2);
        if (currentSection == 0)
        {
            codeSectionOffset += instructionSize;
        }
        else if (currentSection == 1)
        {
            dataSectionOffset += instructionSize;
        }
    }
}

/**
 * Performs the second pass over the assembly input to generate machine code using resolved labels.
 * This function processes each line again, emits opcodes and operands to buffers,
 * handles instructions like mov, add, jumps, calls, etc., resolving labels and library functions.
 * @param inputStartPointer Pointer to the start of the input assembly code.
 */
void secondPass(uint8_t *inputStartPointer)
{
    uint8_t *inputPointer = inputStartPointer;
    uint8_t lineBuffer[MAX_LINE];
    uint8_t instruction[16];
    uint8_t arg1[64];
    uint8_t arg2[64];
    uint8_t labelName[32];
    int isLabel;
    currentSection = 0;

    while (getNextLine(&inputPointer, lineBuffer))
    {
        uint8_t *linePointer = lineBuffer;
        if (*linePointer == ';' || *linePointer == '#') continue; // Skip comments

        parseLine(linePointer, instruction, arg1, arg2, &isLabel, labelName);
        if (isLabel)
        {
            if (instruction[0] == 0) continue;
        }
        if (instruction[0] == 0) continue;

        // Convert instruction and args to lowercase if needed (but args may be labels or regs)
        for (uint8_t *charPointer = instruction; *charPointer; charPointer++) *charPointer = toLower(*charPointer);

        if (strcmp(instruction, (uint8_t *)"section") == 0)
        {
            if (strcmp(arg1, (uint8_t *)".data") == 0)
            {
                currentSection = 1;
            }
            else if (strcmp(arg1, (uint8_t *)".text") == 0 || strcmp(arg1, (uint8_t *)".txt") == 0)
            {
                currentSection = 0;
            }
            continue;
        }

        if (strcmp(instruction, (uint8_t *)"db") == 0 && currentSection == 1 && arg1[0])
        {
            if (arg1[0] == '\'')
            {
                uint8_t *p = arg1 + 1;
                while (*p && *p != '\'')
                {
                    emitByte(*p);
                    p++;
                }

                emitByte('\0');
            }
            continue;
        }

        if (strcmp(instruction, (uint8_t *)"mov") == 0 && arg1[0] && arg2[0])
        {
            int reg1 = parseRegisterIndex(arg1);
            int reg2 = parseRegisterIndex(arg2);
            if (reg1 != -1)
            {
                if (reg2 != -1)
                {
                    emitByte(0x89); // mov reg1, reg2
                    emitByte(0xC0 | (reg2 << 3) | reg1);
                }
                else if (isMemoryOperand(arg2))
                {
                    int base, offset;
                    parseMemoryOperand(arg2, &base, &offset);
                    int disp_bytes = getDispBytes(offset, base);
                    int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                    emitByte(0x8B); // mov reg, mem
                    emitByte((mod << 6) | (reg1 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                }
                else
                {
                    int immediateValue = parseImmediateValue(arg2);
                    int labelIndex = findLabelIndex(arg2);
                    if (labelIndex != -1)
                    {
                        immediateValue = labels[labelIndex].address;
                        uint32_t base = (labels[labelIndex].section == 0) ? 0x00402000 : 0x00402000 + (((codeSectionOffset + 4096 - 1) / 4096) * 4096);
                        immediateValue += base;
                    }
                    else
                    {
                        uint32_t libAddr = findLibFunctionAddr(arg2);
                        if (libAddr != 0)
                        {
                            immediateValue = libAddr;
                        }
                    }
                    emitByte(0xB8 + reg1); // mov reg, imm32
                    emitDword(immediateValue);
                }
            }
            else if (isMemoryOperand(arg1))
            {
                int base, offset;
                parseMemoryOperand(arg1, &base, &offset);
                int disp_bytes = getDispBytes(offset, base);
                int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                if (reg2 != -1)
                {
                    emitByte(0x89); // mov mem, reg
                    emitByte((mod << 6) | (reg2 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                }
                else
                {
                    int immediateValue = parseImmediateValue(arg2);
                    int imm_bytes = getImmBytes(immediateValue);
                    if (imm_bytes == 1)
                    {
                        emitByte(0xC6);
                        emitByte((mod << 6) | base);
                    }
                    else
                    {
                        emitByte(0xC7);
                        emitByte((mod << 6) | base);
                    }
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    if (imm_bytes == 1) emitByte(immediateValue);
                    else emitDword(immediateValue);
                }
            }
        }
        else if (strcmp(instruction, (uint8_t *)"add") == 0 && arg1[0] && arg2[0])
        {
            int reg1 = parseRegisterIndex(arg1);
            int reg2 = parseRegisterIndex(arg2);
            if (reg1 != -1)
            {
                if (reg2 != -1)
                {
                    emitByte(0x01);
                    emitByte(0xC0 | (reg2 << 3) | reg1);
                }
                else if (isMemoryOperand(arg2))
                {
                    int base, offset;
                    parseMemoryOperand(arg2, &base, &offset);
                    int disp_bytes = getDispBytes(offset, base);
                    int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                    emitByte(0x03);
                    emitByte((mod << 6) | (reg1 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                }
                else
                {
                    int imm = parseImmediateValue(arg2);
                    int imm_bytes = getImmBytes(imm);
                    if (imm_bytes == 1)
                    {
                        emitByte(0x83);
                        emitByte(0xC0 + reg1);
                        emitByte(imm);
                    }
                    else
                    {
                        emitByte(0x81);
                        emitByte(0xC0 + reg1);
                        emitDword(imm);
                    }
                }
            }
            else if (isMemoryOperand(arg1))
            {
                int base, offset;
                parseMemoryOperand(arg1, &base, &offset);
                int disp_bytes = getDispBytes(offset, base);
                int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                int imm = parseImmediateValue(arg2);
                int imm_bytes = getImmBytes(imm);
                if (imm_bytes == 1)
                {
                    emitByte(0x83);
                    emitByte((mod << 6) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    emitByte(imm);
                }
                else
                {
                    emitByte(0x81);
                    emitByte((mod << 6) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    emitDword(imm);
                }
            }
        }
        else if (strcmp(instruction, (uint8_t *)"sub") == 0 && arg1[0] && arg2[0])
        {
            int reg1 = parseRegisterIndex(arg1);
            int reg2 = parseRegisterIndex(arg2);
            if (reg1 != -1)
            {
                if (reg2 != -1)
                {
                    emitByte(0x29);
                    emitByte(0xC0 | (reg2 << 3) | reg1);
                }
                else if (isMemoryOperand(arg2))
                {
                    int base, offset;
                    parseMemoryOperand(arg2, &base, &offset);
                    int disp_bytes = getDispBytes(offset, base);
                    int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                    emitByte(0x2B);
                    emitByte((mod << 6) | (reg1 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                }
                else
                {
                    int imm = parseImmediateValue(arg2);
                    int imm_bytes = getImmBytes(imm);
                    if (imm_bytes == 1)
                    {
                        emitByte(0x83);
                        emitByte(0xE8 + reg1);
                        emitByte(imm);
                    }
                    else
                    {
                        emitByte(0x81);
                        emitByte(0xE8 + reg1);
                        emitDword(imm);
                    }
                }
            }
            else if (isMemoryOperand(arg1))
            {
                int base, offset;
                parseMemoryOperand(arg1, &base, &offset);
                int disp_bytes = getDispBytes(offset, base);
                int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                int imm = parseImmediateValue(arg2);
                int imm_bytes = getImmBytes(imm);
                if (imm_bytes == 1)
                {
                    emitByte(0x83);
                    emitByte((mod << 6) | (5 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    emitByte(imm);
                }
                else
                {
                    emitByte(0x81);
                    emitByte((mod << 6) | (5 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    emitDword(imm);
                }
            }
        }
        else if (strcmp(instruction, (uint8_t *)"cmp") == 0 && arg1[0] && arg2[0])
        {
            int reg1 = parseRegisterIndex(arg1);
            int reg2 = parseRegisterIndex(arg2);
            if (reg1 != -1 && reg2 != -1)
            {
                emitByte(0x39);
                emitByte(0xC0 | (reg2 << 3) | reg1);
            }
            else if (reg1 != -1)
            {
                int imm = parseImmediateValue(arg2);
                int imm_bytes = getImmBytes(imm);
                if (imm_bytes == 1)
                {
                    emitByte(0x83);
                    emitByte(0xF8 + reg1); // /7 for cmp
                    emitByte(imm);
                }
                else
                {
                    emitByte(0x81);
                    emitByte(0xF8 + reg1);
                    emitDword(imm);
                }
            }
            else if (isMemoryOperand(arg1))
            {
                int base, offset;
                parseMemoryOperand(arg1, &base, &offset);
                int disp_bytes = getDispBytes(offset, base);
                int mod = (disp_bytes == 0) ? 0 : (disp_bytes == 1 ? 1 : 2);
                int imm = parseImmediateValue(arg2);
                int imm_bytes = getImmBytes(imm);
                if (imm_bytes == 1)
                {
                    emitByte(0x83);
                    emitByte((mod << 6) | (7 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    emitByte(imm);
                }
                else
                {
                    emitByte(0x81);
                    emitByte((mod << 6) | (7 << 3) | base);
                    if (disp_bytes == 1) emitByte(offset);
                    else if (disp_bytes == 4) emitDword(offset);
                    emitDword(imm);
                }
            }
        }
        else if (strcmp(instruction, (uint8_t *)"jmp") == 0 && arg1[0])
        {
            int labelIndex = findLabelIndex(arg1);
            if (labelIndex != -1)
            { // jmp label (relative 32-bit)
                int offset = labels[labelIndex].address - (currentCodeAddress + 5);
                emitByte(0xE9);
                emitDword(offset);
            }
        }
        else if (strcmp(instruction, (uint8_t *)"jne") == 0 && arg1[0])
        {
            int labelIndex = findLabelIndex(arg1);
            if (labelIndex != -1)
            { // jne label (relative 32-bit)
                int offset = labels[labelIndex].address - (currentCodeAddress + 6);
                emitByte(0x0F);
                emitByte(0x85);
                emitDword(offset);
            }
        }
        else if (strcmp(instruction, (uint8_t *)"jle") == 0 && arg1[0])
        {
            int labelIndex = findLabelIndex(arg1);
            if (labelIndex != -1)
            { // jle label (relative 32-bit)
                int offset = labels[labelIndex].address - (currentCodeAddress + 6);
                emitByte(0x0F);
                emitByte(0x8E);
                emitDword(offset);
            }
        }
        else if (strcmp(instruction, (uint8_t *)"jge") == 0 && arg1[0])
        {
            int labelIndex = findLabelIndex(arg1);
            if (labelIndex != -1)
            { // jge label (relative 32-bit)
                int offset = labels[labelIndex].address - (currentCodeAddress + 6);
                emitByte(0x0F);
                emitByte(0x8D);
                emitDword(offset);
            }
        }
        else if (strcmp(instruction, (uint8_t *)"call") == 0 && arg1[0])
        {
            int regIndex = parseRegisterIndex(arg1);
            if (regIndex != -1)
            {
                emitByte(0xFF);
                emitByte(0xC0 | (2 << 3) | regIndex);
            }
            else
            {
                int labelIndex = findLabelIndex(arg1);
                if (labelIndex != -1)
                { // call label (relative 32-bit)
                    int offset = labels[labelIndex].address - (currentCodeAddress + 5);
                    emitByte(0xE8);
                    emitDword(offset);
                }
                else
                {
                    hasExternalLabel = true;
                    uint32_t libAddr = findLibFunctionAddr(arg1);
                    if (libAddr != 0)
                    {
                        uint32_t codeBase = 0x00402000;
                        int offset = libAddr - (codeBase + currentCodeAddress + 5);
                        emitByte(0xE8);
                        emitDword(offset);
                    }
                }
            }
        }
        else if (strcmp(instruction, (uint8_t *)"int") == 0 && arg1[0])
        {
            int immediateValue = parseImmediateValue(arg1);
            emitByte(0xCD); // int opcode
            emitByte(immediateValue & 0xFF);
        }
        else if (strcmp(instruction, (uint8_t *)"pusha") == 0)
        {
            emitByte(0x60); // pusha opcode
        }
        else if (strcmp(instruction, (uint8_t *)"popa") == 0)
        {
            emitByte(0x61); // popa opcode
        }
        else if (strcmp(instruction, (uint8_t *)"push") == 0 && arg1[0])
        {
            int regIndex = parseRegisterIndex(arg1);
            if (regIndex != -1)
            {
                emitByte(0x50 + regIndex); // push reg
            }
        }
        else if (strcmp(instruction, (uint8_t *)"pop") == 0 && arg1[0])
        {
            int regIndex = parseRegisterIndex(arg1);
            if (regIndex != -1)
            {
                emitByte(0x58 + regIndex); // pop reg
            }
        }
        else if (strcmp(instruction, (uint8_t *)"ret") == 0)
        {
            emitByte(0xC3); // ret opcode
        }
    }
}

/**
 * Writes an ELF executable file structure to the output buffer, including headers, code, and data sections.
 * This function sets up the ELF header, program headers for loadable segments (header, text, data),
 * calculates alignments and virtual addresses, copies code and data, and pads sections to page boundaries.
 * It also handles entry point and dynamic library flags.
 * @param outputBuffer The base address where the ELF file is written.
 * @param codeSection The buffer containing the code section data.
 * @param codeSectionSize The size of the code section.
 * @param dataSection The buffer containing the data section data.
 * @param dataSectionSize The size of the data section.
 * @param entryPointOffset The offset of the entry point in the code section.
 */
void writeElf(uint8_t *outputBuffer, const unsigned char *codeSection, int codeSectionSize, const unsigned char *dataSection, int dataSectionSize, int entryPointOffset)
{
    struct elfHeader elfHeader;
    fillMemory((uint8_t*)&elfHeader, 0, sizeof(elfHeader));
    elfHeader.e_ident[0] = 0x7F;
    elfHeader.e_ident[1] = 'E';
    elfHeader.e_ident[2] = 'L';
    elfHeader.e_ident[3] = 'F';
    elfHeader.e_ident[4] = 1; // 32-bit class
    elfHeader.e_ident[5] = 1; // Little-endian
    elfHeader.e_ident[6] = 1; // ELF version
    elfHeader.e_ident[7] = 0; // OS ABI none
    elfHeader.e_type = 2; // Executable
    elfHeader.e_machine = 3; // i386
    elfHeader.e_version = 1;
    elfHeader.e_phoff = 52; // Program header offset
    elfHeader.e_shoff = 0; // No section headers
    // This next line is my custom flag to tell the OS to load libc.o to 0x700000
    if (hasExternalLabel)
    {
        elfHeader.e_flags = ELF_LOAD_DYNAMIC_LIBRARIES;
    }
    else
    {
        elfHeader.e_flags = 0;
    }
    elfHeader.e_ehsize = 52;
    elfHeader.e_phentsize = 32;
    elfHeader.e_phnum = 3;
    elfHeader.e_shentsize = 0;
    elfHeader.e_shnum = 0;
    elfHeader.e_shstrndx = 0;

    uint32_t codeOffset = PAGE_SIZE;
    uint32_t aligned_code_size = ((codeSectionSize + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    uint32_t aligned_data_size = ((dataSectionSize + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;

    uint32_t header_vaddr = 0x00401000;
    uint32_t code_vaddr = header_vaddr + PAGE_SIZE;
    uint32_t data_vaddr = code_vaddr + aligned_code_size;

    struct pHeader programHeaders[3];
    fillMemory((uint8_t*)programHeaders, 0, sizeof(programHeaders));

    // Header segment (ELF header, program headers, and padding)
    programHeaders[0].p_type = 1; // Loadable segment
    programHeaders[0].p_offset = 0;
    programHeaders[0].p_vaddr = header_vaddr;
    programHeaders[0].p_paddr = header_vaddr;
    programHeaders[0].p_filesz = PAGE_SIZE;
    programHeaders[0].p_memsz = PAGE_SIZE;
    programHeaders[0].p_flags = 4; // Read
    programHeaders[0].p_align = PAGE_SIZE;

    // Text segment (code)
    programHeaders[1].p_type = 1; // Loadable segment
    programHeaders[1].p_offset = PAGE_SIZE;
    programHeaders[1].p_vaddr = code_vaddr;
    programHeaders[1].p_paddr = code_vaddr;
    programHeaders[1].p_filesz = aligned_code_size;
    programHeaders[1].p_memsz = aligned_code_size;
    programHeaders[1].p_flags = 5; // Read + Execute
    programHeaders[1].p_align = PAGE_SIZE;

    // Data segment (BSS)
    programHeaders[2].p_type = 1; // Loadable segment
    programHeaders[2].p_offset = PAGE_SIZE + aligned_code_size;
    programHeaders[2].p_vaddr = data_vaddr;
    programHeaders[2].p_paddr = data_vaddr;
    programHeaders[2].p_filesz = aligned_data_size;
    programHeaders[2].p_memsz = aligned_data_size;
    programHeaders[2].p_flags = 6; // Read + Write
    programHeaders[2].p_align = PAGE_SIZE;

    elfHeader.e_entry = code_vaddr + entryPointOffset;

    bytecpy(outputBuffer, (uint8_t *)&elfHeader, sizeof(elfHeader));
    uint8_t *pointer = outputBuffer + sizeof(elfHeader);

    bytecpy(pointer, (uint8_t *)programHeaders, sizeof(programHeaders));
    pointer += sizeof(programHeaders);

    int headerEnd = sizeof(elfHeader) + sizeof(programHeaders);
    int paddingSize = codeOffset - headerEnd;
    fillMemory(pointer, 0, paddingSize);
    pointer += paddingSize;

    // Copy code and pad the text segment
    if (codeSectionSize > 0)
    {
        bytecpy(pointer, (uint8_t *)codeSection, codeSectionSize);
        pointer += codeSectionSize;

        // There a failure mode where the ELF header is written again
        // in the text segment. I am checking for that.
        if (codeSection[0] == 0x7f)
        {
            printString(COLOR_RED, 25, 5, (uint8_t*)"Malformed output file!");
            wait(2);
            systemExit(PROCESS_EXIT_CODE_MALFORMED_OUTPUT);
        }
    }
    int textPadding = aligned_code_size - codeSectionSize;
    fillMemory(pointer, 0, textPadding);
    pointer += textPadding;

    // Copy data and pad the data segment
    if (dataSectionSize > 0)
    {
        bytecpy(pointer, (uint8_t *)dataSection, dataSectionSize);
        pointer += dataSectionSize;
    }
    int dataPadding = aligned_data_size - dataSectionSize;
    fillMemory(pointer, 0, dataPadding);
}

/**
 * The main entry point of the assembler program.
 * This function handles the entire assembly process: opening files, mapping libraries,
 * performing two passes over the input, writing the ELF output, and cleaning up temporary files.
 * It prints status messages and handles errors with appropriate exit codes.
 */
void main()
{
    uint32_t currentPid; 
    uint32_t inputFd;
    uint8_t* inputFileContent;
    uint32_t libcFd;
    uint8_t* libcFileContent;
    uint32_t tempFd;
    uint8_t* tempFileContent;
    uint32_t outputFd;
    uint8_t* outputFileContent;

    clearScreen();
    currentPrintRow++;
    printString(COLOR_LIGHT_BLUE, currentPrintRow++, 5, (uint8_t *)"--------- ikanOS Assembler v1 --------- ");
    currentPrintRow++;
    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Opening input file: ");
    printString(COLOR_WHITE, currentPrintRow-1, 25, (uint8_t*)(COMMAND_BUFFER + 3));

    if (systemOpenFile((uint8_t*)(COMMAND_BUFFER + 3), RDONLY) == SYSCALL_FAIL)
    {
        currentPrintRow++;
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Failed to open input file");
        wait(3);
        systemExit(PROCESS_EXIT_CODE_INPUT_FILE_FAILED);
    }

    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    inputFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    inputFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Opening libc.o ");
    systemOpenFile((uint8_t*)("libc.o\n"), RDONLY);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    libcFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    libcFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);
    
    // Libc grows as I add functions, so I need to figure out how much to allocate
    // and copy. I retrieve the inode and then compute based on size.
    systemGetInodeStructureOfFile((uint8_t*)"libc.o");
    struct inode *Inode = (struct inode *)REQUESTED_INODE_LOC;

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Copying libc.o to 0x700000 ");
    for (uint32_t dynamicLibrariesLoc = 0; dynamicLibrariesLoc < ((ceiling(Inode->i_size, PAGE_SIZE)) * PAGE_SIZE); dynamicLibrariesLoc = dynamicLibrariesLoc + PAGE_SIZE)
    {
        systemMMap((uint8_t *)(SHARED_LIBRARIES_START_LOC + dynamicLibrariesLoc));

    }
    
    bytecpy((uint8_t*)SHARED_LIBRARIES_START_LOC, libcFileContent, ceiling(Inode->i_size, PAGE_SIZE) * PAGE_SIZE);

    uint8_t *inputStartPointer = inputFileContent;
    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Executing first pass.");
    firstPass(inputStartPointer);
    int startLabelIndex = findLabelIndex((uint8_t *)"start");
    int entryPointAddress = (startLabelIndex != -1) ? labels[startLabelIndex].address : 0;
    currentCodeAddress = 0;
    currentPrintRow++;
    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Executing second pass.");
    secondPass(inputStartPointer);

    // Remove the "\n" in this string if you want another argument after this in the 
    // COMMAND_BUFFER.

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Creating temp file.");
    systemOpenEmptyFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".al.tmp\n"), 3);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemCloseFile(tempFd);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Opening temp.");
    systemOpenFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".al.tmp\n"), RDWRITE);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Writing ELF.");
    writeElf(tempFileContent, codeSectionBuffer, codeSectionLength, dataSectionBuffer, dataSectionLength, entryPointAddress);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Creating program binary.");
    systemCreateFile(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), tempFd);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    outputFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    outputFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemCloseFile(tempFd);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    outputFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    outputFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemCloseFile(outputFd);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    outputFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    outputFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemCloseFile(libcFd);
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    libcFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    libcFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Deleting temp file.");
    systemDeleteFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".al.tmp"));
    currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFd = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFileContent = (uint8_t*)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    currentPrintRow++;
    printString(COLOR_LIGHT_BLUE, currentPrintRow++, 5, (uint8_t *)"--------- Complete. Exiting. --------- ");
    
    wait(1);
    
    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}