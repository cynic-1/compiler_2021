#include "type.h"
#include "lex.h"
#include "parse.h"
#include "globalData.h"
#include "error.h"
#include "utils.h"
#include "IR.h"


bool isPreCheck = false;
TokenContext lastToken;

int curScopeIndex = NON_PARENT;


string getOffset(const vector<string>& calledAxis, const vector<int> & offsets) {
    if (calledAxis.empty()) return "0";
    string offset = IR::generateRegister();
    IR::addArithmetic(offset, Add, "0", "0");
    for (int i = 0; i < calledAxis.size(); ++i) {
        string result = IR::generateRegister();
        IR::addArithmetic(result, Mul, calledAxis[i], to_string(offsets[i]));
        string temp = IR::generateRegister();
        IR::addArithmetic(temp, Add, offset, result);
        offset = temp;
    }
    return offset;
}

int getOffset(const vector<int>& calledAxis, const vector<int> & offsets) {
    if (calledAxis.empty()) {
        return 0;
    }
    int offset = 0;
    for (int i = 0; i < calledAxis.size(); ++i) {
        offset += calledAxis[i] * offsets[i];
    }
    offset += calledAxis.back();
    return offset;
}

void tryReplaceConst(string &name, const vector<string>& calledAxis) {
    if (isValue(name)) {
        return;
    }
    symbolTableNode *item = SymbolTable::findVarSymbol(curScopeIndex, name);
    if (item == nullptr) {
        return;
    }
    if (item->kind == Const) {
        if (calledAxis.size() == item->dimension) {
            vector<int> intCalledAxis;
            for (auto iter:calledAxis) {
                if (!isValue(iter)) return;
                intCalledAxis.emplace_back(stoi(iter));
            }
            name = to_string(item->constValues.at(getOffset(intCalledAxis, item->offsets)));
        } else exit(-1);
    }
}

vector<int> getOffsetsVector(vector<int> &axis);


// called only when the curToken is Ident
/**
 * 往后看确定次数个不能决定分支，用预检查机制
 * stmt -> assignStmt -> lVal '=' exp ';'
 * stmt -> [exp] ';' -> lVal ';'
 * 所以通过'='判断
 * @return true if it should jump to assignStmt
 */
bool assignStmtPreCheck() {
    TokenContext lastTmp = lastToken;
    TokenContext curTmp = curTokenContext;
    isPreCheck = true;
    peekPos = 0;
    bool isAssign = assignStmt();
    isPreCheck = false;
    lastToken = lastTmp;
    curTokenContext = curTmp;
    return isAssign;
}

/**
 * 根据类型获取token，如果不匹配则报错
 * @param type expected token type
 * @return token context
 */
TokenContext getToken(int type) {
    ErrorCheckUnit::checkTokenUnmatched(curTokenContext.tokenType, type);

    TokenContext ret = curTokenContext;
    lastToken = curTokenContext;
    getNextToken();
    return ret;
}

/**
 * compUnit -> [compUnit] (Decl | FuncDef)
 */
void compUnit() {
    // create the root scope: index=0, aka global
    curScopeIndex = SymbolTable::addScope(NON_PARENT, NormalScope);
    addLibFun();
    IR::addFuncDecl();

    while (isDeclBranch()) {
        decl();
    }
    while (isFuncDefBranch()) {
        funcDef();
    }
    // 因为不能调用声明在后面的函数，所以main函数后面的函数对程序结果没有影响，不用管了。yeah！
    mainFuncDef();

    curScopeIndex = SymbolTable::exitScope(curScopeIndex);
}

void constDecl() {
    getToken(ConstToken);
    IdentType type;
    bType(type);

    constDef(type);
    while (curTokenContext.tokenType == Comma) {
        getToken(Comma);
        // didn't clean the shared constEntry, because all attrib will be reset.
        constDef(type);
    }
    getToken(Semicolon);
}

void bType(IdentType &type) {
    getToken(Int);
    type = IntType;
}

/**
 * ConstDef     -> Ident { '[' ConstExp ']' } '=' ConstInitVal
 * @param type
 */
void constDef(IdentType type) {
    TokenContext ident = getToken(Ident);
    int axisNum = 0;
    vector<int> axis;
    while (curTokenContext.tokenType == LBracket) {
        getToken(LBracket);

        string len = "0";
        constExp(len);
        axis.emplace_back(stoi(len));

        getToken(RBracket);
    }
    getToken(Assign);

    vector<vector<string>> initValues;
    vector<string> ipb;
    constInitVal(initValues, ipb, 0, axis.size());

    /*************************** new symbolTable *************************/
    ErrorCheckUnit::checkDupDef(curScopeIndex, ident.token, false);

    if (axis.size() > 0) {
        vector<int> offsets = getOffsetsVector(axis);
        vector<int> constValues(offsets[0]);
        int idxConst = 0;
        for (int i = 0; i < initValues.size(); ++i) {
            vector<string> cipb = initValues[i];
            idxConst = i * axis.back();
            for (int j = 0; j < cipb.size(); ++j) {
                constValues[idxConst+j] = stoi(cipb[j]);
            }
        }
        SymbolTable::addConstSymbol(curScopeIndex, ident.token, type, axis, constValues);
        // 常量中除了全局常量，都不输出IR，只填表
        if (curScopeIndex == 0) { // global array
            symbolTableNode* sNode = SymbolTable::findVarSymbol(curScopeIndex, ident.token);
            sNode->offsets = offsets;
            IR::addGlobalArray(ident.token, true, axis, initValues);
        }
    } else {
        vector<int> constValues(1);
        constValues[0] = stoi(ipb.front());
        SymbolTable::addConstSymbol(curScopeIndex, ident.token, type, axis, constValues);
    }

}

vector<int> getOffsetsVector(vector<int> &axis) {
    vector<int> offsets(axis.size());
    offsets.back() = 1;
    for (int i = axis.size()-1; i > 0; --i) {
        offsets[i-1] = offsets[i] * axis[i];
    }
    return offsets;
}

void varDecl() {
    IdentType type;
    bType(type);

    varDef(type);
    while (curTokenContext.tokenType == Comma) {
        getToken(Comma);
        varDef(type);
    }
    getToken(Semicolon);
}

/**
 * varDef       -> ident { '[' constExp ']' }
                | ident { '[' constExp ']' } '=' initVal
 *  全局变/常量声明中指定的初值表达式必须是常量表达式；
    局部变量可以和全局变量同名，在局部变量作用域内，局部变量隐藏了同名全局变量的定义；
    未显式初始化的局部变量，其值是不确定的；而未显式初始化的全局变量，其值均被初始化为 0

    全局 int 类型变量/常量的初值必须是编译时可求值的常量表达式。
    局部 int 类型常量的初值必须是编译时可求值的表达式。（和 C 语言略有不同）
 * @param type must be int
 */
void varDef(IdentType type) {
    TokenContext ident = getToken(Ident);
    vector<int> axis;

    while (curTokenContext.tokenType == LBracket) {
        getToken(LBracket);

        string value;
        constExp(value);
        axis.emplace_back(stoi(value));

        getToken(RBracket);
    }

    ErrorCheckUnit::checkDupDef(curScopeIndex, ident.token, false);
    if (curScopeIndex > 0) { // local aka non-global
        string pointerName = IR::generateRegister();
        IR::addAlloca(pointerName, axis);
        SymbolTable::addVarSymbol(curScopeIndex, ident.token, pointerName, type, axis);
        if (curTokenContext.tokenType == Assign) {
            getToken(Assign);
            vector<vector<string>> initValues;
            vector<string> ipb;
            initVal(initValues, ipb, 0, axis.size());

            // assign with the init values, and iniValues.size() is the count of the var
            if (initValues.empty()) {
                IR::addStore(ipb[0], pointerName);
            } else {
                // array
                vector<int> offsets = getOffsetsVector(axis);
                int bytes = offsets.front() * axis.front() * 4;

                // 拿数组的i32*的指针
                string pointer = IR::generateRegister();
                IR::addGetElePtr(pointer, pointerName, axis);

                // 更新pointerName为i32*型的，添加offsets
                symbolTableNode* sNode = SymbolTable::findVarSymbol(curScopeIndex, ident.token);
                sNode->pointerName = pointer;
                sNode->offsets = offsets;

                // 初始化第一步： memset
                IR::addMemset(pointer, 0, bytes);

                // 第二步：根据初始值补全
                int baseOffset = 0;
                if (offsets.size() >= 2) baseOffset = offsets[offsets.size()-2];
                for (int i = 0; i < initValues.size(); ++i) {
                    vector<string> vs = initValues[i];
                    for (int j = 0; j < vs.size(); ++j) {
                        string curPointer = IR::generateRegister();
                        int curOffset =  baseOffset * i + j;
                        vector<int> temp;
                        IR::addGetElePtr(curPointer, pointer, temp, to_string(curOffset));
                        IR::addStore(vs[j], curPointer);
                    }
                }
            }
        } else {
            if (!axis.empty()) {
                vector<int> offsets = getOffsetsVector(axis);
                int bytes = offsets.front() * axis.front() * 4;

                // 拿数组的i32*的指针
                string pointer = IR::generateRegister();
                IR::addGetElePtr(pointer, pointerName, axis);

                // 更新pointerName为i32*型的，添加offsets
                symbolTableNode* sNode = SymbolTable::findVarSymbol(curScopeIndex, ident.token);
                sNode->pointerName = pointer;
                sNode->offsets = offsets;
            }
        }
    } else { // Global
        string pointerName;
        SymbolTable::addVarSymbol(curScopeIndex, ident.token, pointerName, type, axis);
        symbolTableNode* sNode = SymbolTable::findVarSymbol(curScopeIndex, ident.token);
        if (curTokenContext.tokenType == Assign) {
            getToken(Assign);
            vector<vector<string>> initValues;
            vector<string> ipb;
            initVal(initValues, ipb, 0, axis.size());
            // assign with the init values, and iniValues.size() is the count of the var
            if (axis.empty()) {
                if (ipb.size() == 1) {   // normalVar init
                    if (!isValue(ipb[0])) {
                        exit(-1);
                    }
                    IR::addGlobal(sNode->name, ipb[0]);
                } else if (initValues.empty()) {
                    IR::addGlobal(sNode->name, "0");
                }
                sNode->pointerName = "@" + sNode->name;
            }
            else { // dimension > 0
                // TODO: global array declare
                vector<int> offsets = getOffsetsVector(axis);

                // 更新pointerName为i32*型的，添加offsets

                sNode->offsets = offsets;

                IR::addGlobalArray(ident.token, false, axis, initValues);
            }
        } else { // no-assign
            if (axis.empty()) {
                IR::addGlobal(ident.token, "0");
            } else {
                vector<vector<string>> initValues;
                IR::addGlobalArray(ident.token, false, axis, initValues);
            }
        }
    }
}

/**
 * funcDef      -> funcType ident '(' [funcFParams] ')' block
 */
void funcDef() {
    IdentType type;
    funcType(type);

    TokenContext ident = getToken(Ident);
    vector<symbolTableNode> funcFPsListLinkVersion;

    getToken(LParen);
    if (isFuncFPsAtFuncDef())
        funcFParams(funcFPsListLinkVersion);
    getToken(RParen);

    IR::addFuncDef(type, ident.token, funcFPsListLinkVersion);

    //------------------- new symbolTable --------------------
    vector<pair<IdentType, int>> paramsTypeDimList;
    vector<string> FParamsNames;
    for (const auto& iter:funcFPsListLinkVersion) {
        paramsTypeDimList.emplace_back(make_pair(iter.type, iter.dimension));
        FParamsNames.emplace_back(iter.regName);
    }

    ErrorCheckUnit::checkDupDef(curScopeIndex, ident.token, true);

    SymbolTable::addFuncSymbol(curScopeIndex, ident.token, type, paramsTypeDimList, FParamsNames);

    IR::addLBrace();

    block(funcFPsListLinkVersion, FuncScope, (type == VoidType), ident.token);

    IR::addRBrace();
}

void mainFuncDef() {
    getToken(Int);
    TokenContext ident = getToken(Main);
    getToken(LParen);
    getToken(RParen);

    vector<symbolTableNode> temp;
    IR::addFuncDef(IntType, "main", temp);


    /********************* new SymbolTable ************************/
    vector<pair<IdentType, int>> emptyDimType;
    vector<string> emptyNames;

    ErrorCheckUnit::checkDupDef(curScopeIndex, ident.token, true);

    SymbolTable::addFuncSymbol(curScopeIndex, ident.token, IntType, emptyDimType, emptyNames);


    IR::resetIndex();

    IR::addLBrace();

    block(vector<symbolTableNode>(), FuncScope, false, "main");

    IR::addRBrace();
}

/**
 * funcType     -> 'void' | 'int'
 * @param type
 */
void funcType(IdentType &type) {
    if (curTokenContext.tokenType == Void) {
        getToken(Void);
        type = VoidType;
    } else {
        getToken(Int);
        type = IntType;
    }
}

/**
 * funcFParams -> funcFParam { ',' funcFParam }
 * @param FPsLinkVersion 当前作用域符号表
 */
void funcFParams(vector<symbolTableNode> &FPsLinkVersion) {

    funcFParam(FPsLinkVersion);
    while (curTokenContext.tokenType == Comma) {
        getToken(Comma);
        funcFParam(FPsLinkVersion);
    }
}

/**
 * funcFParam  -> bType ident ['[' ']' { '[' exp ']' }]
 * @param FPsLinkVersion 当前作用域符号表
 */
void funcFParam(vector<symbolTableNode> &FPsLinkVersion) {
    IdentType varType;
    bType(varType);
    TokenContext ident = getToken(Ident);
    vector<int> axis;
    // array as param
    if (curTokenContext.tokenType == LBracket) {
        // e.g. int a[][3]， 第一维为空，不考虑
        getToken(LBracket);
        getToken(RBracket);
        axis.emplace_back(1);

        while (curTokenContext.tokenType == LBracket) {
            getToken(LBracket);

            string value;
            constExp(value);
            axis.emplace_back(stoi(value));

            getToken(RBracket);
        }
    }

    string regName = IR::generateRegister();
    symbolTableNode param = SymbolTable::createParamItem(ident.token, regName, varType, axis);
    if (!axis.empty()) {
        vector<int> offsets = getOffsetsVector(axis);

        param.offsets = offsets;
    }
    FPsLinkVersion.emplace_back(param);
}

/**
 * block -> '{' { BlockItem } '}'
 * @param funcFPsLinkVersion
 * @param kind in FuncScope, NormalScope, LoopScope
 * @param isVoidFunc true if void, false if int
 * @param funcName plain funcname
 */
void block(vector<symbolTableNode> funcFPsLinkVersion, ScopeKind kind, bool isVoidFunc,
           string funcName) {
    // lastToken must be ')'
    int funcNameLineNum = lastToken.lineNum;
    getToken(LBrace);

    curScopeIndex = SymbolTable::addScope(curScopeIndex, kind, isVoidFunc, std::move(funcName));

    // add FParams
    for (auto& iter:funcFPsLinkVersion) {
        ErrorCheckUnit::checkDupDef(curScopeIndex, iter.name, false);
        SymbolTable::addParamSymbol(curScopeIndex, iter);
    }

    while (curTokenContext.tokenType != RBrace) {
        blockItem();
    }
    getToken(RBrace);

    ErrorCheckUnit::checkNoReturn(curScopeIndex);
    Scope scope = SymbolTable::allScopes.at(curScopeIndex);
    if (scope.scopeKind == FuncScope && scope.isVoidFunc && !scope.hasReturned) {
        IR::addRet();
    }
    curScopeIndex = SymbolTable::exitScope(curScopeIndex);
}

/**
 * exp -> addExp
 * @param result
 * @return
 */
pair<IdentType, int> exp(string &result) {
    pair<IdentType, int> ret = addExp(result);
    return ret;
}

/**
 * cond -> lOrExp
 * @param trueLabel
 * @param falseLabel
 */
void cond(string &trueLabel, string &falseLabel) {
    lOrExp(trueLabel, falseLabel);
}

bool preCheckUndef(int curScope, string &name, bool isFunc) {
    symbolTableNode *item;
    if (isFunc) {
        item = SymbolTable::findFuncSymbol(name);
    } else {
        item = SymbolTable::findVarSymbol(curScope, name);
    }
    if (item == nullptr) return true;
    return false;
}

/**
 * lVal         -> ident {'[' exp ']'}
 * @param result
 * @param dimIdx
 * @param isAssigned
 * @return
 */
pair<IdentType, int> lVal(string &result, vector<string> &dimIdx, bool isAssigned) {
    // init value, only useful when varEntry == nullptr
    pair<IdentType, int> ret(IntType, 0);

    TokenContext ident = getToken(Ident);

    string lvalName = ident.token;

    // 防止预检查时乱走
    if(preCheckUndef(curScopeIndex, lvalName, false)) {
//        cout << "lvalName: " + lvalName << endl;
        return ret;
    }

    if (isAssigned) {
        ErrorCheckUnit::checkConstAssign(curScopeIndex, lvalName);
    }

    symbolTableNode *item = SymbolTable::findVarSymbol(curScopeIndex, lvalName);
    ret.first = item->type;
    ret.second = item->dimension;


    // must execute even if error already happened, because we need checkAndGet to push forward.
    vector<string> dimIndex;
    // string dimIndex[MAX_AXIS_NUM];
    while (curTokenContext.tokenType == LBracket) {
        getToken(LBracket);

        string tempResult;
        exp(tempResult);
        dimIndex.emplace_back(tempResult);
        getToken(RBracket);
        ret.second--;
    }
    string name = ident.token;
    symbolTableNode *identSym = SymbolTable::findVarSymbol(curScopeIndex, name);

    if (!isAssigned) {    // rvalue, get the value
        tryReplaceConst(name, dimIndex);
        symbolTableNode *identSym = SymbolTable::findVarSymbol(curScopeIndex, name);
        if (isValue(name)) {
            result = name;
            return ret;
        }
        if (identSym->dimension == 0) {
//            if (identSym->regName.empty()) {
//                string temp = IR::generateRegister();
//                identSym->regName = temp;
//                IR::addLoad(temp, identSym->pointerName);
//            }
//            result = identSym->regName;
            // 还是不做这种优化了，容易出现Instruction does not dominate all uses!的错误
            string temp = IR::generateRegister();
            IR::addLoad(temp, identSym->pointerName);
            return ret;
        } else {
            // array
            if (identSym->pointerName.empty()) {
                string newPointer = IR::generateRegister();
                IR::addGetElePtr(newPointer, "@" + identSym->name, identSym->axis);
                identSym->pointerName = newPointer;
            }
            /***********************到这里说明 tryReplaceConst 没有成功 用str版的getOffset ****************************/
            string offset = getOffset(dimIndex, identSym->offsets);
            string newPointer = IR::generateRegister();
            vector<int> temp;
            IR::addGetElePtr(newPointer, identSym->pointerName, temp, offset);
            if (identSym->dimension > dimIndex.size()) {
                result = newPointer;
            } else if (identSym->dimension == dimIndex.size()){
                result = IR::generateRegister();
                IR::addLoad(result, newPointer);
            } else {
                exit(-1);
            }
        }
    } else {
        // assigned, as left value
        // normalVar, result is its name
        if (identSym->dimension == 0) {
            result = name;
            return ret;
        } else {
            // lval is array and is going to be assigned,
            // so do nothing here, but return the dim info to assignStmt
            // to generate OP_STORE_ARRAY
            /****************************维数不匹配*****************************/
            // TODO: 不确定能不能对指针赋值，过点看看先
            if (identSym->dimension != dimIndex.size()) exit(-1);
            /*****************************************************************/
            result = name;
            dimIdx = dimIndex;
            return ret;
        }
    }
    return ret;
}

pair<IdentType, int> number(string &result) {
    TokenContext intcon = getToken(IntConst);
    pair<IdentType, int> ret(IntType, 0);
    result = intcon.token;
    return ret;
}

TokenType unaryOp() {
    TokenType ret;
    if (curTokenContext.tokenType == Add) {
        getToken(Add);
        ret = Add;
    } else if (curTokenContext.tokenType == Minus) {
        getToken(Minus);
        ret = Minus;
    } else {
        getToken(Not);
        ret = Not;
    }
//    printSyn(UnaryOp);
    return ret;
}

void funcRParams(vector<pair<IdentType, int>> &argumentsType, vector<string> &arguments) {
    // shared argument for all the RParams
    pair<IdentType, int> argument;
    string result;
    argument = exp(result);
    // when return, the argument is always considered as well-set.
    argumentsType.emplace_back(argument);
    arguments.emplace_back(result);


    while (curTokenContext.tokenType == Comma) {
        getToken(Comma);
        string tempResult;
        argument = exp(tempResult);
        argumentsType.emplace_back(argument);
        arguments.emplace_back(tempResult);
    }
}

/**
 * mulExp       -> unaryExp
                | mulExp ('*' | '/' | '%') unaryExp
 * @param result
 * @return
 */
pair<IdentType, int> mulExp(string &result) {

    string tempResult1;
    pair<IdentType, int> ret = unaryExp(tempResult1);
    result = tempResult1;

    TokenType op = static_cast<TokenType>(curTokenContext.tokenType);
    while (op == Mul || op == Div || op == Mod) {
        getNextToken();
        string tempResult2;
        unaryExp(tempResult2);

        vector<string> t;
        tryReplaceConst(tempResult1, t);
        tryReplaceConst(tempResult2, t);
        if (isValue(tempResult1) && isValue(tempResult2)) {
            switch (op) {
                case Mul:
                    tempResult1 = to_string(stoi(tempResult1) * stoi(tempResult2));
                    break;
                case Div:
                    tempResult1 = to_string(stoi(tempResult1) / stoi(tempResult2));
                    break;
                case Mod:
                    tempResult1 = to_string(stoi(tempResult1) % stoi(tempResult2));
                    break;
            }
            result = tempResult1;
        } else {
            string temp = IR::generateRegister();
            IR::addArithmetic(temp, op, tempResult1, tempResult2);
            SymbolTable::addTempSymbol(curScopeIndex, temp, IntType);
            result = tempResult1 = temp;
        }

        op = static_cast<TokenType>(curTokenContext.tokenType);
    }
    return ret;
}

/**
 * addExp       -> mulExp
                | addExp ('+' | '−') mulExp
 * @param result
 * @return
 */
pair<IdentType, int> addExp(string &result) {

    string tempResult1;
    pair<IdentType, int> ret = mulExp(tempResult1);
    result = tempResult1;

    TokenType op = static_cast<TokenType>(curTokenContext.tokenType);
    while (op == Add || op == Minus) {
        getNextToken();
        string tempResult2;
        mulExp(tempResult2);

        // const replace
        vector<string> t;
        tryReplaceConst(tempResult1, t);
        tryReplaceConst(tempResult2, t);
        // const calculate
        if (isValue(tempResult1) && isValue(tempResult2)) {
            switch (op) {
                case Add:
                    tempResult1 = to_string(stoi(tempResult1) + stoi(tempResult2));
                    break;
                case Minus:
                    tempResult1 = to_string(stoi(tempResult1) - stoi(tempResult2));
                    break;
            }
            result = tempResult1;
        } else {
            string temp = IR::generateRegister();
            IR::addArithmetic(temp, op, tempResult1, tempResult2);
            SymbolTable::addTempSymbol(curScopeIndex, temp, IntType);
            result = tempResult1 = temp;
        }

        op = static_cast<TokenType>(curTokenContext.tokenType);

    }
    return ret;
}

/**
 * RelExp       -> AddExp
                | RelExp ('<' | '>' | '<=' | '>=') AddExp
 * @param result Make sure its type is i32
 */
void relExp(string &result) {
    string tempResult1;
    addExp(tempResult1);
    auto op = static_cast<TokenType>(curTokenContext.tokenType);
    bool t1IsBool = false;
    while (op == Lt || op == Leq || op == Gt || op == Geq) {
        getNextToken();
        string tempResult2;
        addExp(tempResult2);

        // 处理 a > b > c的情况
        if (t1IsBool){
            string temp = IR::generateRegister();
            IR::addZextTo(temp, tempResult1);
            tempResult1 = temp;
            t1IsBool = false;
        }

        if (isValue(tempResult1) && isValue(tempResult2)) {
            switch (op) {
                case Lt:
                    tempResult1 = to_string(stoi(tempResult1) < stoi(tempResult2));
                    break;
                case Leq:
                    tempResult1 = to_string(stoi(tempResult1) <= stoi(tempResult2));
                    break;
                case Gt:
                    tempResult1 = to_string(stoi(tempResult1) > stoi(tempResult2));
                    break;
                case Geq:
                    tempResult1 = to_string(stoi(tempResult1) >= stoi(tempResult2));
                    break;
            }
        } else {
            string temp = IR::generateRegister();
            IR::addIcmp(temp, op, tempResult1, tempResult2);
            SymbolTable::addTempSymbol(curScopeIndex, temp, IntType);
            tempResult1 = temp;
            t1IsBool = true;
        }
        op = static_cast<TokenType>(curTokenContext.tokenType);
    }

    result = tempResult1;
}

/**
 * eqExp -> relExp
          | eqExp ('==' | '!=') relExp
 * ------------kill left recursion-------------
 * eqExp -> relExp {('==' | '!=') relExp}*
 * @param cond make sure its type is i1
 */
void eqExp(string &cond) {
    string tempResult1;
    relExp(tempResult1);
    auto op = static_cast<TokenType>(curTokenContext.tokenType);
    bool t1IsBool = false;
    while (op == Eq || op == Neq) {
        getNextToken();
        string tempResult2;
        relExp(tempResult2);

        if (t1IsBool) {
            string temp = IR::generateRegister();
            IR::addZextTo(temp, tempResult1);
            tempResult1 = temp;
            t1IsBool = false;
        }

        switch (op) {
            case Eq:
                if (isValue(tempResult1) && isValue(tempResult2)) {
                    // 数字之间比较就直接比了
                    tempResult1 = to_string(stoi(tempResult1) == stoi(tempResult2));
                } else {
                    // 不全为数字才需要生成代码
                    string temp = IR::generateRegister();
                    IR::addIcmp(temp, op, tempResult1, tempResult2);
                    SymbolTable::addTempSymbol(curScopeIndex, temp, IntType);
                    tempResult1 = temp;
                }
                break;
            case Neq:
                if (isValue(tempResult1) && isValue(tempResult2)) {
                    tempResult1 = to_string(stoi(tempResult1) != stoi(tempResult2));
                } else {
//                    tempResult1 = addIR(OP_NEQ, tempResult1, tempResult2, AUTO_TMP);
                    string temp = IR::generateRegister();
                    IR::addIcmp(temp, op, tempResult1, tempResult2);
                    SymbolTable::addTempSymbol(curScopeIndex, temp, IntType);
                    tempResult1 = temp;
                }
                break;
        }
        op = static_cast<TokenType>(curTokenContext.tokenType);
    }
    cond = tempResult1;
}

/**
 * lAndExp      -> eqExp
                | lAndExp '&&' eqExp
 * ------------kill left recursion-------------
 * lAndExp -> eqExp {(&&) eqExp}*
 * @param trueLabel Synthesis Attribute
 * @param falseLabel Synthesis Attribute
 */
void lAndExp(string &trueLabel, string &falseLabel) {

    // eqExp won't appear "||", so maybe it doesn't need trueLabel?
    // always continue the following code if true without jumping, only jump to falseLabel when false.
    string newTrueLabel;
    string cond;
    eqExp(cond);
    bool flag = false;

    while (curTokenContext.tokenType == And) {
        if (!flag) {
            flag = true;
        }
        getToken(And);
        newTrueLabel = IR::generateRegister();
        IR::addBr(cond, newTrueLabel, falseLabel);
        IR::addLabel(newTrueLabel);
        eqExp(cond);
    }

    // once execute to here, means all the eqExp is true, then jump to trueLabel as overall result of this lAndExp.

    IR::addBr(cond, trueLabel, falseLabel);
}

/**
 * lOrExp       -> lAndExp
                | lOrExp '||' lAndExp
 * ------------kill left recursion-------------
 * lOrExp -> lAndExp {('||') lAndExp}*
 * @param trueLabel Synthesis Attribute
 * @param falseLabel Synthesis Attribute
 */
void lOrExp(string &trueLabel, string &falseLabel) {

    string lAndFalseLabel = IR::generateRegister();
    lAndExp(trueLabel, lAndFalseLabel);

    while (curTokenContext.tokenType == Or) {
        getToken(Or);
        IR::addLabel(lAndFalseLabel);
        lAndFalseLabel = IR::generateRegister();
        lAndExp(trueLabel, lAndFalseLabel);
    }

    // if still didn't jump to the true station, means every lAndExp is false, then the whole lOr is false, goto false location.
    IR::addLabel(lAndFalseLabel);
    IR::addBr(falseLabel);
}

/**
 * constExp -> addExp
 * 在语义上额外约束这里的 addExp 必须是一个可以在编译期求出值的常量
 * @param result Synthesis Attribute
 */
void constExp(string &result) {
    addExp(result);
}

/*----------------------------------------------
 * branch picking part
 * ---------------------------------------------
 */

/**
 * decl -> constDecl | varDecl
 */
void decl() {
    switch (curTokenContext.tokenType) {
        case ConstToken: {
            constDecl();
            break;
        }
        case Int: {
            varDecl();
            break;
        }
        // error
        default: {
            exit(-1);
        }
    }
}

/**
 * constInitVal -> constExp
                | '{' [ constInitVal { ',' constInitVal } ] '}'
 * @param constValues Synthesis Attribute
 */
void constInitVal(vector<vector<string>> &constInitValues, vector<string>& constInitValuePerBrace, int curLBraceNum, int dim) {
    switch (curTokenContext.tokenType) {
        case LBrace: {
            getToken(LBrace);
            curLBraceNum++;
            vector<string> ipb;
            if (curTokenContext.tokenType != RBrace) {
                constInitVal(constInitValues, ipb, curLBraceNum, dim);
                while (curTokenContext.tokenType == Comma) {
                    getToken(Comma);
                    constInitVal(constInitValues, ipb, curLBraceNum, dim);
                }
            }
            getToken(RBrace);
            curLBraceNum--;
            if (curLBraceNum == dim - 1) constInitValues.emplace_back(ipb);
            break;
        }
        case LParen:
        case Ident:
        case IntConst:
        case Add:
        case Minus:
        case Not: {
            string value;
            constExp(value);
            constInitValuePerBrace.emplace_back(value);
            break;
        }
        // error
        default: {
            std::cout<<"oh!no!\n";
            exit(-1);
        }
    }
}

/**
 * initVal      -> exp
                | '{' [ initVal { ',' initVal } ] '}'
 *  全局 int 类型变量/常量的初值必须是编译时可求值的常量表达式。
    局部 int 类型常量的初值必须是编译时可求值的表达式。（和 C 语言略有不同）
 * @param initValues
 */
void initVal(vector<vector<string>> &initValues, vector<string>& initValuePerBrace, int curLBraceNum, int dim) {
    if (isExpBranch()) {
        string result;
        exp(result);
        initValuePerBrace.emplace_back(result);
    } else {
        getToken(LBrace);
        curLBraceNum++;
        vector<string> ipb;
        if (curTokenContext.tokenType != RBrace) {
            initVal(initValues, ipb, curLBraceNum, dim);
            while (curTokenContext.tokenType == Comma) {
                getToken(Comma);
                initVal(initValues, ipb, curLBraceNum, dim);
            }
        }
        getToken(RBrace);
        curLBraceNum--;
        if (curLBraceNum == dim - 1) initValues.emplace_back(ipb);
    }
}

void blockItem() {
    if (isDeclOrStmt()) {
        decl();
    } else {
        string label;
        stmt(label);
        if (!label.empty()) {
            IR::addLabel(label);
        }
    }
}

/**
 * ifStmt -> 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
 */
void ifStmt(string & label) {
    getToken(If);
    getToken(LParen);

    // always make sure the label have and only have tagged once if generated and used in jump.

    string trueLabel = IR::generateRegister();
    string falseLabel = IR::generateRegister();

    cond(trueLabel, falseLabel);
    getToken(RParen);

    // new scope
    curScopeIndex = SymbolTable::addScope(curScopeIndex, NormalScope);

    IR::addLabel(trueLabel);

    string l;
    stmt(l);
    if (!l.empty()) {
        IR::addLabel(l);
    }

    // 跳转到下一个基本块，没有标签，咋搞？从上一层拿
    label = IR::generateRegister();
    IR::addBr(label);

    curScopeIndex = SymbolTable::exitScope(curScopeIndex);



    if (curTokenContext.tokenType == Else) {
        getToken(Else);

        IR::addLabel(falseLabel);

        curScopeIndex = SymbolTable::addScope(curScopeIndex, NormalScope);

        string l;
        stmt(l);
        if (!l.empty()) {
            IR::addLabel(l);
        }

        curScopeIndex = SymbolTable::exitScope(curScopeIndex);

        IR::addBr(label);

    } else {
        // don't have else part, tag the false label plainly.
        IR::addLabel(falseLabel);
        IR::addBr(label);
    }
}

/**
 * whileStmt -> 'while' /condLabel/ '(' Cond ')' /trueLabel/ Stmt /falseLabel/
 */
void whileStmt(string &condLabel, string &falseLabel) {
    getToken(While);
    getToken(LParen);

    string trueLabel = IR::generateRegister();
    falseLabel = IR::generateRegister();

    IR::addLabel(condLabel);

    cond(trueLabel, falseLabel);

    getToken(RParen);

    curScopeIndex = SymbolTable::addScope(curScopeIndex, LoopScope, false, "", condLabel, falseLabel);

    IR::addLabel(trueLabel);

    string l;
    stmt(l);

    if (!l.empty()) {
        IR::addLabel(l);
    }

    IR::addBr(condLabel);

    //-------------------- new symtable -----------------
    curScopeIndex = SymbolTable::exitScope(curScopeIndex);
}

/**
 * breakStmt -> 'break' ';'
 */
void breakStmt() {
    TokenContext breakToken = getToken(Break);
    getToken(Semicolon);

    ErrorCheckUnit::checkOutsideLoop(curScopeIndex);

    string tailLabel = SymbolTable::findFirstLoopScope(curScopeIndex)->scopeTailLabel;
    IR::addBr(tailLabel);
}

/**
 * continueStmt -> 'continue' ';'
 */
void continueStmt() {
    TokenContext continueToken = getToken(Continue);
    getToken(Semicolon);

    ErrorCheckUnit::checkOutsideLoop(curScopeIndex);

    string headLabel = SymbolTable::findFirstLoopScope(curScopeIndex)->scopeHeadLabel;
    IR::addBr(headLabel);
}

/**
 * returnStmt -> 'return' [Exp] ';'
 */
void returnStmt() {
    getToken(Return);
    string retResult = "0";
    if (isExpAtReturnStmt()) {
        ErrorCheckUnit::checkVoidReturn(curScopeIndex);
        exp(retResult);
    }
    getToken(Semicolon);

    SymbolTable::findScope(curScopeIndex)->hasReturned = true;

    string funcName = SymbolTable::findFirstFuncScope(curScopeIndex)->funcName;
    IR::addRet(retResult);
}

/**
 * stmt -> assignStmt -> lVal '=' exp ';'
 * stmt -> [exp] ';' -> lVal ';'
 * 所以通过'='判断
 * @return true if it should jump to assignStmt
 */
bool assignStmt() {
    // pass the isAssigned = true to trigger the ConstAssign error check
    // prepare for the target to be assigned
    string target;
    vector<string> dimIdx;
    lVal(target, dimIdx, true); // must successfully parsed as lVal when preChecking

    if (isPreCheck) {
        // can judge by '=' when preChecking
        if (curTokenContext.tokenType == Assign) {
            return true;
        } else {
            return false;
        }
    }
    getToken(Assign);
    string value;
    exp(value);
    getToken(Semicolon);
    symbolTableNode *targetSym = SymbolTable::findVarSymbol(curScopeIndex, target);

    // !!!!!!!!!! important! !!!!!!!!!!!!!!!!!!
    if (targetSym != nullptr) {
        if (targetSym->dimension == 0) {
                IR::addStore(value, targetSym->pointerName);
                targetSym->regName.clear();
                // 这里还是不能做这样的优化，因为由于if语句，该指令有可能未执行，但是后面又用到了该regName，出错。
//                if (!isValue(value)) {
//                    targetSym->regName = value;
//                }
        }
            // TODO: check array implementation
        else {
            string offset = getOffset(dimIdx, targetSym->offsets);
            string newPointer = IR::generateRegister();
            vector<int> temp;
            IR::addGetElePtr(newPointer, targetSym->pointerName, temp, offset);
            IR::addStore(value, newPointer);
        }
    }
    return true;
}

/**
 * Stmt        -> assignStmt
                | [Exp] ';'
                | Block
                | ifStmt
                | whileStmt
                | breakStmt
                | continueStmt
                | returnStmt
 */
void stmt(string &label) {
    switch (curTokenContext.tokenType) {
        case If: {
            ifStmt(label);
            break;
        }
        case While: {
            string condLabel = IR::generateRegister();
            IR::addBr(condLabel);
            whileStmt(condLabel, label);
            break;
        }
        case Break: {
            breakStmt();
            break;
        }
        case Continue: {
            continueStmt();
            break;
        }
        case Return: {
            returnStmt();
            break;
        }
        case LBrace: {
            block();
            break;
        }
        case Ident: {
            if (assignStmtPreCheck()) {
                assignStmt();
                break;
            }
        }
        default: {
            if (curTokenContext.tokenType != Semicolon) {
                string useless;
                exp(useless);
            }
            getToken(Semicolon);
        }
    }
}

/**
 * PrimaryExp   -> '(' Exp ')' | LVal | Number
 * @param result 输出值
 * @return
 */
pair<IdentType, int> primaryExp(string &result) {
    pair<IdentType, int> ret;
    if (curTokenContext.tokenType == LParen) {
        getToken(LParen);
        ret = exp(result);
        getToken(RParen);
    } else if (isLValAtPrimaryExp()) {
        vector<string> dimIdx;
        ret = lVal(result, dimIdx);
    } else {
        ret = number(result);
    }
    return ret;
}

/**
 * unaryExp     -> primaryExp
                | ident '(' [funcRParams] ')'
                | unaryOp unaryExp
 * @param result 输出值
 * @return 类型和数组维数的对
 */
pair<IdentType, int> unaryExp(string &result) {
    pair<IdentType, int> ret;
    if (isPrimaryExpAtUnExp()) {
        ret = primaryExp(result);
    } else if (curTokenContext.tokenType == Ident) {

        TokenContext ident = getToken(Ident);
        string funcName = ident.token;

        ErrorCheckUnit::checkUndef(curScopeIndex, funcName, true);
        getToken(LParen);

        vector<pair<IdentType, int>> argumentsType;
        vector<string> arguments;
        if (isFuncRPsAtUnExp()) {
            funcRParams(argumentsType, arguments);
        }

        ErrorCheckUnit::checkParamsNum(funcName, argumentsType.size());

        ErrorCheckUnit::checkParamsType(funcName, argumentsType);

        getToken(RParen);

        symbolTableNode *funcSym = SymbolTable::findFuncSymbol(funcName);
        ret.first = funcSym->type;
        ret.second = 0;

        for (int i = 0; i < arguments.size(); i++) {
            string RParam = arguments.at(i);
            string FParam = funcSym->paramsNames.at(i);
        }

        if (funcSym->type == IntType) {
            string temp = IR::generateRegister();
            IR::addCall(temp, funcName, arguments, argumentsType);
            result = temp;
        }
        else {
            IR::addCall(funcName, arguments, argumentsType);
        }
    } else {
        TokenType op = unaryOp();
        string tempResult;
        ret = unaryExp(tempResult);

        vector<string> t;
        tryReplaceConst(tempResult, t);

        if (isValue(tempResult)) {
            switch (op) {
                case Add:
                    result = tempResult;
                    break;
                case Minus:
                    result = to_string(-1 * stoi(tempResult));
                    break;
                case Not:
                    if (stoi(tempResult) != 0) {
                        result = "0";
                    } else {
                        result = "1";
                    }
                    break;
            }
        } else {
            switch (op) {
                case Add:
                    result = tempResult;
                    break;
                case Minus:
                    result = IR::generateRegister();
                    IR::addArithmetic(result, Minus, "0", tempResult);
                    break;
                case Not:
                    string temp = IR::generateRegister();
                    IR::addZextTo(temp, tempResult);
                    result = IR::generateRegister();
                    IR::addIcmp(result, Eq, temp, "0");
                    break;
            }
        }
    }
    return ret;
}

void addLibFun() {
    vector<pair<IdentType, int>> par;
    vector<string> pnames;

    string str = "getint";
    SymbolTable::addFuncSymbol(0, str, IntType, par, pnames);

    str = "getch";
    SymbolTable::addFuncSymbol(0, str, IntType, par, pnames);

    par.emplace_back(IntType, 0);
    pnames.emplace_back("cynic");

    str = "putint";
    SymbolTable::addFuncSymbol(0, str, VoidType, par, pnames);

    str = "putch";
    SymbolTable::addFuncSymbol(0, str, VoidType, par, pnames);

    par.clear();

    par.emplace_back(IntType, 1);

    str = "getarray";
    SymbolTable::addFuncSymbol(0, str, IntType, par, pnames);

    par.clear();
    par.emplace_back(IntType, 0);
    par.emplace_back(IntType, 1);
    pnames.emplace_back("test");
    str = "putarray";
    SymbolTable::addFuncSymbol(0, str, VoidType, par, pnames);
}