// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "constants.h"
#include "libc-main.h"
#include "screen.h"
#include "x86.h"
#include "vm.h"

#define MAX_TOKEN 1000
#define MAX_ASM_OUTPUT 0x1000
#define MAX_STRING_BUFFER 500
#define MAX_SYMBOLS 64
#define MAX_SYMBOL_NAME_LENGTH 50
#define MAX_TOKEN_VALUE 50
#define MAX_EXPR_CODE 200

enum TokenType
{
    INT, INT_POINTER, VOID, FUNCTION, LPAREN, RPAREN, LBRACE, RBRACE, IDENTIFIER, NUMBER,
    EQUALS, RETURN, SEMICOLON, END, IF, ELSE, NOT_EQUAL, COMMA, SYSCALL,
    WHILE, PLUS, LESS, STAR, CHAR, STRING_LITERAL
};

struct SymbolEntry
{
    uint8_t name[MAX_SYMBOL_NAME_LENGTH];
    int offset;
    enum TokenType type;
};

struct DataSymbol
{
    uint8_t name[MAX_SYMBOL_NAME_LENGTH];
};

struct ExprInfo
{
    uint8_t code[MAX_EXPR_CODE];
    int isImmediate;
    uint8_t value[MAX_TOKEN_VALUE];
    uint8_t location[30];
};

struct SymbolEntry symbolTable[MAX_SYMBOLS];
int symbolTableCount = 0;
struct DataSymbol dataSymbolTable[MAX_SYMBOLS];
int dataSymbolCount = 0;
uint8_t definedFunctionNames[MAX_SYMBOLS][MAX_SYMBOL_NAME_LENGTH];
int definedFunctionCount = 0;
uint32_t currentPrintRow = 0;

struct Token
{
    enum TokenType type;
    uint8_t value[MAX_TOKEN_VALUE];
};

uint8_t *sourceCodeInput;
int sourcePosition = 0;
struct Token currentToken;
uint8_t asmCodeBuffer[MAX_ASM_OUTPUT];
int asmCodePosition = 0;
uint8_t dataSectionBuffer[MAX_ASM_OUTPUT];
int dataSectionPosition = 0;
int currentLineNumber = 1;

// Emits the provided assembly string to the assembly code buffer by appending it and updating the position.
// This function is used to generate assembly instructions during compilation.
void emitAssembly(uint8_t *asmStr)
{
    strcpy(asmCodeBuffer + asmCodePosition, asmStr);
    asmCodePosition += strlen(asmStr);
}

// Emits the provided data string to the data section buffer by appending it and updating the position.
// This function is used to generate data declarations in the assembly output.
void emitData(uint8_t *dataStr)
{
    strcpy(dataSectionBuffer + dataSectionPosition, dataStr);
    dataSectionPosition += strlen(dataStr);
}

// Appends the provided string to the code field of the expression info structure.
// This is used to build up assembly code snippets for expressions.
void appendToCode(struct ExprInfo *info, uint8_t *str)
{
    strcpy(info->code + strlen(info->code), str);
}

// Checks if the given token string matches a specific keyword and updates the current token's type and value if it does.
// Returns 1 if matched, otherwise 0. Used for identifying keywords in the source code.
int isKeywordMatch(uint8_t *tokenStr, uint8_t *keyword, TokenType type)
{
    if (strcmp(tokenStr, keyword) == 0)
    {
        currentToken.type = type;
        strcpy(currentToken.value, tokenStr);
        return 1;
    }
    return 0;
}

// Adds a new symbol entry to the symbol table with the provided name and type.
// Computes the stack offset as a negative value based on the current symbol count.
// If the symbol table is full, prints an error message with the line number.
// Also prints debug information about the added symbol.
void addSymbol(uint8_t *symbolName, TokenType symbolType)
{
    if (symbolTableCount >= MAX_SYMBOLS)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Too many variables");
    }
    strcpy(symbolTable[symbolTableCount].name, symbolName);
    symbolTable[symbolTableCount].type = symbolType;
    symbolTable[symbolTableCount].offset = - (symbolTableCount + 1) * 4;
    symbolTableCount++;

    currentPrintRow++;
    printString(COLOR_GREEN, currentPrintRow++, 10, (uint8_t*)"Symbol:");
    printString(COLOR_LIGHT_BLUE, currentPrintRow-1, 18, symbolName);
}

// Retrieves the type of a symbol from the symbol table by searching for the matching name.
// If the symbol is not found, prints an error message indicating an undefined symbol and returns VOID.
enum TokenType getSymbolType(uint8_t *symbolName)
{
    int i;
    for (i = 0; i < symbolTableCount; i++)
    {
        if (strcmp(symbolTable[i].name, symbolName) == 0)
        {
            return symbolTable[i].type;
        }
    }
    printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
    printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Undefined symbol");

    return VOID;
}

// Retrieves the stack offset of a symbol from the symbol table by searching for the matching name.
// If not found, implicitly adds the symbol as an INT type and returns its new offset.
// This allows for undeclared variables to be handled automatically.
int getSymbolOffset(uint8_t *symbolName)
{
    int i;
    for (i = 0; i < symbolTableCount; i++)
    {
        if (strcmp(symbolTable[i].name, symbolName) == 0)
        {
            return symbolTable[i].offset;
        }
    }
    // Implicitly insert undeclared variable
    addSymbol(symbolName, INT);
    return symbolTable[symbolTableCount - 1].offset;
}

// Checks if the given character is alphanumeric (letter or digit).
// Relies on isAlpha and isDigit functions from the libc.
int isAlnum(unsigned char character)
{
    return isAlpha(character) || isDigit(character);
}

// Checks if the given character is valid for use in an identifier (alphanumeric or underscore).
int isIdentifierChar(unsigned char character)
{
    return isAlnum(character) || character == '_';
}

// Peeks at the next non-whitespace character in the source code without advancing the position.
// Skips over spaces, tabs, etc., to look ahead.
unsigned char peekNextNonSpace()
{
    int pos = sourcePosition;
    while (isSpace(sourceCodeInput[pos])) pos++;
    return sourceCodeInput[pos];
}

// Appends the source string to the destination string.
// Uses strcpy to append by calculating the end of the destination.
void appendString(uint8_t *destination, uint8_t *source)
{
    strcpy(destination + strlen(destination), source);
}

// Checks if a function with the given name has been defined by searching the defined function names array.
// Returns 1 if found, otherwise 0.
int isFunctionDefined(uint8_t *name)
{
    int i;
    for (i = 0; i < definedFunctionCount; i++)
    {
        if (strcmp(definedFunctionNames[i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

// Parses and retrieves the next token from the source code input.
// Handles skipping whitespace and comments, parsing numbers (decimal and hex), identifiers, keywords, strings, and symbols.
// Updates the currentToken structure with the type and value.
// Errors on invalid characters.
void getNextToken()
{
    do {
        // Skip whitespace
        while (isSpace(sourceCodeInput[sourcePosition]))
        {
            if (sourceCodeInput[sourcePosition] == '\n') currentLineNumber++;
            sourcePosition++;
        }

        // Skip line comments starting with //
        if (sourceCodeInput[sourcePosition] == '/' && sourceCodeInput[sourcePosition + 1] == '/')
        {
            sourcePosition += 2;
            while (sourceCodeInput[sourcePosition] != '\n' && sourceCodeInput[sourcePosition] != '\0')
            {
                sourcePosition++;
            }
            if (sourceCodeInput[sourcePosition] == '\n')
            {
                currentLineNumber++;
                sourcePosition++;
            }
        }
        else
        {
            break;
        }
    } while (1);

    if (sourceCodeInput[sourcePosition] == '\0')
    {
        currentToken.type = END;
        currentToken.value[0] = '\0';
        return;
    }

    if (sourceCodeInput[sourcePosition] == '"')
    {
        sourcePosition++;
        int i = 0;
        while (sourceCodeInput[sourcePosition] != '"' && sourceCodeInput[sourcePosition] != '\0')
        {
            currentToken.value[i++] = sourceCodeInput[sourcePosition++];
        }
        currentToken.value[i] = '\0';
        if (sourceCodeInput[sourcePosition] == '"') sourcePosition++;
        currentToken.type = STRING_LITERAL;
        return;
    }

    if (isDigit(sourceCodeInput[sourcePosition]))
    {
        currentToken.type = NUMBER;
        int i = 0;
        int isHex = 0;
        if (sourceCodeInput[sourcePosition] == '0' && (sourceCodeInput[sourcePosition + 1] == 'x' || sourceCodeInput[sourcePosition + 1] == 'X'))
        {
            isHex = 1;
            currentToken.value[i++] = sourceCodeInput[sourcePosition++];
            currentToken.value[i++] = sourceCodeInput[sourcePosition++];
        }
        while (1)
        {
            char c = sourceCodeInput[sourcePosition];
            if (isHex)
            {
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) break;
            }
            else
            {
                if (!(c >= '0' && c <= '9')) break;
            }
            currentToken.value[i++] = c;
            sourcePosition++;
        }
        currentToken.value[i] = '\0';
        return;
    }

    if (isAlpha(sourceCodeInput[sourcePosition]))
    {
        currentToken.type = IDENTIFIER;
        int i = 0;
        while (isIdentifierChar(sourceCodeInput[sourcePosition])) currentToken.value[i++] = sourceCodeInput[sourcePosition++];
        currentToken.value[i] = '\0';

        if (isKeywordMatch(currentToken.value, (uint8_t *)"int", INT)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"char", CHAR)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"void", VOID)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"main", FUNCTION)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"return", RETURN)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"if", IF)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"else", ELSE)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"syscall", SYSCALL)) return;
        if (isKeywordMatch(currentToken.value, (uint8_t *)"while", WHILE)) return;
        return;
    }

    switch (sourceCodeInput[sourcePosition])
    {
        case '(': currentToken.type = LPAREN; strcpy(currentToken.value, (uint8_t *)"("); sourcePosition++; break;
        case ')': currentToken.type = RPAREN; strcpy(currentToken.value, (uint8_t *)")"); sourcePosition++; break;
        case '{': currentToken.type = LBRACE; strcpy(currentToken.value, (uint8_t *)"{"); sourcePosition++; break;
        case '}': currentToken.type = RBRACE; strcpy(currentToken.value, (uint8_t *)"}"); sourcePosition++; break;
        case ';': currentToken.type = SEMICOLON; strcpy(currentToken.value, (uint8_t *)";"); sourcePosition++; break;
        case '=': currentToken.type = EQUALS; strcpy(currentToken.value, (uint8_t *)"="); sourcePosition++; break;
        case ',' : currentToken.type = COMMA; strcpy(currentToken.value, (uint8_t *)","); sourcePosition++; break;
        case '+': currentToken.type = PLUS; strcpy(currentToken.value, (uint8_t *)"+"); sourcePosition++; break;
        case '<': currentToken.type = LESS; strcpy(currentToken.value, (uint8_t *)"<"); sourcePosition++; break;
        case '*': currentToken.type = STAR; strcpy(currentToken.value, (uint8_t *)"*"); sourcePosition++; break;
        case '!':
            if (sourceCodeInput[sourcePosition + 1] == '=')
            {
                currentToken.type = NOT_EQUAL;
                strcpy(currentToken.value, (uint8_t *)"!=");
                sourcePosition += 2;
            }
            else
            {
                printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
                printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Invalid character");
            }
            break;
        default:
            printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
            printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Invalid character");
    }
}

// Verifies that the current token matches the expected type and advances to the next token.
// If not matched, prints an error message indicating an unexpected token.
void expectToken(TokenType expectedType)
{
    if (currentToken.type != expectedType)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Unexpected token");
    }
    getNextToken();
}

// Parses an expression and populates the ExprInfo structure with generated code, value, and location.
// Handles immediate numbers, identifiers, and chains of additions.
// If additions are present, generates assembly to compute the result in EAX.



// "currentToken" is the lookahead token in this parser.
// The computation branches based on the type here.
// Based on this, and the parseStatementList() recursive grammar pattern below,
// this is an LL(1) parser (recursive descent) and is right-recursive

void parseExpressionInfo(struct ExprInfo *info)
{
    uint8_t primaryLoc[30];
    uint8_t primaryVal[20];
    int isPrimaryImmediate;

    fillMemory(info->code, 0, MAX_EXPR_CODE);
    info->isImmediate = 0;
    info->value[0] = '\0';
    info->location[0] = '\0';

    if (currentToken.type == NUMBER)
    {
        isPrimaryImmediate = 1;
        strcpy(primaryVal, currentToken.value);
        getNextToken();
    }
    else if (currentToken.type == IDENTIFIER)
    {
        isPrimaryImmediate = 0;
        int isData = 0;
        for (int k = 0; k < dataSymbolCount; k++)
        {
            if (strcmp(dataSymbolTable[k].name, currentToken.value) == 0)
            {
                isData = 1;
                strcpy(primaryLoc, currentToken.value);
                break;
            }
        }
        if (!isData)
        {
            int offset = getSymbolOffset(currentToken.value);
            uint8_t offbuf[10];
            itoa(offset, offbuf);
            strcpy(primaryLoc, (uint8_t *)"[ebp");
            appendString(primaryLoc, offbuf);
            appendString(primaryLoc, (uint8_t *)"]");
        }
        getNextToken();
    }
    else
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number or identifier");
    }

    if (currentToken.type != PLUS)
    {
        if (isPrimaryImmediate)
        {
            info->isImmediate = 1;
            strcpy(info->value, primaryVal);
        }
        else
        {
            info->isImmediate = 0;
            strcpy(info->location, primaryLoc);
        }
        return;
    }

    // Load primary to eax if there is a +
    appendToCode(info, (uint8_t *)"\tmov eax, ");
    if (isPrimaryImmediate)
    {
        appendToCode(info, primaryVal);
    }
    else
    {
        appendToCode(info, primaryLoc);
    }
    appendToCode(info, (uint8_t *)"\n");

    // Handle chain of + number
    while (currentToken.type == PLUS)
    {
        getNextToken();
        if (currentToken.type != NUMBER)
        {
            printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
            printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number after +");
        }
        appendToCode(info, (uint8_t *)"\tadd eax, ");
        appendToCode(info, currentToken.value);
        appendToCode(info, (uint8_t *)"\n");
        getNextToken();
    }

    info->isImmediate = 0;
    strcpy(info->location, (uint8_t *)"eax");
}

// Parses a variable declaration statement, handling types like int, char, pointers, and initializations.
// Supports string literals for char declarations and syscall initializations.
// Generates assembly for initializations and adds the symbol to the table.
void parseDeclaration()
{
    TokenType declType;
    if (currentToken.type == INT)
    {
        declType = INT;
        expectToken(INT);
    }
    else if (currentToken.type == CHAR)
    {
        declType = CHAR;
        expectToken(CHAR);
    }
    else
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected int or char");
    }
    if (currentToken.type == STAR)
    {
        expectToken(STAR);
        if (declType == INT) declType = INT_POINTER;
    }
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier");
    }
    uint8_t variableName[MAX_SYMBOL_NAME_LENGTH];
    strcpy(variableName, currentToken.value);
    getNextToken();

    if (currentToken.type == EQUALS)
    {
        getNextToken();
        if (currentToken.type == STRING_LITERAL)
        {
            if (declType != CHAR)
            {
                printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
                printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"String literals only for char");
            }
            strcpy(dataSymbolTable[dataSymbolCount].name, variableName);
            dataSymbolCount++;
            emitData(variableName);
            emitData((uint8_t*)":\n    db '");
            emitData(currentToken.value);
            emitData((uint8_t*)"'\n\n");
            getNextToken();
            expectToken(SEMICOLON);
            return;
        }
        else if (currentToken.type == SYSCALL)
        {
            getNextToken();
            expectToken(LPAREN);
            if (currentToken.type != NUMBER)
            {
                printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
                printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number");
            }
            emitAssembly((uint8_t *)"\tmov eax, ");
            emitAssembly(currentToken.value);
            emitAssembly((uint8_t *)"\n");
            getNextToken();
            expectToken(COMMA);
            struct ExprInfo info1;
            parseExpressionInfo(&info1);
            emitAssembly(info1.code);
            emitAssembly((uint8_t *)"\tmov ebx, ");
            if (info1.isImmediate)
            {
                emitAssembly(info1.value);
            }
            else
            {
                emitAssembly(info1.location);
            }
            emitAssembly((uint8_t *)"\n");
            expectToken(COMMA);
            struct ExprInfo info2;
            parseExpressionInfo(&info2);
            emitAssembly(info2.code);
            emitAssembly((uint8_t *)"\tmov ecx, ");
            if (info2.isImmediate)
            {
                emitAssembly(info2.value);
            }
            else
            {
                emitAssembly(info2.location);
            }
            emitAssembly((uint8_t *)"\n");
            emitAssembly((uint8_t *)"\tpusha\n");
            emitAssembly((uint8_t *)"\tint 0x80\n");
            emitAssembly((uint8_t *)"\tpopa\n");
            int leftOffset = getSymbolOffset(variableName);
            uint8_t offbuf[10];
            itoa(leftOffset, offbuf);
            uint8_t leftLoc[30];
            strcpy(leftLoc, (uint8_t *)"[ebp");
            appendString(leftLoc, offbuf);
            appendString(leftLoc, (uint8_t *)"]");
            emitAssembly((uint8_t *)"\tmov ");
            emitAssembly(leftLoc);
            emitAssembly((uint8_t *)", eax\n");
            expectToken(RPAREN);
        }
        else
        {
            struct ExprInfo info;
            parseExpressionInfo(&info);
            int leftOffset = getSymbolOffset(variableName);
            uint8_t offbuf[10];
            itoa(leftOffset, offbuf);
            uint8_t leftLoc[30];
            strcpy(leftLoc, (uint8_t *)"[ebp");
            appendString(leftLoc, offbuf);
            appendString(leftLoc, (uint8_t *)"]");
            emitAssembly(info.code);
            emitAssembly((uint8_t *)"\tmov eax, ");
            if (info.isImmediate)
            {
                emitAssembly(info.value);
            }
            else
            {
                emitAssembly(info.location);
            }
            emitAssembly((uint8_t *)"\n");
            emitAssembly((uint8_t *)"\tmov ");
            emitAssembly(leftLoc);
            emitAssembly((uint8_t *)", eax\n");
        }
    }
    addSymbol(variableName, declType);
    expectToken(SEMICOLON);
}

// Parses an assignment statement of the form identifier = expression;
// Generates assembly to evaluate the expression and store the result in the variable's stack location.
// Supports syscall expressions as the right-hand side.
void parseAssignment()
{
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier");
    }
    uint8_t variableName[MAX_SYMBOL_NAME_LENGTH];
    strcpy(variableName, currentToken.value);
    getNextToken();
    expectToken(EQUALS);
    if (currentToken.type == SYSCALL)
    {
        getNextToken();
        expectToken(LPAREN);
        if (currentToken.type != NUMBER)
        {
            printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
            printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number");
            
        }
        emitAssembly((uint8_t *)"\tmov eax, ");
        emitAssembly(currentToken.value);
        emitAssembly((uint8_t *)"\n");
        getNextToken();
        expectToken(COMMA);
        struct ExprInfo info1;
        parseExpressionInfo(&info1);
        emitAssembly(info1.code);
        emitAssembly((uint8_t *)"\tmov ebx, ");
        if (info1.isImmediate)
        {
            emitAssembly(info1.value);
        }
        else
        {
            emitAssembly(info1.location);
        }
        emitAssembly((uint8_t *)"\n");
        expectToken(COMMA);
        struct ExprInfo info2;
        parseExpressionInfo(&info2);
        emitAssembly(info2.code);
        emitAssembly((uint8_t *)"\tmov ecx, ");
        if (info2.isImmediate)
        {
            emitAssembly(info2.value);
        }
        else
        {
            emitAssembly(info2.location);
        }
        emitAssembly((uint8_t *)"\n");
        emitAssembly((uint8_t *)"\tpusha\n");
        emitAssembly((uint8_t *)"\tint 0x80\n");
        emitAssembly((uint8_t *)"\tpopa\n");
        int leftOffset = getSymbolOffset(variableName);
        uint8_t offbuf[10];
        itoa(leftOffset, offbuf);
        uint8_t leftLoc[30];
        strcpy(leftLoc, (uint8_t *)"[ebp");
        appendString(leftLoc, offbuf);
        appendString(leftLoc, (uint8_t *)"]");
        emitAssembly((uint8_t *)"\tmov ");
        emitAssembly(leftLoc);
        emitAssembly((uint8_t *)", eax\n");
        expectToken(RPAREN);
    }
    else
    {
        struct ExprInfo info;
        parseExpressionInfo(&info);
        int leftOffset = getSymbolOffset(variableName);
        uint8_t offbuf[10];
        itoa(leftOffset, offbuf);
        uint8_t leftLoc[30];
        strcpy(leftLoc, (uint8_t *)"[ebp");
        appendString(leftLoc, offbuf);
        appendString(leftLoc, (uint8_t *)"]");
        emitAssembly(info.code);
        emitAssembly((uint8_t *)"\tmov eax, ");
        if (info.isImmediate)
        {
            emitAssembly(info.value);
        }
        else
        {
            emitAssembly(info.location);
        }
        emitAssembly((uint8_t *)"\n");
        emitAssembly((uint8_t *)"\tmov ");
        emitAssembly(leftLoc);
        emitAssembly((uint8_t *)", eax\n");
    }
    expectToken(SEMICOLON);
}

// Parses a dereference assignment statement of the form *identifier = expression;
// Ensures the identifier is a pointer type and generates assembly to store the evaluated expression at the pointed location.
// Supports syscall expressions as the right-hand side.
void parseDerefAssignment()
{
    expectToken(STAR);
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier after *");
    }
    uint8_t variableName[MAX_SYMBOL_NAME_LENGTH];
    strcpy(variableName, currentToken.value);
    enum TokenType symType = getSymbolType(variableName);
    if (symType != INT_POINTER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Cannot dereference non-pointer");
    }
    int offset = getSymbolOffset(variableName);
    uint8_t offbuf[10];
    itoa(offset, offbuf);
    uint8_t ptrLoc[30];
    strcpy(ptrLoc, (uint8_t *)"[ebp");
    appendString(ptrLoc, offbuf);
    appendString(ptrLoc, (uint8_t *)"]");
    getNextToken();
    expectToken(EQUALS);
    if (currentToken.type == SYSCALL)
    {
        getNextToken();
        expectToken(LPAREN);
        if (currentToken.type != NUMBER)
        {
            printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
            printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number");
        }
        emitAssembly((uint8_t *)"\tmov eax, ");
        emitAssembly(currentToken.value);
        emitAssembly((uint8_t *)"\n");
        getNextToken();
        expectToken(COMMA);
        struct ExprInfo info1;
        parseExpressionInfo(&info1);
        emitAssembly(info1.code);
        emitAssembly((uint8_t *)"\tmov ebx, ");
        if (info1.isImmediate)
        {
            emitAssembly(info1.value);
        }
        else
        {
            emitAssembly(info1.location);
        }
        emitAssembly((uint8_t *)"\n");
        expectToken(COMMA);
        struct ExprInfo info2;
        parseExpressionInfo(&info2);
        emitAssembly(info2.code);
        emitAssembly((uint8_t *)"\tmov ecx, ");
        if (info2.isImmediate)
        {
            emitAssembly(info2.value);
        }
        else
        {
            emitAssembly(info2.location);
        }
        emitAssembly((uint8_t *)"\n");
        emitAssembly((uint8_t *)"\tpusha\n");
        emitAssembly((uint8_t *)"\tint 0x80\n");
        emitAssembly((uint8_t *)"\tpopa\n");
        emitAssembly((uint8_t *)"\tmov ebx, eax\n");
        emitAssembly((uint8_t *)"\tmov eax, ");
        emitAssembly(ptrLoc);
        emitAssembly((uint8_t *)"\n");
        emitAssembly((uint8_t *)"\tmov [eax], ebx\n");
        expectToken(RPAREN);
    }
    else
    {
        struct ExprInfo info;
        parseExpressionInfo(&info);
        emitAssembly(info.code);
        emitAssembly((uint8_t *)"\tmov ebx, ");
        if (info.isImmediate)
        {
            emitAssembly(info.value);
        }
        else
        {
            emitAssembly(info.location);
        }
        emitAssembly((uint8_t *)"\n");
        emitAssembly((uint8_t *)"\tmov eax, ");
        emitAssembly(ptrLoc);
        emitAssembly((uint8_t *)"\n");
        emitAssembly((uint8_t *)"\tmov [eax], ebx\n");
    }
    expectToken(SEMICOLON);
}

// Parses a return statement and generates assembly to set the exit code and invoke the exit syscall.
// Evaluates the return expression and places its value in EBX for the syscall.
void parseReturnStatement()
{
    expectToken(RETURN);
    struct ExprInfo info;
    parseExpressionInfo(&info);
    emitAssembly(info.code);
    emitAssembly((uint8_t *)"\tmov ebx, ");
    if (info.isImmediate)
    {
        emitAssembly(info.value);
    }
    else
    {
        emitAssembly(info.location);
    }
    emitAssembly((uint8_t *)"\n");
    emitAssembly((uint8_t *)"\tmov eax, 1\n");
    emitAssembly((uint8_t *)"\tint 0x80\n");
    expectToken(SEMICOLON);
}

// Forward declaration for parseStatementList to allow recursive calls in control structures.
void parseStatementList();

// Parses an if statement, including the condition, body, and optional else clause.
// Generates assembly for comparison and conditional jumps using unique labels.
void parseIfStatement()
{
    expectToken(IF);
    expectToken(LPAREN);
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier");
    }
    uint8_t variable1[MAX_SYMBOL_NAME_LENGTH];
    strcpy(variable1, currentToken.value);
    int offset1 = getSymbolOffset(variable1);
    uint8_t offbuf1[10];
    itoa(offset1, offbuf1);
    uint8_t var1loc[30];
    strcpy(var1loc, (uint8_t *)"[ebp");
    appendString(var1loc, offbuf1);
    appendString(var1loc, (uint8_t *)"]");
    getNextToken();
    if (currentToken.type != NOT_EQUAL)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected !=");
    }
    getNextToken();
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier");
    }
    uint8_t variable2[MAX_SYMBOL_NAME_LENGTH];
    strcpy(variable2, currentToken.value);
    int offset2 = getSymbolOffset(variable2);
    uint8_t offbuf2[10];
    itoa(offset2, offbuf2);
    uint8_t var2loc[30];
    strcpy(var2loc, (uint8_t *)"[ebp");
    appendString(var2loc, offbuf2);
    appendString(var2loc, (uint8_t *)"]");
    getNextToken();
    expectToken(RPAREN);
    expectToken(LBRACE);
    static int labelCount = 0;
    uint8_t buffer[10];
    uint8_t elseLabel[20];
    uint8_t endLabel[20];
    strcpy(elseLabel, (uint8_t *)"else_");
    itoa(labelCount, buffer);
    appendString(elseLabel, buffer);
    strcpy(endLabel, (uint8_t *)"end_");
    itoa(labelCount, buffer);
    appendString(endLabel, buffer);
    labelCount++;
    emitAssembly((uint8_t *)"\tmov eax, ");
    emitAssembly(var1loc);
    emitAssembly((uint8_t *)"\n");
    emitAssembly((uint8_t *)"\tcmp eax, ");
    emitAssembly(var2loc);
    emitAssembly((uint8_t *)"\n");
    emitAssembly((uint8_t *)"\tje ");
    emitAssembly(elseLabel);
    emitAssembly((uint8_t *)"\n");
    parseStatementList();
    expectToken(RBRACE);
    emitAssembly((uint8_t *)"\tjmp ");
    emitAssembly(endLabel);
    emitAssembly((uint8_t *)"\n");
    emitAssembly(elseLabel);
    emitAssembly((uint8_t *)":\n");
    if (currentToken.type == ELSE)
    {
        expectToken(ELSE);
        expectToken(LBRACE);
        parseStatementList();
        expectToken(RBRACE);
    }
    emitAssembly(endLabel);
    emitAssembly((uint8_t *)":\n");
}

// Parses a while statement, including the condition and body.
// Generates assembly for the loop with comparison and jumps using unique labels.
// Supports only identifier < number conditions.
void parseWhileStatement()
{
    expectToken(WHILE);
    expectToken(LPAREN);
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier");

    }
    uint8_t variable1[MAX_SYMBOL_NAME_LENGTH];
    strcpy(variable1, currentToken.value);
    int offset1 = getSymbolOffset(variable1);
    uint8_t offbuf1[10];
    itoa(offset1, offbuf1);
    uint8_t var1loc[30];
    strcpy(var1loc, (uint8_t *)"[ebp");
    appendString(var1loc, offbuf1);
    appendString(var1loc, (uint8_t *)"]");
    getNextToken();
    if (currentToken.type != LESS)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected <");
    }
    getNextToken();
    if (currentToken.type != NUMBER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number");

    }
    uint8_t numberBuffer[20];
    strcpy(numberBuffer, currentToken.value);
    getNextToken();
    expectToken(RPAREN);
    expectToken(LBRACE);
    static int labelCount = 0;
    uint8_t buffer[10];
    uint8_t loopLabel[20];
    uint8_t endLabel[20];
    strcpy(loopLabel, (uint8_t *)"loop_");
    itoa(labelCount, buffer);
    appendString(loopLabel, buffer);
    strcpy(endLabel, (uint8_t *)"end_");
    itoa(labelCount, buffer);
    appendString(endLabel, buffer);
    labelCount++;
    emitAssembly(loopLabel);
    emitAssembly((uint8_t *)":\n");
    emitAssembly((uint8_t *)"\tcmp ");
    emitAssembly(var1loc);
    emitAssembly((uint8_t *)", ");
    emitAssembly(numberBuffer);
    emitAssembly((uint8_t *)"\n");
    emitAssembly((uint8_t *)"\tjge ");
    emitAssembly(endLabel);
    emitAssembly((uint8_t *)"\n");
    parseStatementList();
    expectToken(RBRACE);
    emitAssembly((uint8_t *)"\tjmp ");
    emitAssembly(loopLabel);
    emitAssembly((uint8_t *)"\n");
    emitAssembly(endLabel);
    emitAssembly((uint8_t *)":\n");
}

// Parses a function call statement, handling arguments and generating assembly for pushing arguments, calling, and cleaning the stack.
// Distinguishes between defined functions (no args) and external calls (with args).
// Saves and restores caller-saved registers.
void parseFunctionCall()
{
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier for call");

    }
    uint8_t functionName[MAX_SYMBOL_NAME_LENGTH];
    strcpy(functionName, currentToken.value);
    getNextToken();
    expectToken(LPAREN);
    if (isFunctionDefined(functionName))
    {
        expectToken(RPAREN);
        emitAssembly((uint8_t *)"\tcall ");
        emitAssembly(functionName);
        emitAssembly((uint8_t *)"\n");
    }
    else
    {
        int argCount = 0;
        struct ExprInfo args[5];
        if (currentToken.type != RPAREN)
        {
            do
            {
                if (argCount >= 6)
                {
                    printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
                    printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Too many arguments");
                }
                parseExpressionInfo(&args[argCount]);
                argCount++;
                if (currentToken.type == COMMA)
                {
                    getNextToken();
                }
                else
                {
                    break;
                }
            } while (1);
        }
        expectToken(RPAREN);

        // These are the caller-saved for cdecl
        emitAssembly((uint8_t *)"\tpush eax\n");
        emitAssembly((uint8_t *)"\tpush ecx\n");
        emitAssembly((uint8_t *)"\tpush edx\n");
        for (int i = argCount - 1; i >= 0; i--)
        {
            emitAssembly(args[i].code);
            emitAssembly((uint8_t *)"\tmov eax, ");
            if (args[i].isImmediate)
            {
                emitAssembly(args[i].value);
            }
            else
            {
                emitAssembly(args[i].location);
            }
            emitAssembly((uint8_t *)"\n");
            emitAssembly((uint8_t *)"\tpush eax\n");
        }
        emitAssembly((uint8_t *)"\tcall ");
        emitAssembly(functionName);
        emitAssembly((uint8_t *)"\n");

        // Clean stack for arguments
        uint8_t cleanBuf[10];
        itoa(argCount * 4, cleanBuf);
        emitAssembly((uint8_t *)"\tadd esp, ");
        emitAssembly(cleanBuf);
        emitAssembly((uint8_t *)"\n");

        // Restoring the caller saved registers.
        emitAssembly((uint8_t *)"\tpop edx\n");
        emitAssembly((uint8_t *)"\tpop ecx\n");
        emitAssembly((uint8_t *)"\tpop eax\n");
    }
    expectToken(SEMICOLON);
}

// Parses a syscall statement and generates assembly to set up registers and invoke the interrupt.
// Expects the form syscall(number, expr1, expr2);
void parseSyscallStatement()
{
    expectToken(SYSCALL);
    expectToken(LPAREN);
    if (currentToken.type != NUMBER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected number");
    }
    emitAssembly((uint8_t *)"\tmov eax, ");
    emitAssembly(currentToken.value);
    emitAssembly((uint8_t *)"\n");
    getNextToken();
    expectToken(COMMA);
    struct ExprInfo info1;
    parseExpressionInfo(&info1);
    emitAssembly(info1.code);
    emitAssembly((uint8_t *)"\tmov ebx, ");
    if (info1.isImmediate)
    {
        emitAssembly(info1.value);
    }
    else
    {
        emitAssembly(info1.location);
    }
    emitAssembly((uint8_t *)"\n");
    expectToken(COMMA);
    struct ExprInfo info2;
    parseExpressionInfo(&info2);
    emitAssembly(info2.code);
    emitAssembly((uint8_t *)"\tmov ecx, ");
    if (info2.isImmediate)
    {
        emitAssembly(info2.value);
    }
    else
    {
        emitAssembly(info2.location);
    }
    emitAssembly((uint8_t *)"\n");
    emitAssembly((uint8_t *)"\tpusha\n");
    emitAssembly((uint8_t *)"\tint 0x80\n");
    emitAssembly((uint8_t *)"\tpopa\n");
    expectToken(RPAREN);
    expectToken(SEMICOLON);
}

// Parses a single statement, dispatching to specific parsers based on the current token.
// Handles declarations, assignments, deref assignments, returns, ifs, whiles, function calls, and syscalls.
// Errors on invalid statements.
void parseStatement()
{
    if (currentToken.type == INT || currentToken.type == CHAR)
    {
        parseDeclaration();
    }
    else if (currentToken.type == IDENTIFIER)
    {
        if (peekNextNonSpace() == '=')
        {
            parseAssignment();
        }
        else if (peekNextNonSpace() == '(')
        {
            parseFunctionCall();
        }
        else
        {
            printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
            printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Invalid statement");
        }
    }
    else if (currentToken.type == STAR)
    {
        parseDerefAssignment();
    }
    else if (currentToken.type == RETURN)
    {
        parseReturnStatement();
    }
    else if (currentToken.type == IF)
    {
        parseIfStatement();
    }
    else if (currentToken.type == WHILE)
    {
        parseWhileStatement();
    }
    else if (currentToken.type == SYSCALL)
    {
        parseSyscallStatement();
    }
    else
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Invalid statement");
    }
}

// Parses a sequence of statements until a closing brace or end of input is encountered.
// Calls parseStatement repeatedly in a loop.


// You can think of this as the grammar rule-- StatementList: Statement | StatementList Statement
// This is not strictly recursive from a programming perspective since you don't see explicitly
// a function calling itself, but the fact that in parseStatementList() there is a while loop,
// calling parseStatement, this while loop shows the recursive grammar pattern in action.

void parseStatementList()
{
    while (currentToken.type != RBRACE && currentToken.type != END)
    {
        parseStatement();
    }
}

// Parses a void function definition, including name, empty parameters, and body.
// Resets symbol table, generates function label, parses statements, and adds prologue/epilogue if locals exist.
void parseFunctionDefinition()
{
    expectToken(VOID);
    if (currentToken.type != IDENTIFIER)
    {
        printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Expected identifier");
    }
    uint8_t functionName[MAX_SYMBOL_NAME_LENGTH];
    strcpy(functionName, currentToken.value);
    getNextToken();
    expectToken(LPAREN);
    expectToken(RPAREN);
    expectToken(LBRACE);
    symbolTableCount = 0;
    emitAssembly(functionName);
    emitAssembly((uint8_t *)":\n");
    uint32_t bodyStart = asmCodePosition;
    parseStatementList();
    expectToken(RBRACE);
    uint32_t bodyEnd = asmCodePosition;
    uint32_t bodySize = bodyEnd - bodyStart;
    int stackSize = symbolTableCount * 4;
    if (stackSize > 0)
    {
        uint8_t stackBuf[10];
        itoa(stackSize, stackBuf);
        uint32_t prologueLen = strlen((uint8_t *)"\tpush ebp\n\tmov ebp, esp\n\tsub esp, ") + strlen(stackBuf) + strlen((uint8_t *)"\n");
        // Shift body right
        for (uint32_t x = bodySize; x > 0; x--)
        {
            asmCodeBuffer[bodyStart + prologueLen + x - 1] = asmCodeBuffer[bodyStart + x - 1];
        }
        // Write prologue
        uint32_t pos = bodyStart;
        strcpy(asmCodeBuffer + pos, (uint8_t *)"\tpush ebp\n");
        pos += strlen((uint8_t *)"\tpush ebp\n");
        strcpy(asmCodeBuffer + pos, (uint8_t *)"\tmov ebp, esp\n");
        pos += strlen((uint8_t *)"\tmov ebp, esp\n");
        strcpy(asmCodeBuffer + pos, (uint8_t *)"\tsub esp, ");
        pos += strlen((uint8_t *)"\tsub esp, ");
        strcpy(asmCodeBuffer + pos, stackBuf);
        pos += strlen(stackBuf);
        bytecpy(asmCodeBuffer + pos, (uint8_t *)"\n", 1);
        pos += 1;
        asmCodePosition += prologueLen;
        emitAssembly((uint8_t *)"\tmov esp, ebp\n");
        emitAssembly((uint8_t *)"\tpop ebp\n");
    }
    emitAssembly((uint8_t *)"\tret\n");
}

// Generates code for the main function (int main()), including label, body parsing, and prologue/epilogue if locals exist.
// Assumes the main is the entry point labeled as _start.
void generateMainCode()
{
    expectToken(INT);
    expectToken(FUNCTION);
    expectToken(LPAREN);
    expectToken(RPAREN);
    expectToken(LBRACE);
    symbolTableCount = 0;
    emitAssembly((uint8_t *)"_start:\n");
    uint32_t bodyStart = asmCodePosition;
    parseStatementList();
    expectToken(RBRACE);
    uint32_t bodyEnd = asmCodePosition;
    uint32_t bodySize = bodyEnd - bodyStart;
    int stackSize = symbolTableCount * 4;
    if (stackSize > 0)
    {
        uint8_t stackBuf[10];
        itoa(stackSize, stackBuf);
        uint32_t prologueLen = strlen((uint8_t *)"\tpush ebp\n\tmov ebp, esp\n\tsub esp, ") + strlen(stackBuf) + strlen((uint8_t *)"\n");
        // Shift body right
        for (uint32_t x = bodySize; x > 0; x--)
        {
            asmCodeBuffer[bodyStart + prologueLen + x - 1] = asmCodeBuffer[bodyStart + x - 1];
        }
        // Write prologue
        uint32_t pos = bodyStart;
        strcpy(asmCodeBuffer + pos, (uint8_t *)"\tpush ebp\n");
        pos += strlen((uint8_t *)"\tpush ebp\n");
        strcpy(asmCodeBuffer + pos, (uint8_t *)"\tmov ebp, esp\n");
        pos += strlen((uint8_t *)"\tmov ebp, esp\n");
        strcpy(asmCodeBuffer + pos, (uint8_t *)"\tsub esp, ");
        pos += strlen((uint8_t *)"\tsub esp, ");
        strcpy(asmCodeBuffer + pos, stackBuf);
        pos += strlen(stackBuf);
        bytecpy(asmCodeBuffer + pos, (uint8_t *)"\n", 1);
        pos += 1;
        asmCodePosition += prologueLen;
        emitAssembly((uint8_t *)"\tmov esp, ebp\n");
        emitAssembly((uint8_t *)"\tpop ebp\n");
    }
    emitAssembly((uint8_t *)"\tret\n");
}

// Parses the entire program, handling top-level function definitions and the main function.
// Loops until end of input, dispatching to parseFunctionDefinition or generateMainCode.
// Errors on invalid top-level declarations.
void parseProgram()
{
    while (currentToken.type != END)
    {
        if (currentToken.type == VOID)
        {
            parseFunctionDefinition();
        }
        else if (currentToken.type == INT)
        {
            generateMainCode();
        }
        else
        {
            printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
            printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Invalid top-level declaration");
        }
    }
}

// Performs a pre-pass over the source code to collect names of all defined functions.
// Saves and restores parser state to avoid interfering with main parsing.
// Limits the number of functions and errors if exceeded.
void collectDefinedFunctions()
{
    int savePosition = sourcePosition;
    int saveLine = currentLineNumber;
    struct Token saveToken = currentToken;
    sourcePosition = 0;
    currentLineNumber = 1;
    getNextToken();
    definedFunctionCount = 0;
    while (currentToken.type != END)
    {
        if (currentToken.type == VOID)
        {
            getNextToken();
            if (currentToken.type == IDENTIFIER)
            {
                strcpy(definedFunctionNames[definedFunctionCount], currentToken.value);
                definedFunctionCount++;
                if (definedFunctionCount >= MAX_SYMBOLS)
                {
                    printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
                    printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Too many functions");
                }
            }
            while (currentToken.type != LBRACE && currentToken.type != END)
            {
                getNextToken();
            }
            if (currentToken.type == LBRACE)
            {
                int braceCount = 1;
                getNextToken();
                while (braceCount > 0 && currentToken.type != END)
                {
                    if (currentToken.type == LBRACE) braceCount++;
                    else if (currentToken.type == RBRACE) braceCount--;
                    getNextToken();
                }
            }
        }
        else if (currentToken.type == INT)
        {
            getNextToken();
            if (currentToken.type == FUNCTION)
            {
                strcpy(definedFunctionNames[definedFunctionCount], (uint8_t *)"main");
                definedFunctionCount++;
                if (definedFunctionCount >= MAX_SYMBOLS)
                {
                    printHexNumber(COLOR_RED, currentPrintRow, 1, currentLineNumber);
                    printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Too many functions");
                }
            }
            while (currentToken.type != LBRACE && currentToken.type != END)
            {
                getNextToken();
            }
            if (currentToken.type == LBRACE)
            {
                int braceCount = 1;
                getNextToken();
                while (braceCount > 0 && currentToken.type != END)
                {
                    if (currentToken.type == LBRACE) braceCount++;
                    else if (currentToken.type == RBRACE) braceCount--;
                    getNextToken();
                }
            }
        }
        else
        {
            getNextToken();
        }
    }
    sourcePosition = savePosition;
    currentLineNumber = saveLine;
    currentToken = saveToken;
}

// Compiles the provided source code into assembly.
// Initializes globals, collects defined functions, parses the program, and assembles the output with data and text sections.
void compileSource(uint8_t *sourceCode)
{
    sourceCodeInput = sourceCode;
    sourcePosition = 0;
    asmCodePosition = 0;
    asmCodeBuffer[0] = '\0';
    dataSectionPosition = 0;
    dataSectionBuffer[0] = '\0';
    symbolTableCount = 0;
    dataSymbolCount = 0;
    currentLineNumber = 1;
    definedFunctionCount = 0;
    collectDefinedFunctions();
    getNextToken();
    parseProgram();

    uint8_t dataHeader[] = "section .data\n";
    uint8_t textHeader[] = "\nsection .text\n";
    int dataHLen = strlen(dataHeader);
    int textHLen = strlen(textHeader);
    int textLen = asmCodePosition;
    int dataLen = dataSectionPosition;

    for (uint32_t x = textLen; x > 0; x--)
    {
        asmCodeBuffer[dataHLen + dataLen + textHLen + x - 1] = asmCodeBuffer[x - 1];
    }
    
    asmCodePosition = 0;
    if (dataLen > 0)
    {
        bytecpy(asmCodeBuffer + asmCodePosition, dataHeader, dataHLen);
        asmCodePosition += dataHLen;
        bytecpy(asmCodeBuffer + asmCodePosition, dataSectionBuffer, dataLen);
        asmCodePosition += dataLen;
    }
    bytecpy(asmCodeBuffer + asmCodePosition, textHeader, textHLen);
    asmCodePosition += textHLen;
    asmCodePosition += textLen;
}

// The main entry point of the compiler program.
// Handles file operations: opening input, creating temp file, compiling, writing assembly, and cleaning up.
// Prints status messages and exits with appropriate codes on success or failure.
void main()
{
    uint32_t myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    uint32_t inputFileDescriptor;
    uint32_t tempFileDescriptor;
    uint32_t outputFileDescriptor;
    uint32_t inputFilePointer;
    uint32_t tempFilePointer;
    uint32_t returnValue = SYSCALL_FAIL;

    clearScreen();
    currentPrintRow++;

    printString(COLOR_LIGHT_BLUE, currentPrintRow++, 5, (uint8_t *)"--------- ikanOS Compiler v1 --------- ");
    currentPrintRow++;
    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Opening input file: ");
    printString(COLOR_WHITE, currentPrintRow-1, 25, (uint8_t*)(COMMAND_BUFFER + 3));
    
    returnValue = systemOpenFile((uint8_t*)(COMMAND_BUFFER + 3), RDONLY);
    if (returnValue == SYSCALL_FAIL)
    {
        currentPrintRow++;
        printString(COLOR_RED, currentPrintRow++, 5, (uint8_t *)"Failed to open input file");
        wait(3);
        systemExit(PROCESS_EXIT_CODE_INPUT_FILE_FAILED);
    }
    returnValue = SYSCALL_FAIL; //reset for next one.

    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    inputFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    inputFilePointer = (uint32_t)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    // Remove the "\n" in this string if you want another argument after this in the 
    // COMMAND_BUFFER.

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Creating temp file.");
    systemOpenEmptyFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".cc.tmp\n"), 1);
    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFilePointer = (uint32_t)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    systemCloseFile(tempFileDescriptor);
    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFilePointer = (uint32_t)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Opening temp file");
    systemOpenFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".cc.tmp\n"), RDWRITE);
    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFilePointer = (uint32_t)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    uint8_t *inputStart = (uint8_t*)inputFilePointer;
    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Compiling...");
    compileSource(inputStart);
    currentPrintRow++;
    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Writing code buffer to temp");
    bytecpy((uint8_t*)tempFilePointer, asmCodeBuffer, asmCodePosition + 1);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Saving progtmp buffer to assembly file");
    systemCreateFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".s"), tempFileDescriptor);
    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    outputFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);

    systemCloseFile(tempFileDescriptor);
    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    tempFileDescriptor = readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR);
    tempFilePointer = (uint32_t)readValueFromMemLoc(CURRENT_FILE_DESCRIPTOR_PTR);

    printString(COLOR_WHITE, currentPrintRow++, 5, (uint8_t *)"Deleting temp file");
    systemDeleteFile(strConcat(removeExtension((uint8_t*)(COMMAND_BUFFER + 3)), (uint8_t*)".cc.tmp"));
    myProcessId = readValueFromMemLoc(RUNNING_PID_LOC);
    
    currentPrintRow++;
    printString(COLOR_LIGHT_BLUE, currentPrintRow++, 5, (uint8_t *)"--------- Complete. Exiting. --------- ");
    
    wait(1);

    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}