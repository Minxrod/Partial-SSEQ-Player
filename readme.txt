SSEQPlayer - an attempt at creating a player for a subset of the SSEQ format.

-- What it is:
This is a sub-project for the PTC Emulator, aiming to be able to parse and playback SSEQ files. This project does not aim to handle the entire format currently.
As it is currently, this is a WIP, but a majority of BGM and SFX sound correct or at least pretty close.

-- How to use it:
You can get the SWAR, SBNK, and SSEQ files using sdatxtract, and modify main.cpp to test some of the current functions.
Note that building requires SFML, and this has only been tested on Linux currently.

To compile, you should just be able to do this.
$ make

-- Current functionality:
*Converting SWAR to waveforms
*Reading SBNK
*Partial SSEQ reading/playing
*Samples except 28 are pretty good

-- Known issues:
*Code is messy
*ADSR envelope is not completely correct
*Various SSEQ events are currently ignored or unimplemented
 *Modulation, "Volume 2", priority specifically are known to be needed
*Makefile could use some more work

-- Cool resources:
*GBATek - https://www.problemkaputt.de/gbatek.htm
*NitroStudio2 docs - especially https://gota7.github.io/NitroStudio2/specs/sequence.html
*sseq2mid - https://github.com/loveemu/loveemu-lab/blob/master/nds/sseq2mid/src/sseq2mid.c
*sdatxtract - https://github.com/Oreo639/sdatxtract
And, some extra stuff in the comments.
