#ifndef PROJECT_PARSE_H
#define PROJECT_PARSE_H
#include "SymbolTable.h"

void compUnit();
void constDecl();
void bType(IdentType& type);
void constDef(IdentType type);
void varDecl();
void varDef(IdentType type);
void funcDef();
void mainFuncDef();
void funcType(IdentType&);
void funcFParams(vector<symbolTableNode>& funcFPsListLinkVersion);
void funcFParam(vector<symbolTableNode>& funcFPsListLinkVersion);
void block(vector<symbolTableNode> funcFPsLinkVersion = vector<symbolTableNode>(), ScopeKind kind = NormalScope, bool isVoidFunc = false, string funcName = "");
pair<IdentType, int> exp(string& result);
void cond(string& trueLabel, string& falseLabel);
pair<IdentType, int> lVal(string& result, vector<string>& dimIndex, bool isAssigned = false);
pair<IdentType, int> number(string& result);
TokenType unaryOp();
void funcRParams(vector<pair<IdentType, int>>& argumentsType, vector<string>& arguments);
pair<IdentType, int> mulExp(string& result, bool& isI1);
pair<IdentType, int> addExp(string& result, bool& isI1);


void relExp(string& cond, bool& isI1);
void eqExp(string& cond);
void lAndExp(string& trueLabel, string& falseLabel);
void lOrExp(string& trueLabel, string& falseLabel);

void constExp(string& result);
void decl();
void constInitVal(vector<vector<string>> &constInitValues, vector<string>& constInitValuePerBrace, int curLBraceNum, int dim);
void initVal(vector<vector<string>>& initValues, vector<string>& initValuePerBrace, int curLBraceNum, int dim);
void blockItem();
void stmt(string &label);
pair<IdentType, int> primaryExp(string& result);
pair<IdentType, int> unaryExp(string& result, bool& isI1);
bool assignStmt();

void addLibFun();


#endif //PROJECT_PARSE_H
