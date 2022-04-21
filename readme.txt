SSEQPlayer - an attempt at creating a player for a subset of the SSEQ format.

-- What it is:
This is a sub-project for the PTC Emulator, aiming to be able to parse and playback SSEQ files. This project does not aim to handle the entire format currently.
As it is currently, this is heavily WIP and not ready to be included in the main project, though many of the songs and sounds are recognizable already.

-- How to use it:
Don't.
If you really want to for some reason, you can get the SWAR, SBNK, and SSEQ files using sdatxtract, and modify main.cpp to test some of the current functions.
Note that building requires SFML, and this has only been tested on Linux currently.

-- Why I am uploading this now:
I do not want to lose the current progress on this if my computer breaks or something.

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
*Makefile could use some work

-- Cool resources:
*GBATek - https://www.problemkaputt.de/gbatek.htm
*NitroStudio2 docs - especially https://gota7.github.io/NitroStudio2/specs/sequence.html
*sseq2mid - https://github.com/loveemu/loveemu-lab/blob/master/nds/sseq2mid/src/sseq2mid.c
*sdatxtract - https://github.com/Oreo639/sdatxtract
And, some extra stuff in the comments.
