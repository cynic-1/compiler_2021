#ifndef MINISYSY_IR_H
#define MINISYSY_IR_H

#include "type.h"
#include "SymbolTable.h"

#define REG_PREFIX "%x"

class IR {
private:
    static int index;
    static string irCodes;
    static string tempCodes;
    static bool exitBlockButNotLabelYet;


public:
    static bool exitFuncButNoNewDefYet;

    static void addFuncDef(IdentType returnType, const string& funcName, vector<symbolTableNode>& params);
    static void addFuncParams(vector<symbolTableNode>& params);
    static void addFuncParam(symbolTableNode & param);

    static void addCall(const string& result, const string& funcName, vector<string>& arguments, const vector<pair<IdentType, int>>& argumentsType);
    static void addCall(const string& funcName, vector<string>& arguments, const vector<pair<IdentType, int>>& argumentsType);
    static void addArguments(vector<string>& arguments, const vector<pair<IdentType, int>>& argumentsType);
    static void addArgument(string & arguments, bool isArray);

    // + - * / %
    static void addArithmetic(string& result, TokenType op, string op1, string op2, bool isOp1I1 = false, bool isOp2I1 = false);

    static void addAlloca(const string &result, const vector<int> &axis);
    static void addLoad(const string& result, const string& pointer);
    // There is a bug about reference, so remove the reference
    static void addStore(const string& value, const string pointer);
    static void addGlobal(const string& name, const string &value);
    static void addGlobalArray(const string& name, bool isConst, const vector<int> &axis, const vector<vector<string>>& initValues);
    static void addGlobalArrayIter(string& code, const vector<int> &axis, int idxAxis, const vector<vector<string>>& initValues, int& idxInitVal);

    static void addIcmp(string& result, TokenType cond, const string& op1, const string& op2, bool isOp1I1, bool isOp2I1);
    static void addBr(const string& cond, const string& trueLabel, const string& falseLabel);
    static void addBr(const string& dest);
    static void addRet(const string& value);
    static void addRet();
    static void addLabel(const string& label);
    static void addZextTo(const string& result, const string& value);
    static void addZextToBool(const string& result, const string& value);
//    static void addAnd(const string& result, const string &op1, const string& op2);
//    static void addOr(const string& result, const string &op1, const string& op2);

    static void addLParen();
    static void addRParen();
    static void addLBrace();
    static void addRBrace();
    static void addCommaSpace();
    static void addNewLine();


    static void addMemset(const string& pointerName, int val, int bytes);
    static void addGetElePtr(const string &pointerName, const string &basePointerName, const vector<int>& axis, const string& offset= "");
    static void addGetElePtrLater(const string &pointerName, const string &basePointerName, const vector<int>& axis, const string& offset= "");


    static string generateRegister();
    static void resetIndex();
    static void write2File();
    static void addFuncDecl();

    static string getArrayTypeString(const vector<int> &axis, int idx = 0);
};

#endif //MINISYSY_IR_H
