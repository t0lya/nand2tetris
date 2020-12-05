// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)

// PSEUDOCODE  (try doubling result when possible for faster mult)
//  if (R0 ==== 0)
//      R2 = 0
//      return
//
//  if (R1 === 0)
//      R2 = 0
//      return
//
//  count = 1
//  ret = R0
//  while (count < R1)
//      if (count + count <= R1)
//          count = count + count
//          ret = ret + ret
//      else
//          count = count + 1
//          ret = ret + R0
//  R2 = ret

@R0
D=M
@RETURN
D;JEQ   // if (R0 === 0)

@R1
D=M
@RETURN
D;JEQ   // if (R1 === 0)

@R0
D=M
@ret
M=D // ret = R0
@count
M=1 // count = 1

(LOOP)
    @count
    D=M
    @R1
    D=D-M
    @RETURN
    D;JGE  // while (count < R1)

    @count
    D=M
    @temp
    M=D
    M=D+M
    D=M
    @R1
    D=D-M
    @DOUBLE
    D;JLE // if (count + count <= R1)
    @INC
    0;JMP

(DOUBLE)
    @R1
    D=D+M
    @count
    M=D // count = count + count
    @ret
    D=M
    M=D+M // ret = ret + ret
    @LOOP
    0;JMP

(INC)
    @count
    M=M+1 // count = count + 1
    @R0
    D=M
    @ret
    M=D+M // ret = ret + R0
    @LOOP
    0;JMP

(RETURN)
    @ret
    D=M
    @R2
    M=D

(END)
    @END
    0;JMP