# Overview
The program implements a basic line-oriented text editor called "Edlin" (inspired by the classic MS-DOS edlin utility). The editor is designed for simple text manipulation, focusing on editing individual lines rather than full-screen cursor-based editing like modern editors (e.g., Vim or Nano). It supports a maximum of 250 lines, each up to 65 characters, and displays up to 35 lines at a time on a text-mode screen.
The editor operates in a command-line mode at a prompt (e.g., "001:*"), where users enter commands or line numbers. It handles basic input with shift key support for capitalization and symbols, tab expansion (to 8 spaces), and paging via Page Up/Down. It's not a WYSIWYG editor; edits are done by specifying lines explicitly.
## Key Capabilities
1. Text Buffer and Display Management

Buffer Structure: Text is stored in a 2D array (textBuffer[MAX_LINES][MAX_LINE_LENGTH + 1]), allowing up to 250 lines of 65 characters each (null-terminated).
Display: Shows lines starting from a configurable start line (displayStartLineNumber), prefixed with zero-padded line numbers (e.g., "001: Hello World"). Limits display to 35 lines to fit the screen.
Scrolling: Supports Page Up (scroll up by 35 lines) and Page Down (scroll down by 35 lines) while in command input mode. Adjusts displayStartLineNumber to stay within bounds.
Prompt and Status:
Command prompt at row 35 (e.g., "001:*" in green/red), where "001" is the current line (1-based).
Status messages (e.g., "File loaded") at row 36.
Open files are displayed starting at column 40 via systemShowOpenFiles.


2. Input Handling

Keyboard Input: Uses a custom readKeyboard function to handle scancodes directly. Supports:
Alphanumeric keys with shift for uppercase/symbols (e.g., shift+1 = "!").
Backspace for deletion.
Enter to submit.
Space, tabs (expanded to next 8-column tab stop during printing).
Ignores key releases and non-printable codes.

Line Reading: The readInputLine function reads into a buffer, updating the cursor in real-time. It can enable paging for long listings.
No Mouse or Advanced Input: Pure keyboard-driven; no arrow keys for navigation within lines (edits replace entire lines).

3. Editing Commands
Commands are entered at the prompt and are case-insensitive (e.g., 'e' or 'E'). Most are single-letter, with optional parameters.

































CommandDescription<number> (e.g., "5")Jumps to the specified line (1-based), sets it as current, and redisplays from there.e / EEdits the current line: Displays the old content, reads new input, and replaces it. Updates total lines if extending beyond current end.l / LLists (redisplays) lines starting from the current display start.i / IInsert mode: Adds new lines at the current position. Reads lines until a solo "." is entered. Shifts subsequent lines down; caps at 250 lines.d / DDeletes the current line, shifts subsequent lines up, and adjusts total line count.h / HShows a help screen listing all commands, pauses for Enter, then redisplays the editor.

Navigation: Line-based only; no intra-line cursor movement or multi-line selections.
Error Handling: Basic validation (e.g., invalid line numbers show "Invalid line number"; unknown commands show "Unknown command").

4. File Operations

Loading:
If a filename is provided as a command-line argument (after the program name in COMMAND_BUFFER), loads it automatically.
Parses file content by splitting on newlines (0x0A), trimming nulls.
Command for loading is commented out (o/O <filename>), so only initial load or manual workarounds are supported in this version.

Saving:
w <filename>: Saves to the specified file. Uses a temporary file ("edtmp") to build content (appending newlines), then creates the target file via system calls.
s / S: Saves to the initially loaded file (if any); errors if none specified.

File System Integration: Relies on OS calls like systemOpenFile, systemCreateFile, systemDeleteFile, systemCloseFile. Handles read/write modes, descriptors, and memory pointers. Calculates required pages for allocation.
Limitations: No append/overwrite choice; overwrites existing files. No directory support (filenames only). Assumes files fit in memory.

5. Process and System Integration

Process Management: Uses systemExit to quit successfully, systemSwitchToParent for p/P (switch to parent process, likely for multitasking).
Memory Management: Uses malloc and free (via process ID), fills buffers with zeros for initialization.
Utilities: Includes helper functions like padIntegerToString (for zero-padded line numbers), itoa, atoi, strlen, bytecpy (custom memcpy).

6. Help and Usability Features

Help Screen: Comprehensive list of commands with descriptions.
Paging in Listings: When content exceeds 35 lines, Page Up/Down allows scrolling without losing input buffer state (redisplays prompt and partial input).
Real-Time Feedback: Cursor moves with input; backspace erases visually.