//right rotation x is equivalent to left rotating 16-x
@15
D=A
@R1
M=M&D
D=D+1
M=D-M

@R0
D=M
@R2
M=D

(start)
@R1
D=M
D=D-1
M=D
@end
D;JLT

(shift)
@32767
D=A+1
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
