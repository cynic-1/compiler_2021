#ifndef PROJECT_LINKSYMTABLE_H
#define PROJECT_LINKSYMTABLE_H

#include "type.h"

#define ROOT -1

/**
 * 符号表节点类
 */
class symbolTableNode {
public:
    string name;
    string regName;
    string pointerName;
    IdentKind kind;
    IdentType type;
    int size;   // funcSize is this func's scope's localAddr when exit from it.
    int addr;
    // added to differ the global item
    int scopeIdx;

    // belongs to var or const
    int dimension;
    // both of them can be garbage, depends on the dimension and kind
    vector<int> axis;
    vector<int> offsets;

    // belongs to Const
    vector<int> constValues;

    // belongs to function
    vector<pair<IdentType, int>> paramsTypeDimList;
    vector<string> paramsNames;
    int funcScopeIdx;

    symbolTableNode() = default;
    // constructor for param
    // symbolTableNode(string name, IdentType type, int dimension, int lenOfAxis1, int lenOfAxis2);

};

/**
 * 类数组实现的链表
 */
class Scope {
public:
    vector<symbolTableNode> allSymbols;
    ScopeKind scopeKind;

    string scopeHeadLabel;
    string scopeTailLabel;

    bool isVoidFunc;
    bool hasReturned;

    // for FuncScope
    string funcName;
    map<int, vector<int>> fourByteConstArray_AddrValuesMap;

    int parentScopeIndex;
    int selfIndex;
    // localAddr is updated not only by new Item, but also when Exit from it's child scope.
    int localAddr;  // for the initialize of Item's localAddr, remember to update when exit an scope

    symbolTableNode* lookupVarSym(string& name);
};

/**
 * 符号表类，二维vector实现的链表
 */
class SymbolTable {
public:
    static vector<Scope> allScopes;
    static int nextIndex;
    // set the const array in global
    // update at addVarSymbol, addTempSymbol and addConstSymbol\
    // important warning: must keep it synchronized with scope 0's localAddr!!!
    static int globalAddr;

    static void addFuncSymbol(int curScope, string& funcName, IdentType returnType, vector<pair<IdentType, int>>& paramsTypeDimList, vector<string>& paramsNames);
    static void addConstSymbol(int curScope, string& constName, IdentType constType, vector<int>& axis, vector<int>& constValues);
    static void addVarSymbol(int curScope, const string& name, const string& pointerName, IdentType varType, const vector<int>& axis);
    static void addParamSymbol(int curScope, string& name, IdentType varType, const vector<int>& axis);
    static void addParamSymbol(int curScope, symbolTableNode& param);
    static void addTempSymbol(int curScope, string& name, IdentType tempType);

    static symbolTableNode* findFuncSymbol(const string& funcName, int curScope = 0);
    static symbolTableNode* findVarSymbol(int curScope, string& varName);
    static Scope* findScope(int scopeIndex);

    // for const refill
    static Scope* findFirstFuncScope(int curScope);

    static int addScope(int parentIndex, ScopeKind kind, bool isVoidFunc = false, string funcName = "", string headLabel = "", string tailLabel = "");
    static int exitScope(int curScopeIndex);

    static symbolTableNode createParamItem(const string& name, const string &regOrPointerName, IdentType varType, const vector<int>& axis);

    // for errorCheck
    static symbolTableNode* findVarSymbol4ErrorCheck(int curScope, string& varName);
    static Scope* findFirstLoopScope(int curScope);

};


#endif //PROJECT_LINKSYMTABLE_H
