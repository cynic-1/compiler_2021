#ifndef PROJECT_TYPE_H
#define PROJECT_TYPE_H

#include <fstream>
#include <deque>
#include <map>
#include <string>
#include <set>
#include <iostream>
#include <vector>

#define READ_EOF -1
#define READ_ERROR -3

using namespace std;

// 为了方便替换字符串，把需要替换的放在前面
enum TokenType {
    Add             =   0,
    Minus           =   1,
    Mul             =   2,
    Div             =   3,
    Mod             =   4,
    Int             =   5,
    Void            =   6,
    Lt              =   7,
    Leq             =   8,
    Gt              =   9,
    Geq             =   10,
    Eq              =   11,
    Neq             =   12,
    Break           =   13,
    Continue        =   14,
    Return          =   15,
    ConstToken      =   16,
    Comma           =   17,
    Semicolon       =   18,
    Ident           =   19,
    Main            =   20,
    IntConst        =   21,
    StrConst        =   22,
    If              =   23,
    Else            =   24,
    Not             =   25,
    And             =   26,
    Or              =   27,
    While           =   28,
    Assign          =   29,
    LParen          =   30,
    RParen          =   31,
    LBracket        =   32,
    RBracket        =   33,
    LBrace          =   34,
    RBrace          =   35
};

static const string typeNames[] = {"add", "sub", "mul", "sdiv", "srem", "i32", "void",
                                   "slt", "sle", "sgt", "sge", "eq", "ne"};

typedef struct {
    int tokenType;
    string token;
    int lineNum;
} TokenContext;

// all types of syntax components
enum SyntaxType {
    CompUnit, ConstDecl, BType, ConstDef, VarDecl, VarDef, FuncDef, MainFuncDef, FuncType,
    FuncFParams, FuncFParam, Block, Exp, Cond, LVal, Number, UnaryOp, FuncRParams,
    MulExp, AddExp, RelExp, EqExp, LAndExp, LOrExp, ConstExp,
    Decl, ConstInitVal, InitVal, BlockItem, Stmt, PrimaryExp, UnaryExp, SyntaxTypeNum
};

enum ScopeKind {
    FuncScope, NormalScope, LoopScope
};

enum IdentKind {
    Var, Const, Function, Param, Temp
};

enum IdentType {
    IntType = 5, VoidType = 6
};

extern ifstream inputFile;
extern ofstream irFile;

// defined in lex_analyser.cpp
extern string outputBuffer;
extern TokenContext tokenContextBuffer;
extern char curChar;
extern TokenContext curTokenContext;
extern deque<TokenContext> tokenPeekQueue;
extern int peekPos;

// defined in syntax_analyser.cpp 全局预检查
extern bool isPreCheck;


#endif //PROJECT_TYPE_H
