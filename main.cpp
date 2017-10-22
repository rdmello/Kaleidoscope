
//
// Rylan Dmello
// following along with: https://llvm.org/docs/tutorial/LangImpl01.html
//

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>

#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cassert>

/* for llvm::make_unique */
#include "llvm/ADT/STLExtras.h"

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
    else if (std::isdigit(LastChar) || (LastChar == '.'))
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
    BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, 
        std::unique_ptr<ExprAST> RHS): 
        Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
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

// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto Result = llvm::make_unique<NumberExprAST>(NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
}

// forward-defining ParseExpression
static std::unique_ptr<ExprAST> ParseExpression();

// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr()
{
    getNextToken(); // eat (
    auto V = ParseExpression();
    if (!V) return nullptr;
    if (CurTok != ')') return LogError("Expected ')'");
    getNextToken(); // eat )
    return V;
}

// identifierexpr
//      :: = identifier
//      :: = identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::string IdName = IdentifierStr;
    
    getNextToken(); // eat identifier

    if (CurTok != '(') // simple variable ref
        return llvm::make_unique<VariableExprAST>(IdName);

    // Fcn call
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAST> > Args;
    if (CurTok != ')') {
        while (1) {
            if (auto Arg = ParseExpression())
            {
                Args.push_back(std::move(Arg));
            }
            else return nullptr;

            if (CurTok == ')') break;

            if (CurTok != ',') 
                return LogError("Expected ')' or ',' in arg list");
            
            getNextToken();
        }
    }

    // Eat the ')'
    getNextToken();

    return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

// primary 
//      ::= identifierexpr
//      ::= numberexpr
//      ::= parenexpr 
static std::unique_ptr<ExprAST> ParsePrimary()
{
    switch (CurTok)
    {
    default:
        return LogError("unknown token when expecting an expression");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    }
}

/* 2.5 Binary expression parsing */

// BinopPrecedence - this holds the precendence for defined binary operators
static std::map<char, int> BinopPrecedence;

// GetTokPrecedence - get precedence of pending binary op token
static int GetTokPrecedence()
{
    if (!std::isprint(CurTok)) return -1;

    // make sure its a declared binop
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0) return -1;
    return TokPrec;
}

// forward declaring
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, 
    std::unique_ptr<ExprAST> LHS);

// expression
//      ::= primary binopRHS
static std::unique_ptr<ExprAST> ParseExpression() 
{
    auto LHS = ParsePrimary();
    if (!LHS) return nullptr;
    return ParseBinOpRHS(0, std::move(LHS));
}

// binOpRHS
//      ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, 
    std::unique_ptr<ExprAST> LHS)
{
    // if this is a binop, find its precedence
    while(1)
    {
        int TokPrec = GetTokPrecedence();

        // if this is a binop that binds at least as tightly as the current 
        // binop, consume it, otherwise we are done
        if (TokPrec < ExprPrec) return LHS;

        // okay, we know this is a binop
        int BinOp = CurTok;
        getNextToken(); // eat binop

        // Parse the primary expressino after the binary operator
        auto RHS = ParsePrimary();
        if (!RHS) return nullptr;

        // if the binop binds less tightly with RHS than the operator after
        // RHS, let the pending operator take RHS as its LHS
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec) 
        {
            RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
            if (!RHS) return nullptr;
        }

        // Merge LHS + RHS
        LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS), 
            std::move(RHS));
    } // loop around to the top of the while loop
}

// prototype
//      ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype()
{
    if (CurTok != tok_identifier) 
        return LogErrorP("Expected fcn name in prototype");

    std::string FnName = IdentifierStr;
    getNextToken();

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");
    
    // read the list of argument names
    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier)
    {
        ArgNames.push_back(IdentifierStr);
    }
    if (CurTok != ')')
        return LogErrorP("Expected ')' in prototype");
    
    // success
    getNextToken();         // eat ')'

    return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

// fcn definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition()
{
    getNextToken(); // eat def
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;

    if (auto E = ParseExpression())
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    
    return nullptr;
}

// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern()
{
    getNextToken(); // eat extern
    return ParsePrototype();
}

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
    if (auto E = ParseExpression())
    {
        // make an anonymous proto
        auto Proto = llvm::make_unique<PrototypeAST>("", 
            std::vector<std::string>());
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

/*
 * Top-Level Parsing
 */
static void HandleDefinition()
{
    if (ParseDefinition())
    {
        std::fprintf(stderr, "Parsed a function definition.\n");
    }
    else
    {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleExtern()
{
    if (ParseExtern())
    {
        std::fprintf(stderr, "Parsed an extern.\n");
    }
    else
    {
        // skip token for error recovery
        getNextToken();
    }
}

static void HandleTopLevelExpression()
{
    if (ParseTopLevelExpr())
    {
        std::fprintf(stderr, "Parsed a top-level expression.\n");
    }
    else
    {
        // skip token for error recovery
        getNextToken();
    }
}



// top ::= definition | external | expression | ';'
static void MainLoop()
{
    while(1)
    {
        std::fprintf(stderr, "ready!> ");
        switch (CurTok)
        {
        case tok_eof:
            return;
        case ';':
            getNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

int main()
{
    std::cout << "Hello? " << std::endl;

    // install standard binary operators
    // 1 is lowest precedence
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40; // highest
    // TODO: add division operator

    // Prime the first token
    std::fprintf(stderr, "ready!> ");
    getNextToken();

    // Run the main "interpreter" loop
    MainLoop();

    // assert(gettok() == tok_def);
    return 0;
}
