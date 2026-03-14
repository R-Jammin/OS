// Copyright (c) 2025-2026 Dan O’Malley
// This file is licensed under the MIT License. See LICENSE for details.
// Generated with Grok by xAI.
// 12/2025 with Grok v4.


#include "libc-main.h"
#include "constants.h"
#include "screen.h"
#include "x86.h"
#include "keyboard.h"

// Define constants for special key codes used in input handling
#define KEY_UP 1001
#define KEY_DOWN 1002
#define KEY_LEFT 1003
#define KEY_RIGHT 1004

// Define error code for invalid operations or inputs
#define ERR -1

// Extended ASCII characters for rendering walls and entities in the game map
#define WALL_VERTICAL 0xB3    // │ (vertical wall)
#define WALL_HORIZONTAL 0xC4  // ─ (horizontal wall)
#define WALL_UPPER_LEFT 0xDA  // ┌ (upper left corner)
#define WALL_UPPER_RIGHT 0xBF // ┐ (upper right corner)
#define WALL_LOWER_LEFT 0xC0  // └ (lower left corner)
#define WALL_LOWER_RIGHT 0xD9 // ┘ (lower right corner)
#define WALL_T_UP 0xC2        // ┬ (T junction facing up)
#define WALL_T_DOWN 0xC1      // ┴ (T junction facing down)
#define WALL_T_LEFT 0xB4      // ┤ (T junction facing left)
#define WALL_T_RIGHT 0xC3     // ├ (T junction facing right)
#define WALL_CROSS 0xC5       // ┼ (cross junction)
#define TREASURE_CHAR 0x04    // ♦ (treasure symbol)
#define KEY_CHAR 0x0C         // ♌ (key symbol, or alternative)
#define CROSS_CHAR 0xA8       // ¨ (cross symbol, or alternative like ☨)
#define SWORD_CHAR 0x86       // † (sword symbol)
#define POTION_CHAR 0x03      // ♥ (potion symbol for health)
#define STAIRS_DOWN 0x1F      // ▼ (down stairs symbol)
#define STAIRS_UP 0x1E        // ▲ (up stairs symbol)
#define NOTE_CHAR 0x0F        // ☏ (note symbol for story elements)

// Upper ASCII characters for monster symbols (values >127 for extended set)
#define GOBLIN_CHAR 0x82      // é (goblin placeholder)
#define VAMPIRE_CHAR 0xE9     // é (vampire symbol)
#define SKELETON_CHAR 0xE8    // è (skeleton symbol)
#define RAT_CHAR 0xE7         // ç (rat symbol)
#define DEMON_CHAR 0xD1       // Ñ (demon symbol)
#define WOLF_CHAR 0x84        // ä (wolf symbol)
#define GHOST_CHAR 0x85       // å (ghost symbol)
#define DRAGON_CHAR 0x81      // ü (dragon symbol)
#define BOSS_CHAR 0x80        // À (boss symbol)

// Structure to represent a point (coordinates) in the game map
struct Point
{
    int x, y;  // x and y coordinates
};

// Structure to represent a room in the dungeon level
struct Room
{
    int x, y, w, h;  // Position (x,y) and dimensions (width w, height h)
};

// Structure to represent a monster entity in the game
struct Monster
{
    int x, y, level;  // Position (x,y) and the dungeon level it belongs to
    uint8_t sym;      // Symbol (extended ASCII) for rendering the monster
    uint8_t color;    // Color code for rendering
    uint8_t alive;    // Flag indicating if the monster is alive (1) or dead (0)
    uint8_t isBoss;   // Flag indicating if this is a boss monster (1) or not (0)
    int hp;           // Hit points (health) of the monster
    int attack;       // Attack damage value of the monster
};

// Constant defining the total number of dungeon levels
const int numLevels = 3;
// Constants for the full map dimensions (height and width)
const int height = 160;  // Reduced height for smaller levels
const int width = 100;   // Reduced width for smaller levels
// Constants for the visible portion of the map on screen
const int visibleHeight = 20;  // Reduced to fit smaller levels
const int visibleWidth = 60;   // Reduced visible width in characters
// Starting row for rendering the map on screen
const int startRow = 5;
// Row for displaying status information
const int statusRow = 25;
// Maximum number of rooms per level
const int maxRooms = 30;
// Maximum number of monsters in the game
const int maxMonsters = 20;
// Player's maximum hit points
const int playerMaxHp = 100;
// Initial player hit points
int playerHp = 100;

// 3D array to store map data for all levels (level, y, x)
char mapData[3][160][101];
// Array of rooms for each level
struct Room rooms[3][30];
// Number of rooms generated per level
int numRooms[3];
// Positions of up stairs for each level
struct Point upStair[3];
// Positions of down stairs for each level
struct Point downStair[3];

// Player's current position (x, y)
int playerX, playerY;
// Number of treasures collected by the player
int treasures = 0;
// Total treasures placed in the dungeon
int totalTreasures = 0;
// Flags for whether the player has key items
uint8_t hasKey = 0;
uint8_t hasCross = 0;
uint8_t hasSword = 0;
// Current dungeon level the player is on
int currentLevel = 0;

// Array of monsters
struct Monster monsters[20];

// Array of story texts for each level, providing lore and atmosphere
uint8_t* levelStories[3] = {
    (uint8_t *)"Level 1: The Forgotten Gate - A once-grand entrance now shrouded in mystery and decay. Legend speaks of King Eldric, who sealed the castle after a plague of shadows consumed his court.",
    (uint8_t *)"Level 2: Halls of Echoes - Faint whispers tell tales of betrayal and lost glory. The echoes are said to be the voices of Eldric's advisors, cursed for their treachery.",
    (uint8_t *)"Level 3: Chambers of Sorrow - Spirits mourn the fall of an ancient kingdom. Queen Lira's chamber holds clues to the ritual that summoned the darkness."
};

// Array of story texts for items and notes, adding depth to the game's narrative
uint8_t* itemStories[20] = {
    (uint8_t *)"You found a rusty key. Legend says it unlocks the path to greater power, forged by the king's smith to seal the vaults.",
    (uint8_t *)"An ancient cross, said to ward off the undead horrors of the deep. Blessed by Queen Lira before her demise.",
    (uint8_t *)"A gleaming sword, forged in dragonfire, bane of beasts and shadows. Wielded by the hero who first challenged the Dark Lord.",
    (uint8_t *)"A cryptic note: 'The treasures hold the key to defeating the Dark Lord... but at what cost? Collect them all to reveal the ritual.'",
    (uint8_t *)"A glowing potion restores your vitality. Use wisely in the battles ahead. Brewed by alchemists to sustain the castle's defenders.",
    (uint8_t *)"An old scroll reveals: 'The curse began with a king's hubris, binding souls to this realm. Only blood of the royal line can break it.'",
    (uint8_t *)"A tattered journal entry: 'I saw the Dark Lord rise; only the three artifacts can seal him again. But the cross weakens vampires, the sword slays beasts.'",
    (uint8_t *)"Faded inscription: 'Beware the shifting walls; the labyrinth changes with each full moon, trapping unwary explorers.'",
    (uint8_t *)"Whispered clue: 'The potions hidden in sorrow's chambers restore life, but drink too many and madness ensues.'",
    (uint8_t *)"Ancient tablet: 'The ghosts seek release; offer them peace by destroying their cursed remains scattered below.'",
    (uint8_t *)"Blood-stained letter: 'My brother, the king, delved into forbidden magic, summoning shadows that consumed us all.'",
    (uint8_t *)"Mystic rune: 'The key opens not doors, but fates; use it at the throne to challenge destiny itself.'",
    (uint8_t *)"Seer's vision note: 'A hero will come bearing light's cross, fire's sword, and earth's key to end the eternal night.'",
    (uint8_t *)"Guard's log: 'Wolves prowl the depths, ghosts haunt the chambers; dragons guard the sanctum's fire.'",
    (uint8_t *)"Prophecy fragment: 'When treasures gleam no more, the curse lifts, but only if the Dark Lord's heart is pierced.'",
    (uint8_t *)"Alchemist's recipe: 'Potions heal, but overindulgence warps the flesh; limit thyself to survive the trials.'",
    (uint8_t *)"Traitor's confession: 'I betrayed the queen for power; now I wander as shadow, regretting eternally.'",
    (uint8_t *)"Hero's last words: 'The artifacts are scattered; find them before the madness claims you as it did me.'",
    (uint8_t *)"Lore book page: 'The castle was built on ley lines, amplifying magic; the Dark Lord draws strength from them.'",
    (uint8_t *)"Final clue: 'To defeat the boss, wield all artifacts; without them, doom awaits in the throne room.'"
};

// Static seed for the pseudo-random number generator
static uint32_t seed = 12345;

// Function to generate a pseudo-random number using a linear congruential generator
uint32_t myRand()
{
    // Update the seed using the formula for randomness
    seed = seed * 1103515245 + 12345;
    // Return a value in the range 0 to 32767
    return (seed / 65536) % 32768;
}

// Custom function to read a key press, handling arrow keys and shift
int getchCustom()
{
    uint8_t tempScan;
    uint8_t isShiftDown = 0;
    // Loop until a valid key is processed
    do
    {
        readKeyboard(&tempScan);
        // Handle shift key down
        if (tempScan == 0x2A || tempScan == 0x36)
        {
            isShiftDown = 1;
            continue;
        }
        // Handle shift key up
        if (tempScan == 0xAA || tempScan == 0xB6)
        {
            isShiftDown = 0;
            continue;
        }
        // Handle extended scan codes (for arrow keys)
        if (tempScan == 0xE0)
        {
            readKeyboard(&tempScan);
            // Ignore release codes
            if (tempScan & 0x80) continue;
            // Map to custom key codes
            if (tempScan == 0x48) return KEY_UP;
            if (tempScan == 0x50) return KEY_DOWN;
            if (tempScan == 0x4B) return KEY_LEFT;
            if (tempScan == 0x4D) return KEY_RIGHT;
            return ERR;
        }
        // Ignore release codes
        if (tempScan & 0x80) continue;
        // Handle specific keys like 'q' or 'Q'
        if (tempScan == 0x10) return isShiftDown ? 'Q' : 'q';
        return ERR;
    } while (1);
    return ERR;
}

// Function to wait for any key press, displaying a prompt
void waitForKey()
{
    // Display prompt message
    printString(COLOR_WHITE, 20, 5, (uint8_t *)"Press any key to continue...");
    // Wait for input
    getchCustom();
}

// Function to print text with word wrapping on the screen
void printWrapped(uint32_t color, uint32_t row, uint32_t col, uint8_t *text)
{
    int curCol = col;  // Current column position
    int curRow = row;  // Current row position
    uint8_t word[80];  // Buffer for current word
    int wordLen = 0;   // Length of current word
    // Process each character in the text
    while (*text)
    {
        // Handle space: print the current word and advance
        if (*text == ' ')
        {
            printString(color, curRow, curCol, word);
            curCol += wordLen + 1;
            wordLen = 0;
            text++;
            // Wrap to next line if exceeding width
            if (curCol >= 55)
            {
                curRow++;
                curCol = 5;
            }
            continue;
        }
        // Build the word
        word[wordLen++] = *text++;
        word[wordLen] = '\0';
        // Wrap if word would exceed line
        if (wordLen + curCol > 55)
        {
            curRow++;
            curCol = 5;
            printString(color, curRow, curCol, word);
            curCol += wordLen;
            wordLen = 0;
        }
    }
    // Print any remaining word
    if (wordLen > 0)
    {
        printString(color, curRow, curCol, word);
    }
    // Pause if text spans more than 5 rows
    if (curRow - row > 5)
    {
        waitForKey();
    }
}

// Function to check if a position is a wall, handling out-of-bounds as walls
uint8_t isWall(int level, int y, int x)
{
    // Treat out-of-bounds as walls
    if (y < 0 || y >= height || x < 0 || x >= width) return 1;
    // Check if the map cell is a wall('#')
    return mapData[level][y][x] == '#';
}

// Function to generate a single dungeon level
void generateLevel(int level)
{
    // Fill the entire map with walls('#')
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            mapData[level][y][x] = '#';
        }
        // Null-terminate each row as a string
        mapData[level][y][width] = '\0';
    }

    // Randomly determine number of rooms (20-29)
    numRooms[level] = myRand() % 10 + 20;

    // Generate rooms without overlap
    for (int i = 0; i < numRooms[level]; i++)
    {
        int tries = 0;
        // Try up to 300 times to place a room
        while (tries < 300)
        {
            // Random room dimensions (5-25)
            int roomWidth = myRand() % 21 + 5;
            int roomHeight = myRand() % 21 + 5;
            // Random position, ensuring space around
            int roomX = myRand() % (width - roomWidth - 2) + 1;
            int roomY = myRand() % (height - roomHeight - 2) + 1;

            uint8_t overlap = 0;
            // Check for overlap with existing rooms (with buffer)
            for (int j = 0; j < i; j++)
            {
                if (roomX < rooms[level][j].x + rooms[level][j].w + 4 &&
                    roomX + roomWidth > rooms[level][j].x - 4 &&
                    roomY < rooms[level][j].y + rooms[level][j].h + 4 &&
                    roomY + roomHeight > rooms[level][j].y - 4)
                {
                    overlap = 1;
                    break;
                }
            }
            if (!overlap)
            {
                // Place the room
                rooms[level][i].x = roomX;
                rooms[level][i].y = roomY;
                rooms[level][i].w = roomWidth;
                rooms[level][i].h = roomHeight;

                // Carve out the room interior as empty space
                for (int ry = roomY; ry < roomY + roomHeight; ry++)
                {
                    for (int rx = roomX; rx < roomX + roomWidth; rx++)
                    {
                        mapData[level][ry][rx] = ' ';
                    }
                }
                break;
            }
            tries++;
        }
    }

    // Connect rooms with corridors
    for (int i = 0; i < numRooms[level] - 1; i++)
    {
        int cx1 = rooms[level][i].x + rooms[level][i].w / 2;
        int cy1 = rooms[level][i].y + rooms[level][i].h / 2;
        int cx2 = rooms[level][i+1].x + rooms[level][i+1].w / 2;
        int cy2 = rooms[level][i+1].y + rooms[level][i+1].h / 2;

        // Horizontal corridor
        int minX = cx1 < cx2 ? cx1 : cx2;
        int maxX = cx1 > cx2 ? cx1 : cx2;
        for (int x = minX; x <= maxX; x++)
        {
            mapData[level][cy1][x] = ' ';
        }

        // Vertical corridor
        int minY = cy1 < cy2 ? cy1 : cy2;
        int maxY = cy1 > cy2 ? cy1 : cy2;
        for (int y = minY; y <= maxY; y++)
        {
            mapData[level][y][cx2] = ' ';
        }
    }

    // Place stairs, skipping up-stair on level 0 and down-stair on last level
    if (level > 0) // Only place up-stair if not on first level
    {
        int upRoom = myRand() % numRooms[level];
        upStair[level].x = rooms[level][upRoom].x + myRand() % (rooms[level][upRoom].w - 2) + 1;
        upStair[level].y = rooms[level][upRoom].y + myRand() % (rooms[level][upRoom].h - 2) + 1;
        mapData[level][upStair[level].y][upStair[level].x] = '<';
    }

    if (level < numLevels - 1) // Only place down-stair if not on last level
    {
        int downRoom = myRand() % numRooms[level];
        downStair[level].x = rooms[level][downRoom].x + myRand() % (rooms[level][downRoom].w - 2) + 1;
        downStair[level].y = rooms[level][downRoom].y + myRand() % (rooms[level][downRoom].h - 2) + 1;
        mapData[level][downStair[level].y][downStair[level].x] = '>';
    }

    // Place treasures
    int numTreasures = myRand() % 3 + 2;
    totalTreasures += numTreasures;
    for (int t = 0; t < numTreasures; t++)
    {
        int rr = myRand() % numRooms[level];
        int tx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int ty = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        while (mapData[level][ty][tx] != ' ')
        {
            tx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
            ty = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        }
        mapData[level][ty][tx] = 'T';
    }

    // Place key, cross, sword on specific levels
    if (level == 0)
    {
        int rr = myRand() % numRooms[level];
        int kx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int ky = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        mapData[level][ky][kx] = 'K';
    }
    if (level == 1)
    {
        int rr = myRand() % numRooms[level];
        int cx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int cy = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        mapData[level][cy][cx] = 'X';
    }
    if (level == 2)
    {
        int rr = myRand() % numRooms[level];
        int wx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int wy = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        mapData[level][wy][wx] = 'W';
    }

    // Place potions
    int numPotions = myRand() % 3;
    for (int p = 0; p < numPotions; p++)
    {
        int rr = myRand() % numRooms[level];
        int px = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int py = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        while (mapData[level][py][px] != ' ')
        {
            px = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
            py = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        }
        mapData[level][py][px] = 'P';
    }

    // Place notes
    int numNotes = myRand() % 3 + 1;
    for (int n = 0; n < numNotes; n++)
    {
        int rr = myRand() % numRooms[level];
        int nx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int ny = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        while (mapData[level][ny][nx] != ' ')
        {
            nx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
            ny = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        }
        mapData[level][ny][nx] = 'N';
    }

    // Place monsters
    int numMon = (myRand() % 5 + 3) * (level + 1);
    if (numMon > maxMonsters) numMon = maxMonsters;
    // Place monsters in random rooms
    for (int m = 0; m < numMon; m++)
    {
        monsters[m].level = level;
        monsters[m].alive = 1;
        monsters[m].isBoss = 0;
        // Random HP and attack scaled by level
        monsters[m].hp = (myRand() % 15 + 8) * (level + 1);
        monsters[m].attack = (myRand() % 4 + 2) * (level + 1);
        // Random monster type and symbol/color
        int type = myRand() % 8;
        switch (type)
        {
            case 0: monsters[m].sym = GOBLIN_CHAR; monsters[m].color = 0x02; break;
            case 1: monsters[m].sym = VAMPIRE_CHAR; monsters[m].color = COLOR_RED; break;
            case 2: monsters[m].sym = SKELETON_CHAR; monsters[m].color = COLOR_GREEN; break;
            case 3: monsters[m].sym = RAT_CHAR; monsters[m].color = 0x02; break;
            case 4: monsters[m].sym = DEMON_CHAR; monsters[m].color = COLOR_RED; break;
            case 5: monsters[m].sym = WOLF_CHAR; monsters[m].color = COLOR_LIGHT_BLUE; break;
            case 6: monsters[m].sym = GHOST_CHAR; monsters[m].color = 0x03; break;
            case 7: monsters[m].sym = DRAGON_CHAR; monsters[m].color = COLOR_RED; break;
        }
        // Random position in a room, ensuring empty space
        int rr = myRand() % numRooms[level];
        int mx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
        int my = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        while (mapData[level][my][mx] != ' ')
        {
            mx = rooms[level][rr].x + myRand() % (rooms[level][rr].w - 2) + 1;
            my = rooms[level][rr].y + myRand() % (rooms[level][rr].h - 2) + 1;
        }
        monsters[m].x = mx;
        monsters[m].y = my;
    }

    // Place boss on the last level
    if (level == numLevels - 1)
    {
        int bossIdx = numMon - 1;
        monsters[bossIdx].sym = BOSS_CHAR;
        monsters[bossIdx].color = COLOR_RED;
        monsters[bossIdx].isBoss = 1;
        monsters[bossIdx].hp = 200;
        monsters[bossIdx].attack = 15;
        // Place in center of last room
        int rr = numRooms[level] - 1;
        int mx = rooms[level][rr].x + rooms[level][rr].w / 2;
        int my = rooms[level][rr].y + rooms[level][rr].h / 2;
        mapData[level][my][mx] = ' ';
        monsters[bossIdx].x = mx;
        monsters[bossIdx].y = my;
    }
}

// Function to move the player in a direction, handling interactions
void movePlayer(int dy, int dx)
{
    int newY = playerY + dy;
    int newX = playerX + dx;
    // Cannot move into walls
    if (isWall(currentLevel, newY, newX)) return;

    // Reference to the target tile
    char* tile = &mapData[currentLevel][newY][newX];

    // Handle descending stairs
    if (*tile == '>')
    {
        if (currentLevel >= numLevels - 1) return; // Cannot descend from last level
        printString(COLOR_WHITE, 10, 10, (uint8_t *)"Descending deeper into the abyss...");
        wait(2);
        currentLevel++;
        playerX = upStair[currentLevel].x;
        playerY = upStair[currentLevel].y + 1;
        if (mapData[currentLevel][playerY][playerX] != ' ') playerY -= 2;
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)levelStories[currentLevel]);
        waitForKey();
        return;
    }
    // Handle ascending stairs
    else if (*tile == '<')
    {
        if (currentLevel <= 0) return; // Cannot ascend from first level
        printString(COLOR_WHITE, 10, 10, (uint8_t *)"Ascending towards the light...");
        wait(2);
        currentLevel--;
        playerX = downStair[currentLevel].x;
        playerY = downStair[currentLevel].y - 1;
        if (mapData[currentLevel][playerY][playerX] != ' ') playerY += 2;
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)levelStories[currentLevel]);
        waitForKey();
        return;
    }

    // Handle picking up treasure
    if (*tile == 'T')
    {
        treasures++;
        *tile = ' ';
        // Check for victory condition
        if (treasures >= totalTreasures)
        {
            printString(COLOR_WHITE, 10, 20, (uint8_t *)"You collected all treasures! Victory!");
            wait(5);
            systemExit(PROCESS_EXIT_CODE_SUCCESS);
        }
    }
    // Handle picking up key
    else if (*tile == 'K')
    {
        hasKey = 1;
        *tile = ' ';
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)itemStories[0]);
        waitForKey();
    }
    // Handle picking up cross
    else if (*tile == 'X')
    {
        hasCross = 1;
        *tile = ' ';
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)itemStories[1]);
        waitForKey();
    }
    // Handle picking up sword
    else if (*tile == 'W')
    {
        hasSword = 1;
        *tile = ' ';
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)itemStories[2]);
        waitForKey();
    }
    // Handle picking up potion
    else if (*tile == 'P')
    {
        playerHp += 50;
        if (playerHp > playerMaxHp) playerHp = playerMaxHp;
        *tile = ' ';
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)itemStories[4]);
        waitForKey();
    }
    // Handle reading note
    else if (*tile == 'N')
    {
        *tile = ' ';
        int noteIdx = myRand() % 20;
        printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)itemStories[noteIdx]);
        waitForKey();
    }

    // Check for monster at new position and handle combat
    for (int m = 0; m < maxMonsters; m++)
    {
        struct Monster* mon = &monsters[m];
        if (mon->alive && mon->level == currentLevel && newY == mon->y && newX == mon->x)
        {
            // Simple turn-based combat loop
            while (mon->hp > 0 && playerHp > 0)
            {
                // Player attacks, stronger with sword
                mon->hp -= (hasSword ? 20 : 10);
                if (mon->hp <= 0) break;
                // Monster attacks
                playerHp -= mon->attack;
                // Check for player death
                if (playerHp <= 0)
                {
                    printString(COLOR_WHITE, 10, 20, (uint8_t *)"You died in combat! Game over.");
                    wait(5);
                    systemExit(PROCESS_EXIT_CODE_SUCCESS);
                }
            }
            // If monster defeated
            if (mon->hp <= 0)
            {
                mon->alive = 0;
                // Check for boss defeat victory
                if (mon->isBoss)
                {
                    printString(COLOR_WHITE, 10, 20, (uint8_t *)"The Dark Lord falls! You have saved the realm!");
                    wait(5);
                    systemExit(PROCESS_EXIT_CODE_SUCCESS);
                }
                return;  // Don't move if combat won
            }
        }
    }

    // Update player position
    playerY = newY;
    playerX = newX;
}

// Function to move all monsters randomly
void moveMonsters()
{
    // Loop through all monsters
    for (int m = 0; m < maxMonsters; m++)
    {
        struct Monster* mon = &monsters[m];
        // Skip if not alive or not on current level
        if (!mon->alive || mon->level != currentLevel) continue;

        // Possible movement directions
        int dirs[4][2] = {{-1,0}, {1,0}, {0,-1}, {0,1}};
        // Random direction
        int dirIdx = myRand() % 4;
        int dy = dirs[dirIdx][0];
        int dx = dirs[dirIdx][1];
        int newY = mon->y + dy;
        int newX = mon->x + dx;

        // Cannot move into walls
        if (isWall(currentLevel, newY, newX)) continue;

        // If moving onto player, attack instead
        if (newY == playerY && newX == playerX)
        {
            playerHp -= mon->attack;
            // Check for player death
            if (playerHp <= 0)
            {
                printString(COLOR_WHITE, 10, 20, (uint8_t *)"Monster got you! Game over.");
                wait(5);
                systemExit(PROCESS_EXIT_CODE_SUCCESS);
            }
            continue;
        }

        // Update monster position
        mon->y = newY;
        mon->x = newX;
    }
}

// Function to determine the appropriate wall character based on neighbors
uint8_t getWallChar(int level, int y, int x)
{
    // Check neighboring cells for walls
    uint8_t up = (y > 0 && mapData[level][y-1][x] == '#') ? 1 : 0;
    uint8_t down = (y < height-1 && mapData[level][y+1][x] == '#') ? 1 : 0;
    uint8_t left = (x > 0 && mapData[level][y][x-1] == '#') ? 1 : 0;
    uint8_t right = (x < width-1 && mapData[level][y][x+1] == '#') ? 1 : 0;

    // Count neighbors
    int neighbors = up + down + left + right;

    // Isolated wall (shouldn't happen)
    if (neighbors == 0) return ' ';

    // Select appropriate junction or line character
    if (up && down && left && right) return WALL_CROSS;
    if (up && down && left) return WALL_T_RIGHT;
    if (up && down && right) return WALL_T_LEFT;
    if (left && right && up) return WALL_T_DOWN;
    if (left && right && down) return WALL_T_UP;
    if (up && left) return WALL_LOWER_RIGHT;
    if (up && right) return WALL_LOWER_LEFT;
    if (down && left) return WALL_UPPER_RIGHT;
    if (down && right) return WALL_UPPER_LEFT;
    if (up || down) return WALL_VERTICAL;
    if (left || right) return WALL_HORIZONTAL;

    // Fallback to basic wall
    return '#';
}

// Function to draw the visible portion of the map and status
void draw()
{
    // Clear the screen before drawing
    clearScreen();
    // Calculate view offsets centered on player
    int viewY = playerY - (visibleHeight / 2);
    if (viewY < 0) viewY = 0;
    if (viewY + visibleHeight > height) viewY = height - visibleHeight;

    int viewX = playerX - (visibleWidth / 2);
    if (viewX < 0) viewX = 0;
    if (viewX + visibleWidth > width) viewX = width - visibleWidth;

    // Render the visible map
    for (int dy = 0; dy < visibleHeight; ++dy)
    {
        int y = viewY + dy;
        for (int dx = 0; dx < visibleWidth; ++dx)
        {
            int x = viewX + dx;
            uint8_t c = mapData[currentLevel][y][x];
            uint8_t col = COLOR_WHITE;
            uint8_t drawC = c;
            // Handle wall rendering with special characters
            if (c == '#')
            {
                drawC = getWallChar(currentLevel, y, x);
                col = COLOR_LIGHT_BLUE;
            }
            // Map item symbols
            else if (c == 'T')
            {
                drawC = TREASURE_CHAR;
            }
            else if (c == 'K')
            {
                drawC = KEY_CHAR;
            }
            else if (c == 'X')
            {
                drawC = CROSS_CHAR;
            }
            else if (c == 'W')
            {
                drawC = SWORD_CHAR;
            }
            else if (c == 'P')
            {
                drawC = POTION_CHAR;
            }
            else if (c == '>')
            {
                drawC = STAIRS_DOWN;
            }
            else if (c == '<')
            {
                drawC = STAIRS_UP;
            }
            else if (c == 'N')
            {
                drawC = NOTE_CHAR;
            }

            // Render player symbol if at this position
            if (y == playerY && x == playerX)
            {
                printCharacter(0x0E, dy + startRow, dx, (uint8_t *)"\x1E");  // Yellow ▲
                continue;
            }

            // Check and render monsters if present
            uint8_t drawn = 0;
            for (int m = 0; m < maxMonsters; m++)
            {
                struct Monster* mon = &monsters[m];
                if (mon->alive && mon->level == currentLevel && y == mon->y && x == mon->x)
                {
                    printCharacter(mon->color, dy + startRow, dx, (uint8_t *)&mon->sym);
                    drawn = 1;
                    break;
                }
            }

            // Render tile if no monster
            if (!drawn)
            {
                printCharacter(col, dy + startRow, dx, (uint8_t *)&drawC);
            }
        }
    }

    // Build and print status line
    uint8_t status[80];
    uint8_t* p = status;
    strcpy(p, (uint8_t *)"Level: ");
    p += strlen(p);
    itoa(currentLevel + 1, p);  // 1-based level
    p += strlen(p);
    strcpy(p, (uint8_t *)" HP: ");
    p += strlen(p);
    itoa(playerHp, p);
    p += strlen(p);
    strcpy(p, (uint8_t *)"/100 Treasures: ");
    p += strlen(p);
    itoa(treasures, p);
    p += strlen(p);
    strcpy(p, (uint8_t *)"/");
    p += strlen(p);
    itoa(totalTreasures, p);
    p += strlen(p);
    strcpy(p, (uint8_t *)" | Key: ");
    p += strlen(p);
    strcpy(p, hasKey ? (uint8_t *)"YES" : (uint8_t *)"NO ");
    p += strlen(p);
    strcpy(p, (uint8_t *)" | Cross: ");
    p += strlen(p);
    strcpy(p, hasCross ? (uint8_t *)"YES" : (uint8_t *)"NO ");
    p += strlen(p);
    strcpy(p, (uint8_t *)" | Sword: ");
    p += strlen(p);
    strcpy(p, hasSword ? (uint8_t *)"YES" : (uint8_t *)"NO ");
    p += strlen(p);
    strcpy(p, (uint8_t *)" | Q=Quit");
    *p = '\0';
    printString(COLOR_WHITE, statusRow, 0, status);
}

// Main game entry point
void main()
{
    // Disable cursor for clean display
    disableCursor();
    // Clear screen
    clearScreen();

    // Display welcome message and lore
    printWrapped(COLOR_WHITE, 10, 5, (uint8_t *)"Welcome to the Cursed Castle! Collect treasures, find artifacts, and defeat the Dark Lord.");
    printWrapped(COLOR_WHITE, 12, 5, (uint8_t *)"Beware the monsters lurking in the shadows...");
    waitForKey();
    clearScreen();

    // Seed random generator with process ID for variability
    uint32_t pid = readValueFromMemLoc(RUNNING_PID_LOC);
    seed = pid;

    // Generate all levels
    for (int lvl = 0; lvl < numLevels; lvl++)
    {
        generateLevel(lvl);
    }

    // Set initial player position in first room
    playerX = rooms[0][0].x + rooms[0][0].w / 2;
    playerY = rooms[0][0].y + rooms[0][0].h / 2;

    // Main game loop
    while (1)
    {
        // Draw current view
        draw();

        // Get input
        int ch = getchCustom();
        if (ch == ERR)
        {
            // Move monsters on invalid input
            moveMonsters();
            // Small delay loop
            for (volatile uint32_t i = 0; i < 100000000; i++);
            continue;
        }

        // Handle movement keys
        switch (ch)
        {
            case KEY_UP:    movePlayer(-1, 0); break;
            case KEY_DOWN:  movePlayer(1, 0); break;
            case KEY_LEFT:  movePlayer(0, -1); break;
            case KEY_RIGHT: movePlayer(0, 1); break;
            case 'q':
            case 'Q': systemExit(PROCESS_EXIT_CODE_SUCCESS); break;
        }

        // Move monsters after player action
        moveMonsters();
    }
}