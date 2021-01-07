/*
  _________.__    .___      __   .__        __           _____                            ___ _________   ________   _____   ___    
 /   _____/|__| __| _/____ |  | _|__| ____ |  | __      /     \   ____   ____  __ __     /  / \_   ___ \ /  _____/  /  |  |  \  \   
 \_____  \ |  |/ __ |/ __ \|  |/ /  |/ ___\|  |/ /     /  \ /  \_/ __ \ /    \|  |  \   /  /  /    \  \//   __  \  /   |  |_  \  \  
 /        \|  / /_/ \  ___/|    <|  \  \___|    <     /    Y    \  ___/|   |  \  |  /  (  (   \     \___\  |__\  \/    ^   /   )  ) 
/_______  /|__\____ |\___  >__|_ \__|\___  >__|_ \    \____|__  /\___  >___|  /____/    \  \   \______  /\_____  /\____   |   /  /  
        \/         \/    \/     \/       \/     \/            \/     \/     \/           \__\         \/       \/      |__|  /__/    

 rpimenu.cpp

 RasPiC64 - A framework for interfacing the C64 and a Raspberry Pi 3B/3B+
          - simple menu code running on the C64 (C128 and VIC detection, sending keypress, downloading screen)
 Copyright (c) 2019, 2020 Carsten Dachsbacher <frenetic@dachsbacher.de>

 Logo created with http://patorjk.com/software/taag/
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <conio.h>

extern void detectC128();
extern void detectVIC();

void bootstrapCustomNMIHandler()
{
  //FIXME: Do we have to backup accu value here first to restore it later?
  
  //cmd 1 - lda - first byte (opcode)
  __asm__ ("lda #$a9");
  __asm__ ("sta $c000");

  //cmd 1 - lda - second byte (payload)
  __asm__ ("lda #$01");
  __asm__ ("sta $c001");

  //cmd 2 - sta - first byte (opcode)
  __asm__ ("lda #$8d");
  __asm__ ("sta $c002");

  //cmd 2 - sta - second byte (lsb 0f)
  __asm__ ("lda #$0f");
  __asm__ ("sta $c003");

  //cmd 2 - sta - third byte (msb c0)
  __asm__ ("lda #$c0");
  __asm__ ("sta $c004");

//DEBUG: change border color
/*
  //cmd 3 - lda - first byte (opcode)
  __asm__ ("lda #$a9");
  __asm__ ("sta $c005");

  //cmd 3 - lda - second byte (payload)
  __asm__ ("lda #$02");
  __asm__ ("sta $c006");

  //cmd 3 - sta - first byte (opcode)
  __asm__ ("lda #$8d");
  __asm__ ("sta $c007");

  //cmd 3 - sta - second byte (lsb 20)
  __asm__ ("lda #$20");
  __asm__ ("sta $c008");

  //cmd 3 - sta - third byte (msb d0)
  __asm__ ("lda #$d0");
  __asm__ ("sta $c009");
*/

  //cmd 4 - RTI dec 64 / hex 40
  __asm__ ("lda #$40");
  __asm__ ("sta $c005");
  
  //end of handler
  
  //initialize flag with zero value
  __asm__ ("lda #0");
  __asm__ ("sta $c00f");
  
  //NMI handler will now be expected at $c000
  __asm__ ("lda #$00");
  __asm__ ("sta $0318");
  __asm__ ("lda #$c0");
  __asm__ ("sta $0319");

}

void updateScreen()
{
    __asm__ ("pha");
    __asm__ ("lda $fb");
    __asm__ ("pha");
    __asm__ ("lda $fc");
    __asm__ ("pha");
    __asm__ ("lda $fd");
    __asm__ ("pha");
    __asm__ ("lda $fe");
    __asm__ ("pha");

    __asm__ ("lda #250");
    __asm__ ("wait: cmp $d012");
    __asm__ ("bne wait");

    // copy screen (with color RAM)
    __asm__ ("lda #$01");
    __asm__ ("sta $df00");

    __asm__ ("lda #$00");
    __asm__ ("sta $fb");
    __asm__ ("lda #$04");
    __asm__ ("sta $fc");

    __asm__ ("lda #$00");
    __asm__ ("sta $fd");
    __asm__ ("lda #$d8");
    __asm__ ("sta $fe");


  __asm__ ("ldy #$00");
__asm__ ("loop:");
    __asm__ ("lda $df00");
    __asm__ ("sta ($fb),y");
    __asm__ ("lda $df01");
    __asm__ ("sta ($fd),y");
    __asm__ ("iny");
    __asm__ ("lsr");
    __asm__ ("lsr");
    __asm__ ("lsr");
    __asm__ ("lsr");
    __asm__ ("sta ($fd),y");
    __asm__ ("lda $df00");
    __asm__ ("sta ($fb),y");
    __asm__ ("iny");
    __asm__ ("bne loop");

    __asm__ ("inc $fe");
    __asm__ ("inc $fc");
    __asm__ ("lda $fc");
    __asm__ ("cmp #7");
    __asm__ ("bne loop");

__asm__ ("loop2:");
    __asm__ ("lda $df00");
    __asm__ ("sta ($fb),y");
    __asm__ ("lda $df01");
    __asm__ ("sta ($fd),y");
    __asm__ ("iny");
    __asm__ ("lsr");
    __asm__ ("lsr");
    __asm__ ("lsr");
    __asm__ ("lsr");
    __asm__ ("sta ($fd),y");
    __asm__ ("lda $df00");
    __asm__ ("sta ($fb),y");
    __asm__ ("iny");

    __asm__ ("tya");
    __asm__ ("cmp #232");
    __asm__ ("bne loop2");

    __asm__ ("pla");
    __asm__ ("sta $fe");
    __asm__ ("pla");
    __asm__ ("sta $fd");
    __asm__ ("pla");
    __asm__ ("sta $fc");
    __asm__ ("pla");
    __asm__ ("sta $fb");
    __asm__ ("pla");

    // execute code on the RPi (changed upper/lower case, border/bg color etc.)
    __asm__ ("jsr $df10");
}


void copyCharset()
{
    __asm__ ("pha");
    __asm__ ("lda $fb");
    __asm__ ("pha");
    __asm__ ("lda $fc");
    __asm__ ("pha");
    __asm__ ("lda $fd");
    __asm__ ("pha");
    __asm__ ("lda $fe");
    __asm__ ("pha");

    __asm__ ("sta $df04");

    __asm__ ("lda #$00");
    __asm__ ("sta $fb");
    __asm__ ("lda #$30");
    __asm__ ("sta $fc");

    __asm__ ("ldy #$00");
__asm__ ("loop:");
    __asm__ ("lda $df04");
    __asm__ ("sta ($fb),y");
    __asm__ ("iny");
    __asm__ ("bne loop");

    __asm__ ("inc $fc");
    __asm__ ("lda $fc");
    __asm__ ("cmp #$40");
    __asm__ ("bne loop");

    __asm__ ("pla");
    __asm__ ("sta $fe");
    __asm__ ("pla");
    __asm__ ("sta $fd");
    __asm__ ("pla");
    __asm__ ("sta $fc");
    __asm__ ("pla");
    __asm__ ("sta $fb");
    __asm__ ("pla");
}

// helps Sidekick64 detect if extra wires are connected
void wireDetection()
{
    __asm__ ("sta $df10");  // enable kernal replacement
    __asm__ ("lda $e000");  // access kernal (Sidekick64 will notice if wire connected)
    __asm__ ("sta $df11");  // disable kernal replacement

    __asm__ ("lda #$00");
    __asm__ ("sta $d4ff");  // some access to the SID address range
}

int main (void)
{
    char key;

    *(unsigned char*)(0x01) = 15;

    detectC128();
    __asm__ ("sta $df02");

    detectVIC();
    __asm__ ("sta $df03");

    copyCharset();
    updateScreen();

    wireDetection();
    *((char *)(0xdf01)) = 0; // dummy keypress
    //updateScreen();

    wireDetection();
    *((char *)(0xdf01)) = 0; // dummy keypress
    updateScreen();

    bootstrapCustomNMIHandler();

    while ( 1 )
    {
        while ( kbhit() == 0 && *((char *)(0xc00f)) == 0 )
        {
          //neither a keypress nor an NMI occured
        }
        
        if (*((char *)(0xc00f)) == 0)
        {
          // sendKeypress
          key = cgetc();
          if ( key == 29 && *((char *)(0x0427)) != 0 )
          {
              __asm__ ("lda #$0a");
              __asm__ ("ldx #$20");
              __asm__ ("loop:");
              __asm__ ("sta $d850,x");
              __asm__ ("dex");
              __asm__ ("bne loop");
          }
          wireDetection();
          *((char *)(0xdf01)) = key;
          updateScreen();
        }
        else
        {
          *((char *)(0xc00f)) = 0; //reset update flag set by nmi handler
          __asm__ ("lda #$0a"); //FIXME: do we need this here to have the right value in accu?
          wireDetection();
          updateScreen();
        }
    }

    return 0;
}
