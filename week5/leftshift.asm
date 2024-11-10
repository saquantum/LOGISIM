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
@R2
D=M
D=M+D
M=D
@start
0;JMP

(end)
@end
0;JMP
