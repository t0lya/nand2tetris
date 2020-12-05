// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input.
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel;
// the screen should remain fully black as long as the key is pressed. 
// When no key is pressed, the program clears the screen, i.e. writes
// "white" in every pixel;
// the screen should remain fully clear as long as no key is pressed.

// PSEUDOCODE
//  while (true)
//      if (KBD === 0)
//          for (register in 256 * 32 = 8192 SCREEN registers)
//              register = 0
//      else 
//          for (register in 256 * 32 = 8192 SCREEN registers)
//              register = -1

(LOOP)
    @count
    M=0
    @KBD
    D=M
    @WHITE
    D;JEQ   // if (KBD === 0)
    @BLACK
    0;JMP   // else

(WHITE)
    @color
    M=0
    @DRAW
    0;JMP

(BLACK)
    @color
    M=-1
    @DRAW
    0;JMP

(DRAW)
    @count
    D=M
    @8192
    D=A-D
    @LOOP
    D;JLE
    @count
    D=M
    @SCREEN
    D=D+A
    @pos
    M=D
    @color
    D=M
    @pos
    A=M
    M=D
    @count
    M=M+1
    @DRAW
    0;JMP