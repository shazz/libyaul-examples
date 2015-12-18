This is the port of the "Demo of executable compression on the Saturn (251kB)" from Mic (http://jiggawatt.org/badc0de/console.htm).

It consists of 

* cube.c which is normally compiled with liyaul (Makefile) 
* and another Makefile (Makefile.lzss) is provided to pack the generated bianry (cube.bin) and generate a new binary contained the lzss depacker and the packed binary.

When uploaded to the Saturn, the new binary lzsscube.bin should be written at 0x020210000 (non cached address, just after the GameShark program ending at 0x00021FFFA) in Lower RAM, the depacker will decompress the original binary at 0x006004000 as usual and then executed.

Then jsut run:
 sudo ssload -p 'datalink' -x lzsscube.nim -a 0x020210000