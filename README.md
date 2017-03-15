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

String handling is different than that of the Atari. Dimensioning string
variables gives you an array of strings. I will add standard BASIC string
functions at some point to make up for the loss of the Atari substring
notation.

DEG and RAD are missing. I didn't think they were necessary, particularly 
without any GRAPHICS support.

Error handling is different than Atari's also. This interpreter just displays
an error message--usually as cryptic as Atari's, but without highlighting the
point where the error was detected.

This interpreter doesn't recognize the abbreviated instructions, nor does it
recognize ? as "PRINT" or . as REM. I don't intend to add these: The Atari
converted the abbreviations to the full form internally and they aren't
necessary in a relatively large memory environment.

Four BASIC programs are included. Three are from the Atari BASIC tutorial and
one is from some guy's Atari disk image. I loaded it into an Atari emulator
then saved it to my hard drive. I had to modify it slightly to account for
the differences in string handling, lack of support for "?" as "PRINT"  and 
lack of GRAPHICS modes, but it wasn't too hard.

Currently, TRAP doesn't give you any information about the error that got
TRAPPED. I will clean that up at some point as well.

To build, just:
```
make
```
It should build and run on any machine with gcc and gnu make. I've built and
run it on linux and Max OS X.

Any bug reports or BASIC program contributions will be appreciated.

Warren
