// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "screen.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"
#include "file.h"

#define MAX_LINE_LENGTH 80


// Custom strncmp implementation
int my_strncmp(uint8_t *s1, uint8_t *s2, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        if (s1[i] == 0) {
            return 0;
        }
    }
    return 0;
}

// Custom strstr implementation
uint8_t *my_strstr(uint8_t *haystack, uint8_t *needle) {
    uint32_t n_len = strlen(needle);
    if (n_len == 0) {
        return haystack;
    }
    for (uint8_t *p = haystack; *p != 0; p++) {
        if (my_strncmp(p, needle, n_len) == 0) {
            return p;
        }
    }
    return 0;
}

void main() 
{
    uint32_t currentPid = readValueFromMemLoc(RUNNING_PID_LOC);
    struct openBufferTable *openBufferTable = (struct openBufferTable*)OPEN_BUFFER_TABLE;
    uint8_t* pipeInFdString = malloc(currentPid, 20);
    uint8_t* pipeOutFdString = malloc(currentPid, 20);
    itoa(0, pipeInFdString);
    itoa(1, pipeOutFdString);

    // Get the search word from command arguments, similar to edlin.cpp
    uint8_t *searchWord = (uint8_t *)malloc(currentPid, MAX_LINE_LENGTH + 1);
    uint8_t *commandLine = (uint8_t *)COMMAND_BUFFER;
    uint8_t *pointer = commandLine;

    // Now pointer at the first argument (the search word)
    if (*pointer && *pointer != 0x0a) {
        uint32_t len = 0;
        uint8_t *p = pointer;
        while (*p != 0x0a && *p != 0) { len++; p++; }
        bytecpy(searchWord, pointer, len);
        searchWord[len] = 0;
    } else {
        // No search word provided, exit or handle error (assume provided for simplicity)
        searchWord[0] = 0;
    }

    systemMMap((uint8_t*)openBufferTable->buffers[0]);
    systemMMap((uint8_t*)openBufferTable->buffers[1]);

    systemReadPipe(pipeInFdString);

    // Process the input buffer
    uint8_t *input = (uint8_t*)openBufferTable->buffers[0];
    uint32_t input_size = openBufferTable->bufferSize[0];
    uint8_t *output = (uint8_t*)openBufferTable->buffers[1];
    uint32_t out_pos = 0;

    // Parse lines and filter
    uint8_t *start = input;
    while (start < input + input_size) {
        uint8_t *end = start;
        while (end < input + input_size && *end != 0x0a) end++;
        uint32_t line_len = end - start;

        // Temporarily null-terminate the line for string functions
        uint8_t temp = start[line_len];
        start[line_len] = 0;

        // Check if the line contains the search word
        if (my_strstr(start, searchWord) != 0) {
            // Copy the line to output
            bytecpy(output + out_pos, start, line_len);
            out_pos += line_len;

            // Add a new line since each row in the output will be a hit for the 
            // search word
            output[out_pos++] = 0x0a;

        }

        // Restore the original byte
        start[line_len] = temp;

        // Move to next line
        start = end;
        if (start < input + input_size && *start == 0x0a) start++;
    }

    systemWritePipe(pipeOutFdString);

    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}