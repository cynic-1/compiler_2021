#include "type.h"
#include "globalData.h"


const string tokenTypeStr[40] = {"Ident", "IntConst", "PRINT_STRCON", "Main", "ConstToken", "Int", "Break", "Continue", "If", "Else",
                                 "Not", "And", "Or", "While", "GetInt", "PutInt", "Return", "Add", "Minus",
                                 "Mul", "Div", "Mod", "Lt", "Leq", "Gt", "Geq", "Eq", "Neq",
                                 "Assign", "Semicolon", "Comma", "LParen", "RParen", "LBracket", "RBracket", "LBrace", "RBrace", "Void"};

// reserved words' map
const map<string, TokenType> reservedMap = {
        {"main",     Main},
        {"const",    ConstToken},
        {"int",      Int},
        {"void",     Void},
        {"break",    Break},
        {"continue", Continue},
        {"if",       If},
        {"else",     Else},
        {"while",    While},
//        {"getint",   GetInt},
//        {"putint",   PutInt},
        {"return",   Return}
};

// separatorMap
const map<string, TokenType> separatorMap = {{"!",  Not},
                                             {"&&", And},
                                             {"||", Or},
                                             {"+",  Add},
                                             {"-",  Minus},
                                             {"*",  Mul},
                                             {"/",  Div},
                                             {"%",  Mod},
                                             {"<",  Lt},
                                             {"<=", Leq},
                                             {">",  Gt},
                                             {">=", Geq},
                                             {"==", Eq},
                                             {"!=", Neq},
                                             {"=",  Assign},
                                             {";",  Semicolon},
                                             {",",  Comma},
                                             {"(",  LParen},
                                             {")",  RParen},
                                             {"[",  LBracket},
                                             {"]",  RBracket},
                                             {"{",  LBrace},
                                             {"}",  RBrace}
};

const string syntaxTypeStr[SyntaxTypeNum] = {
        "CompUnit", "ConstDecl", "BType", "ConstDef", "VarDecl", "VarDef", "FuncDef", "MainFuncDef", "FuncType",
        "FuncFParams", "FuncFParam", "Block", "Exp", "Cond", "LVal", "Number", "UnaryOp", "FuncRParams",
        "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp", "ConstExp",
        "Decl", "ConstInitVal", "InitVal", "BlockItem", "Stmt", "PrimaryExp", "UnaryExp"
};


const map<SyntaxType, set<int>> firstSetMap = {
        {ConstDecl, {  ConstToken}},
        {VarDecl, {    Int}},
        {ConstExp, {   LParen,     Ident,  IntConst, Add,   Minus, Not}},
        {Exp, {        LParen,     Ident,  IntConst, Add,   Minus, Not}},  // aka First(PriExp) + First(UnaryOp) + {Ident}
        {Decl, {       ConstToken, Int}},
        {Stmt, {       Ident,      LParen, Add,      Minus, Not,   IntConst, Semicolon, If, While, Break, Return, LBrace}},
        {LVal, {       Ident}},
        {Block, {      LBrace}},
        {Number, {     IntConst}},
        {PrimaryExp, { LParen,     Ident,  IntConst}},
        {UnaryOp, {    Add,        Minus,  Not}},
        {FuncRParams, {LParen,     Ident,  IntConst, Add,   Minus, Not}},  // aka First(Exp)
        {FuncFParams, {Int}},  // aka First(BType)
};
