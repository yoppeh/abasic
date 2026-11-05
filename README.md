# abasic

This is a BASIC interpreter based on Atari's 8 bit BASIC cartridge. I used
this [BNF diagram](https://archive.org/details/AtariBASICBNF), which came
straight from Atari, the [Atari BASIC Reference Manual](https://archive.org/details/atari-basic-reference-manual),
and the [Atari BASIC Self-Teaching Guide](http://www.atariarchives.org/basic/).
This is not meant to be an exact clone and it does some things differently.
First, none of the hardware-specific instructions are included. A CLS
instruction is included to clear the screen.

Programs can be loaded with:
```
load "/path/to/file.bas"
```
And saved with:
```
save "/path/to/file.bas"
```
Files are plain-text listings rather than the Atari's tokenized format.
LOAD clears the current program first, as on the Atari. A program can also
be loaded when starting the interpreter with `abasic /path/to/file.bas`;
type RUN to start it.

Strings work as on the Atari. A string variable must be DIMensioned with its
maximum length before it is used, e.g. `DIM A$(20)`. `A$(3)` is the
substring from character 3 to the end and `A$(3,5)` is characters 3 to 5;
both can also be assigned to. There is no `+` for strings; append with
`A$(LEN(A$)+1)=B$`. As on the Atari, INPUT and READ take only
unsubscripted variables: read into a plain variable, then assign it to the
array element or substring.

Arrays are also as on the Atari: `DIM A(10)` gives subscripts 0 to 10, and
arrays must be DIMensioned before use.

DEG and RAD switch SIN, COS and ATN between degrees and radians.

Errors are reported with the Atari error number and the line number, e.g.
`ERROR 12 at line 20: line 999 not found`. A syntax error in a line being
entered shows the line with a `^` under the point where the error was
detected.

The abbreviated instructions are recognized (PR. for PRINT, G. for GOTO and
so on, worked out from the Atari's keyword table, so R. is REM and REA. is
READ), as are ? for PRINT and . for REM.

TRAP works as on the Atari: after an error is trapped, PEEK(195) gives the
error number and PEEK(186)+256*PEEK(187) the line number; these are the only
addresses PEEK supports. As a shortcut, ERL gives the line number and ERR$
the error message. TRAP 40000 turns trapping off.

Four BASIC programs are included. Three are from the Atari BASIC tutorial and
one is from some guy's Atari disk image. I loaded it into an Atari emulator
then saved it to my hard drive. I had to modify it slightly to account for
the differences in string handling, lack of support for "?" as "PRINT" and
lack of GRAPHICS modes, but it wasn't too hard. (It now also DIMensions its
strings, as Atari strings require, and RESTOREs its DATA before each turn,
without which it ran out of data after a few turns.)

Other differences from Atari BASIC:
- No hardware, sound, graphics or I/O channel statements (COLOR, SOUND,
  OPEN, PEEK/POKE beyond the TRAP locations above, and so on). RUN and
  LIST don't take a file name.
- Keywords must be separated from variable names: `PRINT X`, not `PRINTX`.
- Lower case is accepted for keywords and variable names.
- Numbers are printed with up to 15 significant digits.
- Numeric variables and array elements start at 0 (the Atari leaves arrays
  uninitialized), and the gap left by assigning past the end of a string is
  filled with spaces.

To build, just:
```
make
```
It should build and run on any machine with gcc and gnu make. I've built and
run it on linux and Mac OS X.

Any bug reports or BASIC program contributions will be appreciated.

Warren
