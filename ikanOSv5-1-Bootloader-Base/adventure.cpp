// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


// Modified text adventure reduced to 30 rooms.

// Winning sequence of steps:
// 1. go east (enter building, room 2)
// 2. take keys
// 3. take lamp (optional, but recommended for immersion)
// 4. go west (back to room 0)
// 5. go south (to room 3)
// 6. go south (to room 6)
// 7. go down (to room 7)
// 8. unlock grate
// 9. go down (to room 8)
// 10. go west (to room 9)
// 11. take cage
// 12. go west (to room 10)
// 13. go west (to room 11)
// 14. go west (to room 12)
// 15. take pyramid
// 16. take bird
// 17. go west (to room 13)
// 18. take emerald
// 19. go down (to room 14)
// 20. go west (to room 16)
// 21. go north (to room 15)
// 22. take pillow
// 23. go down (to room 14)
// 24. go down (to room 17)
// 25. take diamonds
// 26. go up (to room 14)
// 27. go up (to room 13)
// 28. go east (to room 12)
// 29. go east (to room 11)
// 30. go east (to room 10)
// 31. xyzzy (teleport to room 2)
// 32. drop pyramid
// 33. drop emerald
// 34. drop diamonds
// 35. drop pillow
// 36. plugh (teleport to room 29)
// 37. go west (to room 18)
// 38. take gold
// 39. take silver
// 40. take jewelry
// 41. take coins
// 42. go west (to room 26)
// 43. take trident
// 44. go east (back to room 18)
// 45. go east (to room 21)
// 46. go east (to room 22)
// 47. go down (to room 23)
// 48. take eggs
// 49. go east (to room 24)
// 50. go north (to room 25)
// 51. take vase
// 52. go south (back to room 24)
// 53. go south (to room 29)
// 54. plugh (teleport to room 2)
// 55. drop gold
// 56. drop silver
// 57. drop jewelry
// 58. drop coins
// 59. drop trident
// 60. drop eggs
// 61. drop vase (pillow prevents breaking)
// Now all 10 treasures are in room 2, you win!

#include "screen.h"
#include "keyboard.h"
#include "libc-main.h"
#include "constants.h"
#include "x86.h"

#define MAX_LINE_LENGTH 80
#define MAX_RESPONSE_LENGTH 4096
#define SCREEN_ROWS 50
#define MAX_DISPLAY_ROW 35

// Function to print the command prompt on the screen at the specified row.
// This displays a green "> " prompt to indicate where the user should enter input.
// Parameters:
// - row: The row on the screen where the prompt should be printed.
// Returns: None.
void printCommandPrompt(uint32_t row)
{
    printString(COLOR_GREEN, row, 0, (uint8_t *)"> ");
}

// Function to get the starting column position for user input after the command prompt.
// This calculates the column immediately following the "> " prompt.
// Parameters: None.
// Returns: The column index where input should begin (which is 2).
uint32_t getInputColumn()
{
    return 2; // Length of "> "
}

// Function to print a string on the screen with word wrapping, handling newlines and carriage returns.
// It prints character by character, wrapping to the next row when the column reaches 80.
// Parameters:
// - color: The color code for the text to be printed.
// - start_row: The starting row on the screen.
// - start_column: The starting column on the screen.
// - message: Pointer to the null-terminated string to print.
// Returns: The next available row after printing (accounting for wrapping).
uint32_t printWrappedString(uint32_t color, uint32_t start_row, uint32_t start_column, uint8_t *message)
{
    uint32_t currentRow = start_row;
    uint32_t currentColumn = start_column;
    while (*message != 0x00)
    {
        if (*message == 0x0a)
        {
            currentRow++;
            currentColumn = 0;
            message++;
        }
        else if (*message == 0x0d)
        {
            message++;
        }
        else
        {
            printCharacter(color, currentRow, currentColumn, message);
            message++;
            currentColumn++;
            if (currentColumn >= 80)
            {
                currentColumn = 0;
                currentRow++;
            }
        }
    }
    return (currentColumn == 0) ? currentRow : currentRow + 1;
}

// Function to read a line of input from the keyboard, handling key presses, backspace, and shift.
// It displays the input on the screen in real-time and stops when Enter is pressed.
// Supports basic editing with backspace but ignores paging keys if handlePaging is true.
// Parameters:
// - buffer: Pointer to the buffer where the input string will be stored (null-terminated).
// - row: The row on the screen where input is displayed.
// - column: The starting column for input display.
// - handlePaging: Flag to indicate if Page Up/Down should be handled (currently ignored).
// Returns: The length of the input string (excluding null terminator).
uint32_t readInputLine(uint8_t *buffer, uint32_t row, uint32_t column, uint32_t handlePaging)
{
    uint32_t inputLength = 0;
    uint32_t cursorColumn = column;
    uint8_t isShiftPressed = 0;
    moveCursor(row, cursorColumn);
    while (true)
    {
        uint8_t tempScanCode;
        uint8_t scanCode;
        do
        {
            readKeyboard(&tempScanCode);
            if (tempScanCode == 0x2A || tempScanCode == 0x36)
            {
                isShiftPressed = 1;
                continue;
            }
            if (tempScanCode == 0xAA || tempScanCode == 0xB6)
            {
                isShiftPressed = 0;
                continue;
            }
            if (tempScanCode & 0x80)
            {
                continue;
            }
            scanCode = tempScanCode;
            break;
        } while (1);

        if (scanCode == 0x1C)
        { // Enter key
            buffer[inputLength] = 0;
            break;
        }
        else if (scanCode == 0x0E)
        { // Backspace key
            if (inputLength > 0)
            {
                inputLength--;
                cursorColumn--;
                uint8_t spaceCharacter = 0x20;
                printCharacter(COLOR_WHITE, row, cursorColumn, &spaceCharacter);
                moveCursor(row, cursorColumn);
            }
        }
        else if (handlePaging && scanCode == 0x49)
        { // Page Up
            continue;
        }
        else if (handlePaging && scanCode == 0x51)
        { // Page Down
            continue;
        }
        else
        {
            uint8_t inputChar = 0;
            if (scanCode == 0x10) inputChar = isShiftPressed ? 0x51 : 0x71;
            else if (scanCode == 0x11) inputChar = isShiftPressed ? 0x57 : 0x77;
            else if (scanCode == 0x12) inputChar = isShiftPressed ? 0x45 : 0x65;
            else if (scanCode == 0x13) inputChar = isShiftPressed ? 0x52 : 0x72;
            else if (scanCode == 0x14) inputChar = isShiftPressed ? 0x54 : 0x74;
            else if (scanCode == 0x15) inputChar = isShiftPressed ? 0x59 : 0x79;
            else if (scanCode == 0x16) inputChar = isShiftPressed ? 0x55 : 0x75;
            else if (scanCode == 0x17) inputChar = isShiftPressed ? 0x49 : 0x69;
            else if (scanCode == 0x18) inputChar = isShiftPressed ? 0x4F : 0x6F;
            else if (scanCode == 0x19) inputChar = isShiftPressed ? 0x50 : 0x70;
            else if (scanCode == 0x1E) inputChar = isShiftPressed ? 0x41 : 0x61;
            else if (scanCode == 0x1F) inputChar = isShiftPressed ? 0x53 : 0x73;
            else if (scanCode == 0x20) inputChar = isShiftPressed ? 0x44 : 0x64;
            else if (scanCode == 0x21) inputChar = isShiftPressed ? 0x46 : 0x66;
            else if (scanCode == 0x22) inputChar = isShiftPressed ? 0x47 : 0x67;
            else if (scanCode == 0x23) inputChar = isShiftPressed ? 0x48 : 0x68;
            else if (scanCode == 0x24) inputChar = isShiftPressed ? 0x4A : 0x6A;
            else if (scanCode == 0x25) inputChar = isShiftPressed ? 0x4B : 0x6B;
            else if (scanCode == 0x26) inputChar = isShiftPressed ? 0x4C : 0x6C;
            else if (scanCode == 0x2C) inputChar = isShiftPressed ? 0x5A : 0x7A;
            else if (scanCode == 0x2D) inputChar = isShiftPressed ? 0x58 : 0x78;
            else if (scanCode == 0x2E) inputChar = isShiftPressed ? 0x43 : 0x63;
            else if (scanCode == 0x2F) inputChar = isShiftPressed ? 0x56 : 0x76;
            else if (scanCode == 0x30) inputChar = isShiftPressed ? 0x42 : 0x62;
            else if (scanCode == 0x31) inputChar = isShiftPressed ? 0x4E : 0x6E;
            else if (scanCode == 0x32) inputChar = isShiftPressed ? 0x4D : 0x6D;
            else if (scanCode == 0x02) inputChar = isShiftPressed ? 0x21 : 0x31;
            else if (scanCode == 0x03) inputChar = isShiftPressed ? 0x40 : 0x32;
            else if (scanCode == 0x04) inputChar = isShiftPressed ? 0x23 : 0x33;
            else if (scanCode == 0x05) inputChar = isShiftPressed ? 0x24 : 0x34;
            else if (scanCode == 0x06) inputChar = isShiftPressed ? 0x25 : 0x35;
            else if (scanCode == 0x07) inputChar = isShiftPressed ? 0x5E : 0x36;
            else if (scanCode == 0x08) inputChar = isShiftPressed ? 0x26 : 0x37;
            else if (scanCode == 0x09) inputChar = isShiftPressed ? 0x2A : 0x38;
            else if (scanCode == 0x0A) inputChar = isShiftPressed ? 0x28 : 0x39;
            else if (scanCode == 0x0B) inputChar = isShiftPressed ? 0x29 : 0x30;
            else if (scanCode == 0x0C) inputChar = isShiftPressed ? 0x5F : 0x2D;
            else if (scanCode == 0x0D) inputChar = isShiftPressed ? 0x2B : 0x3D;
            else if (scanCode == 0x1A) inputChar = isShiftPressed ? 0x7B : 0x5B;
            else if (scanCode == 0x1B) inputChar = isShiftPressed ? 0x7D : 0x5D;
            else if (scanCode == 0x2B) inputChar = isShiftPressed ? 0x7C : 0x5C;
            else if (scanCode == 0x33) inputChar = isShiftPressed ? 0x3C : 0x2C;
            else if (scanCode == 0x34) inputChar = isShiftPressed ? 0x3E : 0x2E;
            else if (scanCode == 0x35) inputChar = isShiftPressed ? 0x3F : 0x2F;
            else if (scanCode == 0x27) inputChar = isShiftPressed ? 0x3A : 0x3B;
            else if (scanCode == 0x28) inputChar = isShiftPressed ? 0x22 : 0x27;
            else if (scanCode == 0x39) inputChar = 0x20;
            if (inputChar != 0 && inputLength < MAX_LINE_LENGTH)
            {
                buffer[inputLength] = inputChar;
                inputLength++;
                printCharacter(COLOR_WHITE, row, cursorColumn, &inputChar);
                cursorColumn++;
                moveCursor(row, cursorColumn);
            }
        }
    }
    return inputLength;
}

// Function to convert a string to uppercase by copying it to a destination buffer.
// It processes each character, converting lowercase letters to uppercase while leaving others unchanged.
// Parameters:
// - dest: Pointer to the destination buffer where the uppercase string will be stored.
// - src: Pointer to the source null-terminated string.
// Returns: None (modifies dest in place).
void toUpper(uint8_t *dest, uint8_t *src)
{
    while (*src)
    {
        *dest = (*src >= 'a' && *src <= 'z') ? *src - 32 : *src;
        src++;
        dest++;
    }
    *dest = 0;
}

// Game definitions
#define NUM_LOCATIONS 30
#define NUM_OBJECTS 20
#define NUM_DIRECTIONS 6

uint8_t *directionNames[NUM_DIRECTIONS] = {
    (uint8_t *)"north",
    (uint8_t *)"south",
    (uint8_t *)"east",
    (uint8_t *)"west",
    (uint8_t *)"up",
    (uint8_t *)"down"
};

uint8_t *locationDescriptions[NUM_LOCATIONS] = {
    (uint8_t *)"You are standing at the end of a road before a small brick building. Around you is a forest. A small stream flows out of the building and down a gully.",
    (uint8_t *)"You are on a hill in the forest. The road slopes down to a building in the distance.",
    (uint8_t *)"You are inside a building, a well house for a large spring.",
    (uint8_t *)"You are in a valley in the forest beside a stream tumbling along a rocky bed.",
    (uint8_t *)"You are in open forest, with a deep valley to one side.",
    (uint8_t *)"You are in open forest near both a valley and a road.",
    (uint8_t *)"At your feet all the water of the stream splashes into a 2-inch slit in the rock. Downstream the streambed is bare rock.",
    (uint8_t *)"You are in a 20-foot depression with a steel grate set in concrete. A dry streambed leads in.",
    (uint8_t *)"You are in a small chamber beneath a 3x3 steel grate. A low crawl leads west.",
    (uint8_t *)"You are crawling over cobbles in a low passage with dim light to the east.",
    (uint8_t *)"You are in a debris room filled with stuff from the surface. A canyon leads west. A note says 'magic word xyzzy'.",
    (uint8_t *)"You are in a sloping east/west canyon.",
    (uint8_t *)"You are in a splendid chamber with orange stone walls. Passages exit east and west.",
    (uint8_t *)"At your feet is a small pit with white mist. An east passage ends here except for a small crack.",
    (uint8_t *)"You are at one end of a vast hall with mist and a staircase leading down. A passage is behind you.",
    (uint8_t *)"Rough stone steps lead up a dome.",
    (uint8_t *)"You are on the east bank of a fissure in the hall. The mist is thick, and the fissure too wide to jump.",
    (uint8_t *)"You are in a low room with a note: 'you won't get it up the steps'.",
    (uint8_t *)"You are in the hall of the mountain king, with passages in all directions.",
    (uint8_t *)"You are in a small cave with a narrow passage to the south.", 
    (uint8_t *)"You are in a rocky chamber with exits to the north and west.", 
    (uint8_t *)"You are in a tight crawl with a passage opening to the east.", 
    (uint8_t *)"You are in a misty alcove with a path leading up.", 
    (uint8_t *)"You are in a cavern with sparkling walls and a passage to the east.", 
    (uint8_t *)"You are in a dusty passage with exits to the north and south.", 
    (uint8_t *)"You are in a low chamber with a delicate vase on a pedestal.", 
    (uint8_t *)"You are in a chamber with a jeweled trident embedded in stone.", 
    (uint8_t *)"You are in the west side chamber of the hall of the mountain king. A passage goes west.",
    (uint8_t *)"You can't get by the snake.", // Kept for snake block message
    (uint8_t *)"You are in a large room with a 'Y2' on a rock. Passages lead south and west." // Kept for plugh
};

int connections[NUM_LOCATIONS][NUM_DIRECTIONS] = {
    { -1, 3, 2, 1, -1, -1 }, // Room 0: end of road
    { -1, -1, 0, -1, -1, -1 }, // Room 1: hill
    { -1, -1, -1, 0, -1, -1 }, // Room 2: building
    { 0, 6, -1, 4, -1, -1 }, // Room 3: valley
    { -1, -1, 3, -1, -1, -1 }, // Room 4: forest
    { -1, -1, -1, -1, -1, -1 }, // Room 5: forest
    { 3, -1, -1, -1, -1, 7 }, // Room 6: slit
    { -1, -1, -1, -1, 6, 8 }, // Room 7: depression
    { -1, -1, -1, 9, 7, -1 }, // Room 8: chamber
    { -1, -1, 8, 10, -1, -1 }, // Room 9: cobbles
    { -1, -1, 9, 11, -1, -1 }, // Room 10: debris
    { -1, -1, 10, 12, -1, -1 }, // Room 11: canyon
    { -1, -1, 11, 13, -1, -1 }, // Room 12: splendid chamber
    { -1, -1, 12, -1, -1, 14 }, // Room 13: pit
    { -1, -1, -1, 16, 13, 17 }, // Room 14: hall of mists
    { -1, -1, -1, -1, -1, 14 }, // Room 15: dome steps
    { 15, 8, -1, -1, -1, -1 }, // Room 16: fissure
    { 15, -1, -1, -1, 14, -1 }, // Room 17: low room
    { 15, 28, 11, 26, 27, 29 }, // Room 18: hall of mt king
    { 18, 20, -1, -1, -1, -1 }, // Room 19: small cave
    { -1, -1, -1, 19, -1, -1 }, // Room 20: rocky chamber
    { -1, -1, 22, -1, -1, -1 }, // Room 21: tight crawl
    { -1, -1, -1, 21, -1, 23 }, // Room 22: misty alcove
    { -1, -1, 24, -1, -1, -1 }, // Room 23: cavern
    { 25, 29, -1, -1, -1, -1 }, // Room 24: dusty passage
    { -1, 24, -1, -1, -1, -1 }, // Room 25: vase chamber
    { -1, -1, 18, -1, -1, -1 }, // Room 26: trident chamber
    { -1, -1, -1, -1, -1, 18 }, // Room 27: west chamber
    { -1, 18, -1, -1, -1, -1 }, // Room 28: snake block
    { 24, -1, -1, 18, -1, -1 }  // Room 29: Y2
};

struct Object {
    uint8_t *name;
    uint8_t *description;
    int location;
};

Object objects[NUM_OBJECTS] = {
    {(uint8_t *)"lamp", (uint8_t *)"A shiny brass lamp.", 2},
    {(uint8_t *)"keys", (uint8_t *)"A set of keys.", 2},
    {(uint8_t *)"bottle", (uint8_t *)"A clear glass bottle.", 2},
    {(uint8_t *)"food", (uint8_t *)"Tasty food.", 2},
    {(uint8_t *)"cage", (uint8_t *)"A small wicker cage.", 9},
    {(uint8_t *)"bird", (uint8_t *)"A cheerful little bird.", 12},
    {(uint8_t *)"rod", (uint8_t *)"A black rod with a rusty star.", 10},
    {(uint8_t *)"pillow", (uint8_t *)"A velvet pillow.", 15},
    {(uint8_t *)"snake", (uint8_t *)"A huge fierce green snake bars the way!", 18},
    {(uint8_t *)"gold", (uint8_t *)"A large sparkling nugget of gold.", 18},
    {(uint8_t *)"diamonds", (uint8_t *)"Several diamonds.", 17},
    {(uint8_t *)"silver", (uint8_t *)"Bars of silver.", 18},
    {(uint8_t *)"jewelry", (uint8_t *)"Precious jewelry.", 18},
    {(uint8_t *)"coins", (uint8_t *)"Rare coins.", 18},
    {(uint8_t *)"chest", (uint8_t *)"A pirate's treasure chest.", -1},
    {(uint8_t *)"eggs", (uint8_t *)"Golden eggs.", 23}, 
    {(uint8_t *)"trident", (uint8_t *)"A jeweled trident.", 26}, 
    {(uint8_t *)"vase", (uint8_t *)"A delicate, precious ming vase.", 25}, 
    {(uint8_t *)"emerald", (uint8_t *)"An emerald the size of a plover's egg.", 13},
    {(uint8_t *)"pyramid", (uint8_t *)"A platinum pyramid.", 12}
};

bool grateLocked = true;
bool vaseBroken = false;
int playerLocation = 0;
bool visited[NUM_LOCATIONS];

// Function to find an object by its name in the objects array.
// It searches through all objects and returns the index if a match is found.
// Parameters:
// - name: Pointer to the null-terminated string of the object name to search for.
// Returns: The index of the object if found, otherwise -1.
int findObject(uint8_t *name)
{
    for (int i = 0; i < NUM_OBJECTS; i++)
    {
        if (strcmp(objects[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Function to print the description of the current player location and any visible objects there.
// It uses printWrappedString to display the location description and lists objects (excluding the snake, which is handled specially).
// Parameters:
// - start_row: The starting row on the screen to begin printing.
// Returns: The next available row after printing all descriptions.
uint32_t printLocation(uint32_t start_row)
{
    uint32_t nextRow = printWrappedString(COLOR_WHITE, start_row, 0, locationDescriptions[playerLocation]);

    for (int i = 0; i < NUM_OBJECTS; i++)
    {
        if (objects[i].location == playerLocation && i != 8)
        {
            printWrappedString(COLOR_WHITE, nextRow, 0, (uint8_t *)"There is a ");
            uint32_t currentColumn = strlen((uint8_t *)"There is a ");
            uint8_t upperName[20];
            toUpper(upperName, objects[i].name);
            printWrappedString(COLOR_WHITE, nextRow, currentColumn, upperName);
            currentColumn += strlen(upperName);
            printWrappedString(COLOR_WHITE, nextRow, currentColumn, (uint8_t *)" here.");
            nextRow++;
        }
        else if (objects[i].location == playerLocation && i == 8)
        {
            printWrappedString(COLOR_WHITE, nextRow, 0, (uint8_t *)"There is a ");
            uint32_t currentColumn = strlen((uint8_t *)"There is a ");
            printWrappedString(COLOR_WHITE, nextRow, currentColumn, objects[i].name);
            currentColumn += strlen(objects[i].name);
            printWrappedString(COLOR_WHITE, nextRow, currentColumn, (uint8_t *)" here.");
            nextRow++;
        }
    }
    return nextRow;
}

// Main function that runs the text adventure game loop.
// It initializes the game state, processes user commands in a loop, updates the screen, and handles game logic like movement, item interactions, and win conditions.
// Parameters: None.
// Returns: None (exits the program when quit or win condition is met).
void main()
{
    uint8_t inputBuffer[MAX_LINE_LENGTH + 1];
    uint8_t responseMessage[MAX_RESPONSE_LENGTH];
    uint32_t currentRow = 35; // Force initial clear
    uint8_t hasResponseMessage = 0;
    uint8_t needToPrintLocation = 1;

    // Initialize visited
    for (int i = 0; i < NUM_LOCATIONS; i++)
    {
        visited[i] = false;
    }
    visited[0] = true;

    while (true)
    {
        if (currentRow >= 35)
        {
            clearScreen();
            printString(COLOR_LIGHT_BLUE, 0, 0, (uint8_t *)"Basic Text Adventure");
            currentRow = 2;
            needToPrintLocation = 1;
        }

        if (needToPrintLocation)
        {
            currentRow = printLocation(currentRow);
            needToPrintLocation = 0;
        }

        if (hasResponseMessage)
        {
            currentRow = printWrappedString(COLOR_GREEN, currentRow, 0, responseMessage);
            hasResponseMessage = 0;
        }

        printCommandPrompt(currentRow);
        uint32_t inputLength = readInputLine(inputBuffer, currentRow, getInputColumn(), 0);
        currentRow++;

        // Parse input
        uint8_t *commandWord = inputBuffer;
        uint8_t *objectOrDirection = 0;
        for (uint32_t i = 0; i < inputLength; i++)
        {
            if (inputBuffer[i] == ' ')
            {
                inputBuffer[i] = 0;
                objectOrDirection = &inputBuffer[i + 1];
                break;
            }
        }

        if (strcmp(commandWord, (uint8_t *)"go") == 0)
        {
            if (objectOrDirection == 0)
            {
                strcpy(responseMessage, (uint8_t *)"Go where?");
                hasResponseMessage = 1;
                continue;
            }
            int directionIndex = -1;
            for (int i = 0; i < NUM_DIRECTIONS; i++)
            {
                if (strcmp(objectOrDirection, directionNames[i]) == 0)
                {
                    directionIndex = i;
                    break;
                }
            }
            if (directionIndex == -1)
            {
                strcpy(responseMessage, (uint8_t *)"Invalid direction.");
                hasResponseMessage = 1;
                continue;
            }
            int newLocation = connections[playerLocation][directionIndex];
            if (newLocation == -1)
            {
                strcpy(responseMessage, (uint8_t *)"You can't go that way.");
                hasResponseMessage = 1;
                continue;
            }
            if (playerLocation == 7 && directionIndex == 5 && grateLocked)
            {
                strcpy(responseMessage, (uint8_t *)"The grate is locked.");
                hasResponseMessage = 1;
                continue;
            }
            if (playerLocation == 8 && directionIndex == 4 && grateLocked)
            {
                strcpy(responseMessage, (uint8_t *)"The grate is locked.");
                hasResponseMessage = 1;
                continue;
            }
            if (playerLocation == 18 && directionIndex == 1 && objects[8].location == playerLocation)
            {
                strcpy(responseMessage, (uint8_t *)"You can't get by the snake.");
                hasResponseMessage = 1;
                continue;
            }
            playerLocation = newLocation;
            visited[playerLocation] = true;
            needToPrintLocation = 1;
        }
        else if (strcmp(commandWord, (uint8_t *)"take") == 0)
        {
            if (objectOrDirection == 0)
            {
                strcpy(responseMessage, (uint8_t *)"Take what?");
                hasResponseMessage = 1;
                continue;
            }
            int objectIndex = findObject(objectOrDirection);
            if (objectIndex == -1)
            {
                strcpy(responseMessage, (uint8_t *)"No such object.");
                hasResponseMessage = 1;
                continue;
            }
            if (objects[objectIndex].location != playerLocation)
            {
                strcpy(responseMessage, (uint8_t *)"It's not here.");
                hasResponseMessage = 1;
                continue;
            }
            if (objectIndex == 5)
            {
                if (objects[6].location == -1)
                {
                    strcpy(responseMessage, (uint8_t *)"The bird is frightened by the rod.");
                    hasResponseMessage = 1;
                    continue;
                }
                if (objects[4].location != -1)
                {
                    strcpy(responseMessage, (uint8_t *)"You have nothing to put the bird in.");
                    hasResponseMessage = 1;
                    continue;
                }
            }
            objects[objectIndex].location = -1;
            strcpy(responseMessage, (uint8_t *)"Taken.");
            hasResponseMessage = 1;
            needToPrintLocation = 1;
        }
        else if (strcmp(commandWord, (uint8_t *)"drop") == 0)
        {
            if (objectOrDirection == 0)
            {
                strcpy(responseMessage, (uint8_t *)"Drop what?");
                hasResponseMessage = 1;
                continue;
            }
            int objectIndex = findObject(objectOrDirection);
            if (objectIndex == -1)
            {
                strcpy(responseMessage, (uint8_t *)"No such object.");
                hasResponseMessage = 1;
                continue;
            }
            if (objects[objectIndex].location != -1)
            {
                strcpy(responseMessage, (uint8_t *)"You don't have it.");
                hasResponseMessage = 1;
                continue;
            }
            responseMessage[0] = '\0';
            if (objectIndex == 17)
            {
                int pillowIndex = findObject((uint8_t *)"pillow");
                if (objects[pillowIndex].location != playerLocation)
                {
                    vaseBroken = true;
                    objects[objectIndex].location = -2;
                    strcpy(responseMessage, (uint8_t *)"The vase has shattered.");
                    hasResponseMessage = 1;
                    needToPrintLocation = 1;
                    continue;
                }
            }
            if (objectIndex == 5)
            {
                if (objects[8].location == playerLocation)
                {
                    objects[8].location = -2;
                    strcpy(responseMessage + strlen(responseMessage), (uint8_t *)"The bird attacks the snake and drives it away.\n");
                }
            }
            objects[objectIndex].location = playerLocation;
            strcpy(responseMessage + strlen(responseMessage), (uint8_t *)"Dropped.");
            hasResponseMessage = 1;
            needToPrintLocation = 1;
        }
        else if (strcmp(commandWord, (uint8_t *)"inventory") == 0)
        {
            strcpy(responseMessage, (uint8_t *)"Inventory:\n");
            uint32_t messagePosition = strlen(responseMessage);
            uint8_t hasItem = 0;
            for (int i = 0; i < NUM_OBJECTS; i++)
            {
                if (objects[i].location == -1)
                {
                    uint8_t upperName[20];
                    toUpper(upperName, objects[i].name);
                    strcpy(responseMessage + messagePosition, upperName);
                    messagePosition += strlen(upperName);
                    responseMessage[messagePosition++] = '\n';
                    hasItem = 1;
                }
            }
            if (!hasItem)
            {
                strcpy(responseMessage + messagePosition, (uint8_t *)"Nothing.\n");
                messagePosition += strlen((uint8_t *)"Nothing.\n");
            }
            responseMessage[messagePosition] = 0;
            hasResponseMessage = 1;
        }
        else if (strcmp(commandWord, (uint8_t *)"look") == 0)
        {
            needToPrintLocation = 1;
        }
        else if (strcmp(commandWord, (uint8_t *)"parent") == 0)
        {
            clearScreen();
            systemSwitchToParent();
            clearScreen();
        }
        else if (strcmp(commandWord, (uint8_t *)"quit") == 0)
        {
            break;
        }
        else if (strcmp(commandWord, (uint8_t *)"xyzzy") == 0)
        {
            if (playerLocation == 10)
            {
                playerLocation = 2;
                visited[playerLocation] = true;
                needToPrintLocation = 1;
            }
            else if (playerLocation == 2)
            {
                playerLocation = 10;
                visited[playerLocation] = true;
                needToPrintLocation = 1;
            }
            else
            {
                strcpy(responseMessage, (uint8_t *)"Nothing happens.");
                hasResponseMessage = 1;
            }
        }
        else if (strcmp(commandWord, (uint8_t *)"plugh") == 0)
        {
            if (playerLocation == 29)
            {
                playerLocation = 2;
                visited[playerLocation] = true;
                needToPrintLocation = 1;
            }
            else if (playerLocation == 2)
            {
                playerLocation = 29;
                visited[playerLocation] = true;
                needToPrintLocation = 1;
            }
            else
            {
                strcpy(responseMessage, (uint8_t *)"Nothing happens.");
                hasResponseMessage = 1;
            }
        }
        else if (strcmp(commandWord, (uint8_t *)"unlock") == 0 || strcmp(commandWord, (uint8_t *)"open") == 0)
        {
            if (objectOrDirection == 0)
            {
                strcpy(responseMessage, (uint8_t *)"Unlock what?");
                hasResponseMessage = 1;
                continue;
            }
            if (strcmp(objectOrDirection, (uint8_t *)"grate") == 0)
            {
                if (playerLocation == 7 || playerLocation == 8)
                {
                    int keysIndex = findObject((uint8_t *)"keys");
                    if (objects[keysIndex].location == -1)
                    {
                        grateLocked = false;
                        strcpy(responseMessage, (uint8_t *)"The grate is now unlocked.");
                        hasResponseMessage = 1;
                    }
                    else
                    {
                        strcpy(responseMessage, (uint8_t *)"You have no keys.");
                        hasResponseMessage = 1;
                    }
                }
                else
                {
                    strcpy(responseMessage, (uint8_t *)"No grate here.");
                    hasResponseMessage = 1;
                }
            }
            else
            {
                strcpy(responseMessage, (uint8_t *)"Can't unlock that.");
                hasResponseMessage = 1;
            }
        }
        else if (strcmp(commandWord, (uint8_t *)"lock") == 0)
        {
            if (objectOrDirection == 0)
            {
                strcpy(responseMessage, (uint8_t *)"Lock what?");
                hasResponseMessage = 1;
                continue;
            }
            if (strcmp(objectOrDirection, (uint8_t *)"grate") == 0)
            {
                if (playerLocation == 7 || playerLocation == 8)
                {
                    int keysIndex = findObject((uint8_t *)"keys");
                    if (objects[keysIndex].location == -1)
                    {
                        grateLocked = true;
                        strcpy(responseMessage, (uint8_t *)"The grate is now locked.");
                        hasResponseMessage = 1;
                    }
                    else
                    {
                        strcpy(responseMessage, (uint8_t *)"You have no keys.");
                        hasResponseMessage = 1;
                    }
                }
                else
                {
                    strcpy(responseMessage, (uint8_t *)"No grate here.");
                    hasResponseMessage = 1;
                }
            }
            else
            {
                strcpy(responseMessage, (uint8_t *)"Can't lock that.");
                hasResponseMessage = 1;
            }
        }
        else if (strcmp(commandWord, (uint8_t *)"map") == 0)
        {
            strcpy(responseMessage, (uint8_t *)"Explored Map:\n");
            uint32_t messagePosition = strlen(responseMessage);
            for (int roomIndex = 0; roomIndex < NUM_LOCATIONS; roomIndex++)
            {
                if (visited[roomIndex])
                {
                    uint8_t roomNumberString[10];
                    itoa(roomIndex, roomNumberString);
                    strcpy(responseMessage + messagePosition, (uint8_t *)"Room ");
                    messagePosition += strlen((uint8_t *)"Room ");
                    strcpy(responseMessage + messagePosition, roomNumberString);
                    messagePosition += strlen(roomNumberString);
                    if (roomIndex == playerLocation)
                    {
                        strcpy(responseMessage + messagePosition, (uint8_t *)"*");
                        messagePosition += 1;
                    }
                    strcpy(responseMessage + messagePosition, (uint8_t *)": ");
                    messagePosition += 2;
                    uint8_t hasConnection = 0;
                    for (int directionIndex = 0; directionIndex < NUM_DIRECTIONS; directionIndex++)
                    {
                        int connectedRoom = connections[roomIndex][directionIndex];
                        if (connectedRoom != -1 && visited[connectedRoom])
                        {
                            if (hasConnection)
                            {
                                strcpy(responseMessage + messagePosition, (uint8_t *)", ");
                                messagePosition += 2;
                            }
                            strcpy(responseMessage + messagePosition, directionNames[directionIndex]);
                            messagePosition += strlen(directionNames[directionIndex]);
                            strcpy(responseMessage + messagePosition, (uint8_t *)" -> ");
                            messagePosition += strlen((uint8_t *)" -> ");
                            itoa(connectedRoom, roomNumberString);
                            strcpy(responseMessage + messagePosition, roomNumberString);
                            messagePosition += strlen(roomNumberString);
                            hasConnection = 1;
                        }
                    }
                    responseMessage[messagePosition++] = '\n';
                }
            }
            responseMessage[messagePosition] = 0;
            hasResponseMessage = 1;
        }
        else if (strcmp(commandWord, (uint8_t *)"help") == 0)
        {
            strcpy(responseMessage, (uint8_t *)"Available commands:\n");
            uint32_t messagePosition = strlen(responseMessage);
            strcpy(responseMessage + messagePosition, (uint8_t *)"go <direction> (north, south, east, west, up, down)\n");
            messagePosition += strlen((uint8_t *)"go <direction> (north, south, east, west, up, down)\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"take <object>\n");
            messagePosition += strlen((uint8_t *)"take <object>\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"drop <object>\n");
            messagePosition += strlen((uint8_t *)"drop <object>\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"inventory\n");
            messagePosition += strlen((uint8_t *)"inventory\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"look\n");
            messagePosition += strlen((uint8_t *)"look\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"xyzzy\n");
            messagePosition += strlen((uint8_t *)"xyzzy\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"plugh\n");
            messagePosition += strlen((uint8_t *)"plugh\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"unlock/open grate\n");
            messagePosition += strlen((uint8_t *)"unlock/open grate\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"lock grate\n");
            messagePosition += strlen((uint8_t *)"lock grate\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"map\n");
            messagePosition += strlen((uint8_t *)"map\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"parent\n");
            messagePosition += strlen((uint8_t *)"parent\n");
            strcpy(responseMessage + messagePosition, (uint8_t *)"quit\n");
            messagePosition += strlen((uint8_t *)"quit\n");
            responseMessage[messagePosition] = 0;
            hasResponseMessage = 1;
        }
        else
        {
            strcpy(responseMessage, (uint8_t *)"Unknown command.");
            hasResponseMessage = 1;
        }

        // Check for win
        if (playerLocation == 2)
        {
            int treasureCount = 0;
            for (int i = 9; i < NUM_OBJECTS; i++)
            {
                if (i == 14) continue; // exclude chest
                if (objects[i].location == 2) treasureCount++;
            }
            if (treasureCount == 10)
            {
                strcpy(responseMessage, (uint8_t *)"You have collected all the treasures! You win!");
                hasResponseMessage = 1;
                currentRow = printWrappedString(COLOR_GREEN, currentRow, 0, responseMessage);
                break;
            }
        }
    }

    systemExit(PROCESS_EXIT_CODE_SUCCESS);
}