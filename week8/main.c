#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symboltable.h"
#include "token.h"

// Lexing functions
void lex_file(SymbolTable *labels, FILE *input, FILE *output);
void lex_line(int *rom_address, SymbolTable *labels, const char *line, FILE *output);
int lex_token(Token *dest, const char *line);
void lex_label(const char *line, int rom_address, SymbolTable *labels);

// Parsing functions
void parse_file(const SymbolTable *labels, SymbolTable *variables, FILE *input, FILE *output);
int get_next_instruction(Token *dest[], FILE *input);
void parse_instruction(const SymbolTable *labels, SymbolTable *variables, Token *instruction[], int length,
                       FILE *output);
void parse_a_instruction(const SymbolTable *labels, SymbolTable *variables, Token *instruction[], char *dest);
void parse_c_instruction(Token *instruction[], int length, char *dest_str);
void parse_c_comp(Token *instruction[], int length, char *comp_str);
void parse_c_jump(Token *instruction[], int length, char *jump_str);
void parse_c_dest(Token *instruction[], int length, char *dest_str);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Please supply two arguments: an input .asm file, and an output .hack file.");
        exit(EXIT_FAILURE);;
    }
    char *input_name = argv[1];
    char *output_name = argv[2];

    SymbolTable *labels = malloc_table();
    SymbolTable *variables = malloc_table();

    FILE *input = fopen(input_name, "r");
    if (input == NULL) {
        exit(EXIT_FAILURE);
    }
    FILE *lex_output = fopen("temp.lex", "w");
    if (lex_output == NULL) {
        exit(EXIT_FAILURE);
    }
    lex_file(labels, input, lex_output);
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
    parse_file(labels, variables, lex_input, output);
    fclose(lex_input);
    fclose(output);

    free_table(labels);
    free_table(variables);
    return EXIT_SUCCESS;
}

// Outputs a tokenised version of input to output while populating labels.
void lex_file(SymbolTable *labels, FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    int rom_address = 0;
    while (fgets(line, MAX_LINE_LENGTH, input) != NULL) {
        lex_line(&rom_address, labels, line, output);
    }
}

// Reads the next line from input, tokenises it, updates the label table, and writes the resulting tokens to output.
// Increments *rom_address if the current line contains an instruction (rather than e.g. labels, comments, etc.)
void lex_line(int *rom_address, SymbolTable *labels, const char *line, FILE *output) {
    int pos = 0;
    bool tokens_on_line = false;

    // In each iteration of this loop, everything up to line[pos] has been lexed.
    while ((line[pos] != '\n') && (line[pos] != '\r')) {
        Token *next = malloc_token();

        // INSERT CODE: If you see whitespace, ignore it.
        if(line[pos]==' '){
            pos++;
            continue;
        }
        //=====================
        // INSERT CODE: If you see a comment, ignore the rest of the line.
        if(line[pos]=='/' && pos+1<(int)strlen(line) && line[pos+1]=='/'){
            break;
        }
        //=====================
        // Add labels to the symbol table.
        if (line[pos] == '(') {
            lex_label(line+pos, *rom_address, labels);
            break;
        }

        // Otherwise, we have at least one non-newline token, so lex it.
        tokens_on_line = true;
        pos += lex_token(next, line + pos);
        write_token(next, output);
        free_token(next);
    }

    if (tokens_on_line) {
        // INSERT CODE: If the line you just lexed contained tokens (i.e. if it corresponds to a line of machine code),
        // lex the newline at the end and update rom_address.
        Token *newline = malloc_token();
        lex_token(newline,"\n");
        write_token(newline, output);
        (*rom_address)++;
        free_token(newline);
        // =====================
    }
}

// Assuming line contains a label and starts with "(" (so no leading whitespace), extracts the label and adds it to
// the symbol table with ROM address rom_address. The line may end with a comment (e.g. "(LOOP) // Loop here").
void lex_label(const char *line, int rom_address, SymbolTable *labels) {
    // Your code here!
    int n=0;
    int length=0;
    char name[256]={0};
    while(line[n]!='(' && line[n]){
        if(line[n]==')'){
            break;
        }
        name[length]=line[n];
        length++;
        n++;
    }
    add_to_table(labels, name, rom_address);
    // =================
}

// Reads the next token from a non-empty, non-label, non-comment line into dest, then returns the number of characters
// in that token.
int lex_token(Token *dest, const char *line) {
    static bool at_previous = false; // Will be set to 1 if the last time this function was called, it lexed an @.
    int length;

    if (line[0] == '\n') { // Newlines
        dest->type = NEWLINE;
        length = 1;
    } else if (line[0] == '@' || line[0] == '+' || line[0] == '-' || line[0] == '&' || line[0] == '|'
               || line[0] == ';' || line[0] == '!' || line[0] == '=') { // Symbols
        dest->type = SYMBOL;
        dest->value.char_val = line[0];
        length = 1;
    } else if (line[0] >= '0' && line[0] <= '9') { // Integer literals
        // Could be either a 0 or 1 inside a C-instruction, or something longer inside an A-instruction.
        dest->type = INTEGER_LITERAL;
        char literal[MAX_LINE_LENGTH];
        for(length = 0; line[length] >= '0' && line[length] <= '9'; length++) {
            literal[length] = line[length];
        }
        literal[length] = '\0';
        dest->value.int_val = strtol(literal, NULL, 10);
    }
    // INSERT CODE: Fill in the rest of the cases for keywords and identifiers.
    else if(!at_previous && (line[0]=='A'||line[0]=='M'||line[0]=='D')){
        dest->type=KEYWORD;
        switch(line[0]){
            case 'A': dest->value.key_val=KW_A;break;
            case 'M': dest->value.key_val=KW_M;break;
            case 'D': dest->value.key_val=KW_D;break;
        }
        length=1;
    }else{
        // extract the word
        char* string=(char*)calloc(256,sizeof(char));
        int k=0;
        while(line[k] && line[k]!=' ' && line[k]!='\n' && line[k]!='\r'){
            string[k]=line[k];
            k++;
        }
        char* keywords[]={"JGT", "JEQ", "JLT", "JGE", "JNE", "JLE", "JMP",
    "SCREEN", "KBD", "SP", "LCL", "ARG", "THIS", "THAT",
    "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"};
        int n=0;
        bool iskey=false;
        for(;n<30;n++){
            if(!strcmp(keywords[n],string)){
                iskey=true;
                break;
            }
        }
        if(iskey){
            dest->value.key_val=n;
            dest->type=KEYWORD;
            length=strlen(keywords[n]);
        }else{
            length=strlen(string);
            dest->value.str_val=calloc(length+1,sizeof(char));
            strcpy(dest->value.str_val,string);
            dest->type=IDENTIFIER;
        }
    }
    // =================
    at_previous = (line[0] == '@');
    return length;
}

// Labels should be a pre-populated label symbol table. Input should be a tokenised file. Populates the variables symbol
// table and writes Hack machine code to output.
void parse_file(const SymbolTable *labels, SymbolTable *variables, FILE *input, FILE *output) {
    Token *instruction[MAX_LINE_LENGTH];
    while (1) {
        int length = get_next_instruction(instruction, input);
        if (length == 0) {
            free_token(instruction[0]);
            break;
        }
        parse_instruction(labels, variables, instruction, length, output);
        for(int i=0; i<length; i++) {
            free_token(instruction[i]);
        }
    }
}

// Copies a list of tokens corresponding to the next Hack instruction into dest, omitting the newline. Return number
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

// Parse the given instruction (containing length operands) and write the corresponding Hack code to the output file.
void parse_instruction(const SymbolTable *labels, SymbolTable *variables, Token *instruction[], int length,
                       FILE *output) {
    char hack_instruction[18] = "";
    if (instruction[0]->type==SYMBOL && instruction[0]->value.char_val=='@') // INSERT CODE: Fill in the right condition here
        parse_a_instruction(labels, variables, instruction, hack_instruction);
    } else {
        parse_c_instruction(instruction, length, hack_instruction);
    }
    strcat(hack_instruction, "\n");
    fputs(hack_instruction, output);
}

// Parse the operand of the given A instruction, updating the variables table accordingly, and put the corresponding
// Hack command into dest.
void parse_a_instruction(const SymbolTable *labels, SymbolTable *variables, Token *instruction[], char *dest) {
    Token *operand = instruction[1];
    int value_to_load;
    if (operand->type == INTEGER_LITERAL) {
        value_to_load = operand->value.int_val;
    } else if (operand->type == IDENTIFIER) {
        // INSERT CODE: If the token is a label, set value_to_load to the correct ROM address. Otherwise, it's a
        // variable; set value_to_load to the correct RAM address, first adding the variable to the symbol table if
        // needed.
        int label_index=get_table_entry(labels,operand->value.str_val);
        int var_index=get_table_entry(variables,operand->value.str_val);
        if(label_index!=-1){
            value_to_load=labels->tabel_array[label_index]->address;
        }else if(var_index!=-1){
            value_to_load=variables->tabel_array[var_index]->address;
        }else{
            int address=16+variables->table_length;
            add_to_table(variables, operand->value.str_val, address);
            value_to_load=address;
        }
        // ================
    } else {
        // INSERT CODE: If the token is a keyword, set value_to_load to the correct integer literal.
        switch(operand->value.key_val){
            case SCREEN: value_to_load=16384;break;
            case KBD: value_to_load=24576;break;
            case SP: value_to_load=0;break;
            case LCL: value_to_load=1;break;
            case ARG: value_to_load=2;break;
            case THIS: value_to_load=3;break;
            case THAT: value_to_load=4;break;
            default: value_to_load=operand->value.key_val-R0;break;
        }
        // ====================
    }
    // Writes the address to dest, padding with zeroes.
    int_to_bin_string(value_to_load, dest);
}

// Parse the operand of the given C instruction and put the corresponding Hack command into dest_str.
void parse_c_instruction(Token *instruction[], int length, char *dest_str) {
    char comp_op[8];
    char dest_op[4];
    char jump_op[4];

    parse_c_comp(instruction, length, comp_op);
    parse_c_dest(instruction, length, dest_op);
    parse_c_jump(instruction, length, jump_op);

    strcat(dest_str, "111");
    strcat(dest_str, comp_op);
    strcat(dest_str, dest_op);
    strcat(dest_str, jump_op);
}

// Given a list of tokens of length [length] forming a C-instruction, copy the jump operand into jump_str.
void parse_c_jump(Token *instruction[], int length, char *jump_str) {
    // INSERT CODE: Compute the C-instruction's jump operand and copy it into jump_str.
    strcpy(jump_str,"000");
    if(length >= 2 && instruction[length-2]->type == SYMBOL && instruction[length-2]->value.char_val == ';') {
        switch(instruction[length-1]->value.key_val){
            case JGT: strcpy(jump_str,"001");break;
            case JEQ: strcpy(jump_str,"010");break;
            case JLT: strcpy(jump_str,"100");break;
            case JGE: strcpy(jump_str,"011");break;
            case JNE: strcpy(jump_str,"101");break;
            case JLE: strcpy(jump_str,"110");break;
            case JMP: strcpy(jump_str,"111");break;
            default: exit(1);break;
        }
    }
    //======================
}

// Given a list of tokens of length [length] forming a C-instruction in assembly, copy the dest operand into dest_str.
void parse_c_dest(Token *instruction[], int length, char *dest_str) {
    // INSERT CODE: Compute the C-instruction's dest operand and copy it into jump_str.
    int dest_end = 0;
    for(int i=0; i<length; i++) {
        if (instruction[i]->type == SYMBOL && instruction[i]->value.char_val == '=') {
            dest_end=i;
            break;
        }
    }
    strcpy(dest_str, "000");
    for(int i=0;i<dest_end;i++){
        switch(instruction[i]->value.key_val){
            case KW_A: dest_str[0]='1';break;
            case KW_M: dest_str[2]='1';break;
            case KW_D: dest_str[1]='1';break;
        }
    }
    
    // ====================
}

// Given a list of tokens of length [length] forming a C-instruction, copy the comp operand into comp_str.
void parse_c_comp(Token *instruction[], int length, char *comp_str) {
    // Set comp_start to the index of the start of the computation part of the instruction, i.e. after the = if there
    // is one or at the start otherwise.
    int comp_start = 0;
    for(int i=0; i<length; i++) {
        if (instruction[i]->type == SYMBOL && instruction[i]->value.char_val == '=') {
            comp_start = i+1;
            break;
        }
    }
    // Set comp_end to the index of the end of the computation part of the instruction, i.e. before the ";" if there is
    // one or at the end otherwise.
    int comp_end;
    if(length >= 2 && instruction[length-2]->type == SYMBOL && instruction[length-2]->value.char_val == ';') {
        comp_end = length-3;
    } else {
        comp_end = length-1;
    }

    // Number of characters in the computation part of the instruction.
    int comp_length = comp_end - comp_start + 1;

    strcpy(comp_str, "0000000");
    switch(comp_length) {
        case 1: // Must be 0, 1, A, D or M
            switch (instruction[comp_start]->type) {
                case INTEGER_LITERAL:
                    switch (instruction[comp_start]->value.int_val) {
                        case 0:
                            strcpy(comp_str, "0101010");
                            break;
                        case 1:
                            strcpy(comp_str, "0111111");
                            break;
                        default:
                            exit(EXIT_FAILURE);
                    }
                    break;
                case KEYWORD:
                    switch (instruction[comp_start]->value.key_val) {
                        case KW_A:
                            strcpy(comp_str, "0110000");
                            break;
                        case KW_D:
                            strcpy(comp_str, "0001100");
                            break;
                        case KW_M:
                            strcpy(comp_str, "1110000");
                            break;
                        default:
                            exit(EXIT_FAILURE);
                    }
                    break;
                default:
                    exit(EXIT_FAILURE);
            }
            break;
        case 2: // Must start with ! or -
            switch (instruction[comp_start]->value.char_val) {
                case '!':
                    // INSERT CODE: Deal with !A, !D or !M.
                    switch(instruction[comp_start+1]->value.key_val){
                        case KW_A: strcpy(comp_str, "0110001");break;
                        case KW_M: strcpy(comp_str, "1110001");break;
                        case KW_D: strcpy(comp_str, "0001101");break;
                        default: exit(1);
                    }
                    //===============
                    break;
                case '-':
                    if (instruction[comp_start + 1]->type == INTEGER_LITERAL) { // Must be -1
                        strcpy(comp_str, "0111010");
                        break;
                    }
                    switch (instruction[comp_start + 1]->value.key_val) {
                        case KW_A:
                            strcpy(comp_str, "0110011");
                            break;
                        case KW_D:
                            strcpy(comp_str, "0001111");
                            break;
                        case KW_M:
                            strcpy(comp_str, "1110011");
                            break;
                        default:
                            exit(EXIT_FAILURE);
                    }
                    break;
                default:
                    exit(EXIT_FAILURE);
            }
            break;
        case 3: {
            // Without these as shorthand things get truly horrible
            bool operands_ad = (instruction[comp_start]->value.key_val == KW_A &&
                                instruction[comp_start + 2]->value.key_val == KW_D);
            bool operands_da = (instruction[comp_start]->value.key_val == KW_D &&
                                instruction[comp_start + 2]->value.key_val == KW_A);
            bool operands_dm = (instruction[comp_start]->value.key_val == KW_D &&
                                instruction[comp_start + 2]->value.key_val == KW_M);
            bool operands_md = (instruction[comp_start]->value.key_val == KW_M &&
                                instruction[comp_start + 2]->value.key_val == KW_D);
            switch(instruction[comp_start+1]->value.char_val) {
                case '+':
                    if (instruction[comp_start+2]->type == INTEGER_LITERAL) { // Must be A+1, D+1 or M+1
                        switch(instruction[comp_start]->value.key_val) {
                            case KW_A: strcpy(comp_str, "0110111"); break;
                            case KW_D: strcpy(comp_str, "0011111"); break;
                            case KW_M: strcpy(comp_str, "1110111"); break;
                            default: exit(EXIT_FAILURE);
                        }
                        // Otherwise, the only options are A+D, D+A, M+D or D+M.
                    } else if (operands_ad || operands_da) {
                        strcpy(comp_str, "0000010"); // A+D or D+A
                    } else if (operands_md || operands_dm) {
                        strcpy(comp_str, "1000010"); // M+D or D+M
                    } else {
                        exit(EXIT_FAILURE);
                    } break;
                case '-':
                    // INSERT CODE: Deal with A-1, D-1, M-1, A-D, D-A, M-D or D-M,
                    if (instruction[comp_start+2]->type == INTEGER_LITERAL) { // Must be A-1, D-1 or M-1
                        switch(instruction[comp_start]->value.key_val) {
                            case KW_A: strcpy(comp_str, "0110010"); break;
                            case KW_D: strcpy(comp_str, "0001110"); break;
                            case KW_M: strcpy(comp_str, "1110010"); break;
                            default: exit(EXIT_FAILURE);
                        }
                        // Otherwise, the only options are A+D, D+A, M+D or D+M.
                    } else if (operands_ad) {
                        strcpy(comp_str, "0000111"); // A-D
                    } else if (operands_da) {
                        strcpy(comp_str, "0010011"); // D-A
                    } else if (operands_md) {
                        strcpy(comp_str, "1000111"); // M-D
                    } else if (operands_dm) {
                        strcpy(comp_str, "1010011"); // D-M
                    } else {
                        exit(EXIT_FAILURE);
                    } break;
                    // ===================
                    break;
                case '&': // Must be A&D, D&A, M&D or D&M
                    if (operands_ad || operands_da) {
                        strcpy(comp_str, "0000000"); // A&D
                    } else if (operands_md || operands_dm) {
                        strcpy(comp_str, "1000000"); // D&A
                    } else {
                        exit(EXIT_FAILURE);
                    } break;
                case '|': // Must be A|D, D|A, M|D or D|M
                    if (operands_ad || operands_da) {
                        strcpy(comp_str, "0010101"); // A&D
                    } else if (operands_md || operands_dm) {
                        strcpy(comp_str, "1010101"); // D&A
                    } else {
                        exit(EXIT_FAILURE);
                    } break;
                default: exit(EXIT_FAILURE);
            } break;
        }
        default: exit(EXIT_FAILURE);
    }
}

// Converts the given integer into a 16-bit binary value, storing the result in dest. Pad with zeroes at the left.
void int_to_bin_string(int num, char *dest) {
    for(int i=15; i>=0; i--, num /= 2) {
        int binary_digit = num % 2;
        dest[i] = (char)(binary_digit + '0');
    }
    dest[16] = '\0';
}
