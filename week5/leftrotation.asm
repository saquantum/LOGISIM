// send RAM[0] to RAM[2]
@R0
D=M
@R2
M=D
// check if there are more loops to double RAM[2]
(start)
@R1
D=M
D=D-1
M=D
@end
D;JLT
// check the MSB of RAM[2] and choose a branch
(shift)
@32767
D=A+1 // now D = 1000 0000 0000 0000
@R2
D=D&M
@MSB1
D;JLT

(MSB0)
@R2
D=M
D=M+D
M=D
@start
0;JMP
(MSB1)
@R2
D=M
D=M+D
M=D+1
@start
0;JMP

(end)
@end
0;JMP
