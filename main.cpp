
//
// Rylan Dmello
// following along with: https://llvm.org/docs/tutorial/LangImpl01.html
//

#include <iostream>
#include <string>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cassert>

enum Token {
    tok_eof         = -1,
    tok_def         = -2,   // commands
    tok_extern      = -3,
    tok_identifier  = -4,   // primary
    tok_number      = -5,
};

static std::string IdentifierStr;   // set if tok_identifier
static double NumVal;               // set if tok_number

// gettok - Return the next token from standard input
// Q - why is this a static fcn??
static int gettok() {
    static int LastChar = ' ';

    // skip whitespace
    while (std::isspace(LastChar)) LastChar = std::getchar();

    /* def, extern, and identifier: [a-zA-Z][a-zA-Z0-9]* */
    if (std::isalpha(LastChar)) 
    { 
        IdentifierStr = LastChar;
        while (std::isalnum(LastChar = getchar())) IdentifierStr += LastChar;
        if (IdentifierStr == "def") return tok_def;
        if (IdentifierStr == "extern") return tok_extern;
        return tok_identifier;
    }
    /* numbers f64 only */
    else if (std::isdigit(LastChar) || (LastChar = '.') || (LastChar = '-') || (LastChar = '+')) 
    {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = std::getchar();
        } while (std::isdigit(LastChar) || LastChar == '.');

        NumVal = std::strtod(NumStr.c_str(), 0);
        return tok_number;
    } 
    /* comments */
    else if (LastChar == '#') 
    {
        do
            LastChar = std::getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF) return gettok();
    }
    /* Check for EOF, don't eat the EOF */
    else if (LastChar == EOF) 
    { 
        return tok_eof; 
    }
    /* Otherwise, just return the character as its ascii value */
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

int main() {
    std::cout << "Hello? " << std::endl;

    assert(gettok() == tok_def);
    return 0;
}


