#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include "symboltable.h"
#include "token.h"

// Lexing functions
void lex_file(SymbolTable *labels, FILE *input, FILE *output);
void lex_line(int *rom_address, SymbolTable *labels, const char *line, FILE *output);
int lex_token(Token *dest, const char *line);
void lex_label(const char *line, int rom_address, SymbolTable *labels);


void test();
int main(void) {
    test();
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
        // ========================================
        // INSERT CODE: If you see a comment, ignore the rest of the line.
        if(line[pos]=='/' && pos+1<(int)strlen(line) && line[pos+1]=='/'){
            break;
        }
        // ========================================
        // Add labels to the symbol table.
        if (line[pos] == '(') {
            lex_label(line+pos, *rom_address, labels);
            break;
        }

        // Otherwise, we have at least one non-newline token, so lex it.
        tokens_on_line = true;
        pos += lex_token(next, line + pos);
        printf("pos=%d\n",pos);
        write_token(next, output);
        free_token(next);
    }

    if (tokens_on_line) {
        // INSERT CODE: If the line you just lexed contained tokens (i.e. if it corresponds to a line of machine code),
        // lex the newline at the end and update rom_address.
        Token *next = malloc_token();
        pos += lex_token(next, "\n");
        write_token(next, output);
        free_token(next);
        (*rom_address)++;
        return;
        // ========================================
    }
}

// Assuming line contains a label and starts with "(" (so no leading whitespace), extracts the label and adds it to
// the symbol table with ROM address rom_address. The line may end with a comment (e.g. "(LOOP) // Loop here").
void lex_label(const char *line, int rom_address, SymbolTable *labels) {
    // Your code here!
    char name[256]={0};
    int k=0;
    int index=0;
    bool valid=false;
    while(line[k]){
        if(line[k]=='('){
            valid=true;
        }else if(line[k]==')'){
            valid=false;
            break;
        }else if(valid){
            if(line[k]==' ' || line[k]=='\t' || line[k]=='\v' || line[k]=='\f'){
                printf("no space permitted in a label.\n");
                exit(1);
            }
            name[index]=line[k];
            index++;
        }
        k++;
    }
    for(int i=0;i<(int)strlen(name);i++){
        printf("%d ",name[i]);
    }
    printf("\n");
    add_to_table(labels, name, rom_address);
    // ========================================
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
    else {
        char* keywords[]={"JGT", "JEQ", "JLT", "JGE", "JNE", "JLE", "JMP",
    "SCREEN", "KBD", "SP", "LCL", "ARG", "THIS", "THAT",
    "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "KW_A", "KW_D", "KW_M"};
        char* string=(char*)calloc(256,sizeof(char));
        int i=0;
        int k=0;
        while(line[k] && ( isalpha(line[k]) || isdigit(line[k]) || line[k]=='_' )){
            string[i++]=line[k];
            k++;
        }
            bool flag=false;
            for(int i=0;i<32;i++){
                if(!strcmp(keywords[i],string)){
                    flag=true;
                }
            }
            if(flag){
                dest->type=KEYWORD;
            }else{
                dest->type=IDENTIFIER;
            }
        dest->value.str_val=string;
        length=strlen(string);
    }
    // ========================================
    at_previous = (line[0] == '@');
    return length;
}

void test(){
    SymbolTable* labels=(SymbolTable*)calloc(1,sizeof(SymbolTable));
    labels->table_array=(TableEntry**)calloc(256,sizeof(SymbolTable*));
    labels->table_space=256;
    char line[256]={0};
    strcpy(line," 123  (abc) >?  // comment");
    int rom_address=0;
    lex_label(line, rom_address, labels);
    printf("rom_address=%d,labels->table_length=%d,labels->table_space=%d\n",rom_address,labels->table_length,labels->table_space);
    free(labels->table_array);
    free(labels);
    
    Token* dest=(Token*)calloc(1,sizeof(Token));
    strcpy(line,"");
    strcpy(line,"\n");
    assert(1==lex_token(dest, line));
    assert(dest->type==4);
    
    strcpy(line,"");
    strcpy(line,"@");
    assert(1==lex_token(dest, line));
    assert(dest->type==0);
    
    strcpy(line,"");
    strcpy(line,"123");
    assert(3==lex_token(dest, line));
    assert(dest->type==2);
    
    strcpy(line,"");
    strcpy(line,"@");
    assert(1==lex_token(dest, line));
    assert(dest->type==0);
    
    strcpy(line,"");
    strcpy(line,"aabb");
    assert(4==lex_token(dest, line));
    assert(dest->type==3);
    
    strcpy(line,"");
    strcpy(line,"@");
    assert(1==lex_token(dest, line));
    assert(dest->type==0);
    
    strcpy(line,"");
    strcpy(line,"JMP");
    assert(3==lex_token(dest, line));
    assert(dest->type==1);
    
    strcpy(line,"");
    strcpy(line,"KW_D=KW_D+KW_A");
    assert(4==lex_token(dest, line));
    assert(dest->type==1);
    
    
    strcpy(line,"");
    strcpy(line,"KW_D=KW_D+KW_M;JGE\n");
    FILE* fp=fopen("output","w");
    lex_line(&rom_address, labels, line, fp);
    fclose(fp);
    
}
