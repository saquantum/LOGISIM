#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "token.h"

// Lexing functions
void lex_file(FILE *input, FILE *output);
void lex_line(const char *line, FILE *output);
int lex_token(Token *dest, const char *line);

// Parsing functions
void parse_file(char *filename, FILE *input, FILE *output);
int get_next_instruction(Token *dest[], FILE *input);
void parse_instruction(Token *instruction[], char *filename, FILE *output);
void parse_push(char *filename, Token *segment, Token *address, char *dest);
void parse_pop(char *filename, Token *segment, Token *address, char *dest);
void parse_add(char *dest);
void parse_sub(char *dest);
void parse_neg(char *dest);
void parse_and(char *dest);
void parse_or(char *dest);
void parse_not(char *dest);
void parse_eq(char *filename, char *dest);
void parse_lt(char *filename, char *dest);
void parse_gt(char *filename, char *dest);
void parse_label(char *filename, Token *label, char *dest);
void parse_goto(char *filename, Token *label, char *dest);
void parse_ifgoto(char *filename, Token *label, char *dest);
void get_next_label_name(char *filename, char *dest);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Please supply two arguments: an input .vm file, and an output .asm file.");
        exit(EXIT_FAILURE);
    }
    char *input_name = argv[1];
    char *output_name = argv[2];

    FILE *input = fopen(input_name, "r");
    if (input == NULL) {
        exit(EXIT_FAILURE);
    }
    FILE *lex_output = fopen("temp.lex", "w");
    if (lex_output == NULL) {
        exit(EXIT_FAILURE);
    }
    lex_file(input, lex_output);
    fclose(input);
    fclose(lex_output);

    FILE *lex_input = fopen("temp.lex", "r");
    if (lex_input == NULL) {
        exit(EXIT_FAILURE);
    }
    FILE *output = fopen(output_name, "w");
    if (output == NULL) {
        exit(EXIT_FAILURE);
    }
    parse_file(input_name, lex_input, output);
    fclose(lex_input);
    fclose(output);

    return EXIT_SUCCESS;
}

// Outputs a tokenised version of input to output.
void lex_file(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, input) != NULL) {
        lex_line(line, output);
    }
}

// Reads the next line from input, tokenises it, and writes the resulting tokens to output.
void lex_line(const char *line, FILE *output) {
    int pos = 0;
    bool tokens_on_line = false;

    // In each iteration of this loop, everything up to line[pos] has been lexed.
    while ((line[pos] != '\n') && (line[pos] != '\r')) {
        Token *next = malloc_token();

        // Ignore all whitespace.
        if (line[pos] == ' ') {
            pos++;
            continue;
        }

        // Ignore all comments.
        if (line[pos] == '/') {
            break;
        }

        // Otherwise, we have at least one non-newline token, so lex it.
        tokens_on_line = true;
        pos += lex_token(next, line + pos);
        write_token(next, output);

        free_token(next);
    }

    // Ignore empty lines, but otherwise lex the newline at the end.
    if (tokens_on_line) {
        Token *next = malloc_token();
        lex_token(next, "\n");
        write_token(next, output);
        free_token(next);
    }
}

// Reads the next token from a non-empty, non-label, non-comment line into dest, then returns the number of characters
// in that token.
int lex_token(Token *dest, const char *line) {
    int length;

    if (line[0] == '\n') {
        dest->type = NEWLINE;
        length = 1;
    } else if (line[0] >= '0' && line[0] <= '9') {
        // Identifiers can't start with integers, so this must be an integer literal
        dest->type = INTEGER_LITERAL;
        char literal[MAX_LINE_LENGTH];
        for(length = 0; line[length] >= '0' && line[length] <= '9'; length++) {
            literal[length] = line[length];
        }
        literal[length] = '\0';
        dest->value.int_val = strtol(literal, NULL, 10);
    } else { // We either have an identifier or a keyword
        // Either way, it keeps going until reaching either a space, a newline. (Per the definition of an identifier
        // token, if there's a // before a space, it counts as part of the identifier rather than a comment.)
        length = 0;
        while(line[length] != ' ' && line[length] != '\n' && line[length] != '\r'){
            length++;
        }

        dest->type = KEYWORD; // Default
        if (strncmp(line, "push", length) == 0) {
            dest->value.key_val = PUSH;
        } else if (strncmp(line, "pop", length) == 0) {
            dest->value.key_val = POP;
        } else if (strncmp(line, "add", length) == 0) {
            dest->value.key_val = ADD;
        } else if (strncmp(line, "sub", length) == 0) {
            dest->value.key_val = SUB;
        } else if (strncmp(line, "neg", length) == 0) {
            dest->value.key_val = NEG;
        } else if (strncmp(line, "and", length) == 0) {
            dest->value.key_val = AND;
        } else if (strncmp(line, "or", length) == 0) {
            dest->value.key_val = OR;
        } else if (strncmp(line, "not", length) == 0) {
            dest->value.key_val = NOT;
        } else if (strncmp(line, "eq", length) == 0) {
            dest->value.key_val = EQ;
        } else if (strncmp(line, "gt", length) == 0) {
            dest->value.key_val = GT;
        } else if (strncmp(line, "lt", length) == 0) {
            dest->value.key_val = LT;
        } else if (strncmp(line, "local", length) == 0) {
            dest->value.key_val = LOCAL;
        } else if (strncmp(line, "constant", length) == 0) {
            dest->value.key_val = CONSTANT;
        } else if (strncmp(line, "this", length) == 0) {
            dest->value.key_val = THIS;
        } else if (strncmp(line, "that", length) == 0) {
            dest->value.key_val = THAT;
        } else if (strncmp(line, "pointer", length) == 0) {
            dest->value.key_val = POINTER;
        } else if (strncmp(line, "argument", length) == 0) {
            dest->value.key_val = ARGUMENT;
        } else if (strncmp(line, "static", length) == 0) {
            dest->value.key_val = STATIC;
        } else if (strncmp(line, "temp", length) == 0) {
            dest->value.key_val = TEMP;
        } else if (strncmp(line, "label", length) == 0) {
            dest->value.key_val = LABEL;
        } else if (strncmp(line, "goto", length) == 0) {
            dest->value.key_val = GOTO;
        } else if (strncmp(line, "if-goto", length) == 0) {
            dest->value.key_val = IFGOTO;
        } else if (strncmp(line, "function", length) == 0) {
            dest->value.key_val = FUNCTION;
        } else if (strncmp(line, "call", length) == 0) {
            dest->value.key_val = CALL;
        } else if (strncmp(line, "return", length) == 0) {
            dest->value.key_val = RETURN;
        } else { // We've now ruled out all the keywords, so it must be an identifier.
            dest->type = IDENTIFIER;
            dest->value.str_val = (char *)malloc(length+1);
            strncpy(dest->value.str_val, line, length);
            dest->value.str_val[length] = '\0'; // Strncpy doesn't null-terminate.
        }
    }
    return length;
}

// Input should be a tokenised file. Writes Hack assembly code to output.
void parse_file(char *filename, FILE *input, FILE *output) {
    Token *instruction[MAX_LINE_LENGTH];
    // Send code to output to initialise SP.
    fputs("@256\n"
          "D=A\n"
          "@SP\n"
          "M=D\n", output);
    while (1) {
        // Get the next instruction.
        int length = get_next_instruction(instruction, input);
        // If we just have an EOF, free the token and finish.
        if (length == 0) {
            free_token(instruction[0]);
            break;
        }
        // Otherwise, actually parse the instruction, free the token and repeat.
        parse_instruction(instruction, filename, output);
        for(int i=0; i<length; i++) {
            free_token(instruction[i]);
        }
    }
    // Send code to output to end with infinite loop.
    fputs("(HaltInfiniteLoop)\n"
          "@HaltInfiniteLoop\n"
          "0;JMP", output);
}

// Copies a list of tokens corresponding to the next Hack VM instruction into dest, omitting the newline. Returns number
// of tokens.
int get_next_instruction(Token *dest[], FILE *input) {
    dest[0] = malloc_token();
    bool instruction_exists = read_token(dest[0], input);
    if (!instruction_exists) {
        return 0;
    }
    int length = 1;
    while (1) {
        dest[length] = malloc_token();
        read_token(dest[length], input);
        if (dest[length]->type == NEWLINE) {
            free_token(dest[length]);
            break;
        }
        length++;
    }
    return length;
}

// Parse the given VM instruction and write the corresponding assembly code to the output file.
void parse_instruction(Token *instruction[], char *filename, FILE *output) {
    char code_to_output[10000] = "";

    // We can tell the entire syntax of the instruction from the first token, which should be a keyword.
    if (instruction[0]->type != KEYWORD) {
        printf("Malformed instruction!");
        exit(EXIT_FAILURE);
    } switch (instruction[0]->value.key_val) {
        case PUSH:   parse_push(filename, instruction[1], instruction[2], code_to_output); break;
        case POP:    parse_pop(filename, instruction[1], instruction[2], code_to_output); break;
        case ADD:    parse_add(code_to_output); break;
        case SUB:    parse_sub(code_to_output); break;
        case NEG:    parse_neg(code_to_output); break;
        case AND:    parse_and(code_to_output); break;
        case OR:     parse_or(code_to_output); break;
        case NOT:    parse_not(code_to_output); break;
        case EQ:     parse_eq(filename, code_to_output); break;
        case LT:     parse_lt(filename, code_to_output); break;
        case GT:     parse_gt(filename, code_to_output); break;
        case LABEL:  parse_label(filename, instruction[1], code_to_output); break;
        case GOTO:   parse_goto(filename, instruction[1], code_to_output); break;
        case IFGOTO: parse_ifgoto(filename, instruction[1], code_to_output); break;
        case FUNCTION:
        case CALL:
        case RETURN: printf("Function calling not implemented."); exit(EXIT_FAILURE);
        default: printf("Malformed instruction!"); exit(EXIT_FAILURE);
    }

    fputs(code_to_output, output);
    fflush(output);
}

// Append assembly code for an "add" instruction into dest.
void parse_add(char *dest) {
    strcat(dest, "// add\n"); // TODO: Add Hack assembly here!
    
    // SP--;RAM[SP-1]=RAM[SP-1]+RAM[SP];
    strcat(dest,"@SP\nM=M-1\nA=M\nD=M\nA=A-1\nD=D+M\nM=D\n");
    // ======================
}

// Append assembly code for the instruction "push [segment] [address]" into dest.
void parse_push(char *filename, Token *segment, Token *address, char *dest) {
    if (segment->type != KEYWORD) {
        printf("Malformed instruction!");
        exit(EXIT_FAILURE);
    }

    char code[500];

    switch (segment->value.key_val) {
        case CONSTANT:
            sprintf(code, "// push constant\n"); // TODO: Add Hack assembly here!
            
            // integer literal
            sprintf(code+strlen(code),"@%d\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
        case LOCAL:
            sprintf(code, "// push local\n"); // TODO: Add Hack assembly here!
            
            // base address stored in LCL
            sprintf(code+strlen(code),"@%d\nD=A\n@LCL\nA=M\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
        case ARGUMENT:
            sprintf(code, "// push argument\n"); // TODO: Add Hack assembly here!
            
            // base address stored in ARG
            sprintf(code+strlen(code),"@%d\nD=A\n@ARG\nA=M\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
        case THIS:
            sprintf(code, "// push this\n"); // TODO: Add Hack assembly here!
            
            // base address stored in THIS
            sprintf(code+strlen(code),"@%d\nD=A\n@THIS\nA=M\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
        case THAT:
            sprintf(code, "// push that\n"); // TODO: Add Hack assembly here!
            
            // base address stored in THAT
            sprintf(code+strlen(code),"@%d\nD=A\n@THAT\nA=M\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
        case TEMP:
            sprintf(code, "// push temp\n"); // TODO: Add Hack assembly here!
            
            // base address 5
            sprintf(code+strlen(code),"@%d\nD=A\n@5\nA=A+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
        case POINTER:
            sprintf(code, "// push pointer\n"); // TODO: Add Hack assembly here!
            
            switch(address->value.int_val){
                case 0: sprintf(code+strlen(code),"@%d\nD=A\n@THIS\nA=M\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val); break;
                case 1: sprintf(code+strlen(code),"@%d\nD=A\n@THAT\nA=M\nA=M+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val); break;
                default: exit(1);
            }
            
            break;
        case STATIC:
            sprintf(code, "// push static\n"); // TODO: Add Hack assembly here!
            
            // base address 16
            sprintf(code+strlen(code),"@%d\nD=A\n@16\nA=A+D\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n",address->value.int_val);
            // ======================
            break;
    }
    strcat(dest, code);
}

// Append assembly code for the instruction "pop [segment] [address]" into dest.
void parse_pop(char *filename, Token *segment, Token *address, char *dest) {
    if (segment->type != KEYWORD) {
        printf("Malformed instruction!");
        exit(EXIT_FAILURE);
    }

    char code[500];
    
    switch (segment->value.key_val) {
        case CONSTANT:
            sprintf(code, "// pop constant\n"); // TODO: Add Hack assembly here!
            
            exit(1);
            // ======================
            break;
        case LOCAL:
            sprintf(code, "// pop local\n"); // TODO: Add Hack assembly here!
            
            // base address LCL
            sprintf(code+strlen(code),"@%d\nD=A\n@LCL\nD=M+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val);
            // ======================
            
            break;
        case ARGUMENT:
            sprintf(code, "// pop argument\n"); // TODO: Add Hack assembly here!
            
            // base address ARG
            sprintf(code+strlen(code),"@%d\nD=A\n@ARG\nD=M+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val);
            // ======================
            break;
        case THIS:
            sprintf(code, "// pop this\n"); // TODO: Add Hack assembly here!
            
            // base address THIS
            sprintf(code+strlen(code),"@%d\nD=A\n@THIS\nD=M+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val);
            // ======================
            break;
        case THAT:
            sprintf(code, "// pop that\n"); // TODO: Add Hack assembly here!
            
            // base address THAT
            sprintf(code+strlen(code),"@%d\nD=A\n@THAT\nD=M+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val);
            // ======================
            break;
        case TEMP:
            sprintf(code, "// pop temp\n"); // TODO: Add Hack assembly here!
            
            // base address 5
            sprintf(code+strlen(code),"@%d\nD=A\n@5\nD=A+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val);
            // ======================
            break;
        case POINTER:
            sprintf(code, "// pop pointer\n"); // TODO: Add Hack assembly here!
            
            switch(address->value.int_val){
                case 0: sprintf(code+strlen(code),"@%d\nD=A\n@THIS\nD=M+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val); break;
                case 1: sprintf(code+strlen(code),"@%d\nD=A\n@THAT\nD=M+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val); break;
                default: exit(1);
            }
            break;
        case STATIC:
            sprintf(code, "// pop static\n"); // TODO: Add Hack assembly here!
            
            // base address 16
            sprintf(code+strlen(code),"@%d\nD=A\n@16\nD=A+D\n@5\nM=D\n@SP\nM=M-1\nA=M\nD=M\n@5\nA=M\nM=D\n",address->value.int_val);
            // ======================
            break;
    }
    strcat(dest, code);
}

// Append assembly code for the instruction "sub" into dest.
void parse_sub(char *dest) {
    strcat(dest, "// sub\n"); // TODO: Add Hack assembly here!
    // SP--;RAM[SP-1]=RAM[SP-1]-RAM[SP];
    strcat(dest,"@SP\nM=M-1\nA=M\nD=-M\nA=A-1\nD=D+M\nM=D\n");
    // ======================
}

// Append assembly code for the instruction "neg" into dest.
void parse_neg(char *dest) {
    strcat(dest, "// neg\n"); // TODO: Add Hack assembly here!

    // SP--;RAM[SP]=!RAM[SP]+1;SP++
    strcat(dest,"@SP\nM=M-1\nA=M\nD=!M\nM=D\nM=M+1\n@SP\nM=M+1\n");
    // ======================
}

// Append assembly code for the instruction "and" into dest.
void parse_and(char *dest) {
    strcat(dest, "// and\n"); // TODO: Add Hack assembly here!
    
    // SP--;RAM[SP-1]=RAM[SP-1]&RAM[SP];
    strcat(dest,"@SP\nM=M-1\nA=M\nD=M\nA=A-1\nD=D&M\nM=D\n");
    // ======================
}

// Append assembly code for the instruction "or" into dest.
void parse_or(char *dest) {
    strcat(dest, "// or\n"); // TODO: Add Hack assembly here!
    
    // SP--;RAM[SP-1]=RAM[SP-1]|RAM[SP];
    strcat(dest,"@SP\nM=M-1\nA=M\nD=M\nA=A-1\nD=D|M\nM=D\n");
    // ======================
}

// Append assembly code for the instruction "not" into dest.
void parse_not(char *dest) {
    strcat(dest, "// not\n"); // TODO: Add Hack assembly here!
    
    // SP--;RAM[SP]=!RAM[SP];SP++
    strcat(dest,"@SP\nM=M-1\nA=M\nD=!M\nM=D\n@SP\nM=M+1\n");
    // ======================
}

// Append assembly code for the instruction "eq" into dest.
void parse_eq(char *filename, char *dest) {
    char new_label1[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label1);
    char new_label2[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label2);

    char code[1000];
    sprintf(code, "// eq\n"); // TODO: Add Hack assembly here!
    
    // SP--;RAM[SP-1]=RAM[SP-1]-RAM[SP];
    strcat(code+strlen(code),"@SP\nM=M-1\nA=M\nD=-M\nA=A-1\nD=D+M\nM=D\n");
    // branch
    strcat(code+strlen(code),"@%s\nD;JEQ\nD=1\n(%s)\nD=D-1\n",new_label1);
    // paste result back to RAM[SP-1]
    strcat(code+strlen(code),"@SP\nA=M-1\nM=D\n");
    // ========================
    strcat(dest, code);
}

// Append assembly code for the instruction "lt" into dest.
void parse_lt(char *filename, char *dest) {
    char new_label1[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label1);
    char new_label2[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label2);

    char code[1000];
    sprintf(code, ""); // TODO: Add Hack assembly here!

    // SP--;RAM[SP-1]=RAM[SP-1]-RAM[SP];
    strcat(code+strlen(code),"@SP\nM=M-1\nA=M\nD=-M\nA=A-1\nD=D+M\nM=D\n");
    // branch
    strcat(code+strlen(code),"@%s\nD;JLT\n@SP\nA=M-1\nM=0\n@%s\n0;JMP\n(%s)\n@SP\nA=M-1\nM=-1\n(%s)\n",new_label1,new_label2,new_label1,new_label2);
    // ========================

    strcat(dest, code);
}

// Append assembly code for the instruction "gt" into dest.
void parse_gt(char *filename, char *dest) {
    char new_label1[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label1);
    char new_label2[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label2);

    char code[1000];
    sprintf(code, ""); // TODO: Add Hack assembly here!

    // SP--;RAM[SP-1]=RAM[SP-1]-RAM[SP];
    strcat(code+strlen(code),"@SP\nM=M-1\nA=M\nD=-M\nA=A-1\nD=D+M\nM=D\n");
    // branch
    strcat(code+strlen(code),"@%s\nD;JGT\n@SP\nA=M-1\nM=0\n@%s\n0;JMP\n(%s)\n@SP\nA=M-1\nM=-1\n(%s)\n",new_label1,new_label2,new_label1,new_label2);
    // ========================

    strcat(dest, code);
}

// Append assembly code for the instruction "label [label]" into dest.
void parse_label(char *filename, Token *label, char *dest) {
    char code[1000] = "";
    sprintf(code, "// Label\n"); // TODO: Add Hack assembly here!
    
    strcat(code+strlen(code),"(%s)\n",label->value.str_val);
    // ========================
    strcat(dest, code);
}

// Append assembly code for the instruction "goto [label]" into dest.
void parse_goto(char *filename, Token *label, char *dest) {
    char code[1000] = "";
    sprintf(code, "// Goto\n"); // TODO: Add Hack assembly here!
    
    strcat(code+strlen(code),"@%s\n0;JMP\n",label->value.str_val);
    // ========================
    strcat(dest, code);
}`

// Append assembly code for the instruction "if-goto [label]" into dest.
void parse_ifgoto(char *filename, Token *label, char *dest) {
    char code[1000] = "";
    sprintf(code, "// If-goto\n"); // TODO: Add Hack assembly here!
    
    // if-goto: SP--;@label RAM[SP];JNE
    strcat(code+strlen(code),"@SP\nM=M-1\nA=M\nD=M\n@%s\nD;JNE\n",label->value.str_val);
    // ========================
    
    strcat(dest, code);
}

// Returns a label name of the form auto$[filename]$[number], where [number] is unique to the file.
// Labels with names specified directly in VM code are translated into the form manual$[filename]$[label_name], so
// there's no danger of duplication.
void get_next_label_name(char *filename, char *dest) {
    static int label_count = 0;
    char buffer[MAX_LINE_LENGTH];
    sprintf(buffer, "auto$%s$%d", filename, label_count);
    strcat(dest, buffer);
    label_count++;
}
