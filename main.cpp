
//
// Rylan Dmello
// following along with: https://llvm.org/docs/tutorial/LangImpl01.html
//

#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cassert>

/*
 * CH 1: BUILDING A LEXER
 */
enum Token
{
    tok_eof = -1,
    tok_def = -2, // commands
    tok_extern = -3,
    tok_identifier = -4, // primary
    tok_number = -5,
};

static std::string IdentifierStr; // set if tok_identifier
static double NumVal;             // set if tok_number

// gettok - Return the next token from standard input
// Q - why is this a static fcn??
static int gettok()
{
    static int LastChar = ' ';

    // skip whitespace
    while (std::isspace(LastChar))
        LastChar = std::getchar();

    /* def, extern, and identifier: [a-zA-Z][a-zA-Z0-9]* */
    if (std::isalpha(LastChar))
    {
        IdentifierStr = LastChar;
        while (std::isalnum(LastChar = getchar()))
            IdentifierStr += LastChar;
        if (IdentifierStr == "def")
            return tok_def;
        if (IdentifierStr == "extern")
            return tok_extern;
        return tok_identifier;
    }
    /* numbers f64 only */
    else if (std::isdigit(LastChar) || (LastChar = '.'))
    {
        std::string NumStr;
        do
        {
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

        if (LastChar != EOF)
            return gettok();
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

/*
 * CH 2: WRITING A PARSER WITH AST DEFINITION
 */

/* ExprAST: Base class for all expression nodes */
class ExprAST
{
public:
    virtual ~ExprAST() {}
};

/* NumberExprAST: expression class for numeric literals like "1.0" */
class NumberExprAST : public ExprAST
{
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
};

/* VariableExprAST: expression class for referencing a variable, like "a" */
class VariableExprAST : public ExprAST
{
    std::string Name;

public:
    VariableExprAST(const std::string &Name): Name(Name) {}
};

/* BinaryExprAST: expression class for a binary operator */
class BinaryExprAST : public ExprAST 
{
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS;

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS, 
        std::unique_ptr<ExprAST> RHS): 
        Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

/* CallExprAST: expression class for function calls */
class CallExprAST : public ExprAST 
{
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST> > Args;

public:
    CallExprAST(const std::string &Callee, 
        std::vector<std::unique_ptr<ExprAST> > Args): 
        Callee(Callee), Args(std::move(Args)) {}
};

/* PrototypeAST - represents the prototype for a function,
 * captures its name, argument names
 */
class PrototypeAST
{
    std::string Name;
    std::vector<std::string> Args;
    
public: 
    PrototypeAST(const std::string& name, std::vector<std::string> Args): 
        Name(name), Args(std::move(Args)) {}

    const std::string &getName() const { return Name; }
};

/* FunctionAST - represents a function definition itself */
class FunctionAST
{
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;
    
public:
    FunctionAST(std::unique_ptr<PrototypeAST> Proto,
        std::unique_ptr<ExprAST> Body): 
        Proto(std::move(Proto)), Body(std::move(Body)) {}
};

/* CurTok/getNextToken: provide a simple token buffer
 * CurTok is the current token the parser is looking at
 * getNextToken reads another token from the lexer and updates CurTok with its
 * results 
 */
static int CurTok;
static int getNextToken() 
{
    return CurTok = gettok();
}

/* LogError - these are helper functions for error handling */
std::unique_ptr<ExprAST> LogError(const char* Str)
{
    fprintf(stderr, "LogError: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* Str)
{
    LogError(Str);
    return nullptr;    
}


int main()
{
    std::cout << "Hello? " << std::endl;

    assert(gettok() == tok_def);
    return 0;
}
