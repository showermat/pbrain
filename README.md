# pbrain

This is a simple interpreter for pbrain, an extension of [an esoteric programming langauge of a similar name](http://esolangs.org/wiki/Brainfuck).  It provides console and curses interfaces for running pbrain programs, as well as non-interactive modes.

## Implementation Choices

This implementation of pbrain provides a tape that is infinite in both directions.  The tape is non-sparse and allocated as cells are accessed, so incrementing the first cell and then the 10,000th will allocate 10 KB of memory.  Each cell contains an `unsigned char` initialized to 0, and decrementing 0 or incrementing 255 causes the value to wrap around.

If the end of the file is encountered when reading from an input file, the EOF value (generally -1) is cast to an `unsigned char` and placed in the current cell.  This means that reading past the end of a file should result in the current cell being set to 255.  Providing an EOF on standard input will result in the ASCII EOF character (0x04) being sent to the program.

Pressing enter returns an ASCII newline (0x0A) to the program.

## Usage

Execute the script in a file:

    pbrain test.bf

Read the input from a file as well:

    pbrain test.bf input.txt

Start in interactive (console) mode:

    pbrain

### Flags

  - `-g`: Display the ncurses UI
  - `-e`: When running a script from a file, switch to interactive mode after finishing rather than exiting immediately
  - `-p`: Disable interpretation of the pbrain commands `(`, `)`, and `:`, treating the deck as standard Brainfuck
  - `-d`: Disable the debugging extensions listed below

### Debugging Extensions

This interpreter provides a few extra commands for debugging convenience.  Naturally, they should only be used for debugging and not in production programs.

  - `=`: Print the value of the current cell as an unsigned integer
  - `?`: Display the number of the current cell
  - `%`: Print a newline
  - `!`: Stop execution

### Interpreter Commands

Interpreter commands begin with a slash and can only be typed by themselves on a console line, not embedded in the input deck.

  - `/q`: Quit the interpreter
  - `/r`: Reset the machine state
  - `/c`: Resume execution from the current cell (only meaningful )

## Bugs

  - Special characters such as newlines are not echoed by the input/output boxes.  They are stored and processed but do not appear on the screen.
  - The whole thing needs a lot more testing.

