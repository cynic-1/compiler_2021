//
// Created by song on 2021/12/11.
//

#include "IR.h"
#include "utils.h"

int IR::index = 0;
string IR::irCodes;
string IR::temp;
bool IR::brButNotLabelYet = false;


void IR::addFuncDef(IdentType returnType, const string& funcName, vector<symbolTableNode>& params) {
    irCodes += ("define " + typeNames[returnType] + " @" + funcName);
    IR::addLParen();
    IR::addFuncParams(params);
    IR::addRParen();
}
void IR::addFuncParams(vector<symbolTableNode>& params) {
    bool noParam = true;
    if (!params.empty()) {
        for (auto & param : params) {
            if (noParam) {
                IR::addFuncParam(param);
                noParam = false;
                continue;
            }
            IR::addCommaSpace();
            IR::addFuncParam(param);
        }
    }
}
void IR::addFuncParam(symbolTableNode & param) {
    if (param.dimension == 0) {
        irCodes += ("i32 " + param.regName);
    } else {
        irCodes += ("i32* " + param.pointerName);
    }
}

/**
 * <result> = add <ty> <op1>, <op2>
   <result> = sub <ty> <op1>, <op2>
   <result> = mul <ty> <op1>, <op2>
   <result> = sdiv <ty> <op1>, <op2>
   <result> = srem <ty> <op1>, <op2>
 * @param result
 * @param op
 * @param op1
 * @param op2
 */
void IR::addArithmetic(const string& result, TokenType op, const string &op1, const string& op2) {
    irCodes += (result + " = " + typeNames[op] + " i32 " + op1);
    addCommaSpace();
    irCodes += op2;
    cout << irCodes << endl;
    IR::addNewLine();
}

/**
 * <result> = call [ret attrs] <ty> <fnptrval>(<function args>)
 * @param result
 * @param funcName
 * @param arguments
 */
void IR::addCall(const string& result, const string& funcName, vector<string>& arguments, const vector<pair<IdentType, int>>& argumentsType) {
    irCodes += (result + " = call i32 @" + funcName);
    IR::addLParen();
    addArguments(arguments, argumentsType);
    IR::addRParen();
    IR::addNewLine();
}

void IR::addCall(const string &funcName, vector<string>& arguments, const vector<pair<IdentType, int>>& argumentsType) {
    irCodes += ("call void @" + funcName);
    IR::addLParen();
    addArguments(arguments, argumentsType);
    IR::addRParen();
    IR::addNewLine();
}

void IR::addArguments(vector<string>& arguments, const vector<pair<IdentType, int>>& argumentsType) {
    bool noArg = true;
    if (!arguments.empty()) {
        for (int i = 0; i < arguments.size(); i++) {
            if (noArg) {
                IR::addArgument(arguments[i], argumentsType[i].second > 0);
                noArg = false;
                continue;
            }
            IR::addCommaSpace();
            IR::addArgument(arguments[i], argumentsType[i].second > 0);
        }
    }
}

void IR::addArgument(string & arguments, bool isArray) {
    if (isArray) {
        irCodes += ("i32* " + arguments);
    } else {
        irCodes += ("i32 " + arguments);
    }
}

/**
 * <result> = alloca <type>
 * @param result
 */
void IR::addAlloca(const string &result, const vector<int> &axis) {
    string code;
    code = (result + " = alloca ");
    code += getArrayTypeString(axis);
    irCodes += code;
    IR::addNewLine();
}

string IR::getArrayTypeString(const vector<int> &axis, int idx) {
    string code;
    for (int i = idx; i < axis.size(); i++) {
        code += ("[" + to_string(axis[i]) + " x ");
    }
    code += "i32";
    for (int i = idx; i < axis.size(); ++i) {
        code += "]";
    }
    return code;
}

void IR::addMemset(const string &pointerName, int val, int bytes) {
    irCodes += ("call void @memset(i32* " + pointerName + ", i32 " + to_string(val) + ", i32 " + to_string(bytes) + ")");
    IR::addNewLine();
}

void IR::addGetElePtr(const string &pointerName, const string &basePointerName, const vector<int>& axis, const string& offset) {
    string str, type;
    type = getArrayTypeString(axis);
    str = (pointerName + " = getelementptr " + type + ", " + type + "* " + basePointerName);
    if (!offset.empty()) {
        str += (", i32 " + offset);
    } else {
        for (int j = 0; j <= axis.size(); ++j) {
            str += (", i32 0");
        }
    }
    irCodes += str;
    IR::addNewLine();
}

void IR::addGetElePtrLater(const string &pointerName, const string &basePointerName, const vector<int>& axis, const string& offset) {
    string str, type;
    type = getArrayTypeString(axis);
    str = (pointerName + " = getelementptr " + type + ", " + type + "* " + basePointerName);
    if (!offset.empty()) {
        str += (", i32 " + offset);
    } else {
        for (int j = 0; j <= axis.size(); ++j) {
            str += (", i32 0");
        }
    }
    temp += str;
}

void IR::addGlobalArray(const string& name, bool isConst, const vector<int> &axis,
                        const vector<vector<string>>& initValues) {
    string code;
    if (isConst) {
        code = ("@" + name + " = dso_local constant ");
    } else {
        code = ("@" + name + " = dso_local global ");
    }
    int idxInitVal = 0;
    IR::addGlobalArrayIter(code, axis, 0, initValues, idxInitVal);
    irCodes += code;
    IR::addNewLine();
}

void IR::addGlobalArrayIter(string &code, const vector<int> &axis, int idxAxis, const vector<vector<string>>& initValues,
                            int &idxInitVal) {
    if (idxAxis > axis.size()) return;
    if (idxInitVal >= initValues.size()) {
        if (idxAxis < axis.size()) {
            code += IR::getArrayTypeString(axis, idxAxis);
        }
        code += " zeroinitializer";
        return;
    }

    if (idxAxis == axis.size()) {
        vector<string> initPerBrace = initValues[idxInitVal++];
        code += " [";
        for (int j = 0; j < axis.back()-1; ++j) {
            if (j < initPerBrace.size()) {
                /******全局数组的 ConstInitVal/InitVal 中的 ConstExp/Exp 必须是编译时可求值的常量表达式。******/
                if (!isValue(initPerBrace[j])) exit(-1);
                /**************************************************************************************/
                code += "i32 " + initPerBrace[j] + ", ";
            } else {
                code += "i32 0, ";
            }
        }
        if (axis.back()-1 < initPerBrace.size()) {
            /******全局数组的 ConstInitVal/InitVal 中的 ConstExp/Exp 必须是编译时可求值的常量表达式。******/
            if (!isValue(initPerBrace.back())) exit(-1);
            /**************************************************************************************/
            code += "i32 " + initPerBrace.back();
        } else {
            code += "i32 0";
        }
        code += "]";
        return;
    }

    if (idxAxis == 0) {
        code += IR::getArrayTypeString(axis, idxAxis);
        IR::addGlobalArrayIter(code, axis, idxAxis+1, initValues, idxInitVal);
    } else {
        code += " [";
        for (int j = 0; j < axis[idxAxis-1]-1; ++j) {
            code += IR::getArrayTypeString(axis, idxAxis);
            IR::addGlobalArrayIter(code, axis, idxAxis+1, initValues, idxInitVal);
            code += ", ";
        }
        code += IR::getArrayTypeString(axis, idxAxis);
        IR::addGlobalArrayIter(code, axis, idxAxis+1, initValues, idxInitVal);
        code += "]";
    }
}


/**
 * <result> = load <ty>, <ty>* <pointer>
 * @param result
 * @param pointer
 */
void IR::addLoad(const string& result, const string& pointer) {
    irCodes += (result + " = load i32, i32* " + pointer);
    IR::addNewLine();
}

/**
 * store <ty> <value>, <ty>* <pointer>
 * @param value
 * @param pointer
 */
void IR::addStore(const string& value, const string pointer) {
    string s = "store i32 " + value + ", i32* " + pointer;
    irCodes += s;
    IR::addNewLine();
}

void IR::addGlobal(const string &name, const string& value) {
    irCodes += ("@" + name + " = dso_local global i32 " + value);
    IR::addNewLine();
}

/**
 * <result> = icmp <cond> <ty> <op1>, <op2>
 * e.g. %14 = icmp eq i32 %13, 10
 * @param result
 * @param cond
 * @param op1
 * @param op2
 */
void IR::addIcmp(const string &result, TokenType cond, const string& op1, const string& op2) {
    irCodes += (result + " = icmp " + typeNames[cond] + " i32 " + op1 + ", " + op2);
    IR::addNewLine();
}

/**
 * <result> = and <ty> <op1>, <op2>
 * @param result
 * @param value
 */
void IR::addZextTo(const string &result, const string &value) {
    irCodes += (result + " = zext i1 " + value + " to i32");
    IR::addNewLine();
}

void IR::addLabel(const string& label) {
    IR::addNewLine();
    irCodes += (label.substr(1) + ":");
    IR::addNewLine();
    brButNotLabelYet = false;
}

void IR::addBr(const string& cond, const string& trueLabel, const string& falseLabel) {
    if (!brButNotLabelYet) {
        irCodes += ("br i1 " + cond + ", label " + trueLabel + ", label " + falseLabel);
        IR::addNewLine();
        brButNotLabelYet = true;
    }
}

void IR::addBr(const string& dest) {
    if (!brButNotLabelYet) {
        irCodes += ("br label " + dest);
        IR::addNewLine();
        brButNotLabelYet = true;
    }
}

void IR::addRet(const string& value) {
    irCodes += ("ret i32 " + value);
    IR::addNewLine();
}

void IR::addRet() {
    irCodes += "ret void";
    IR::addNewLine();
}

void IR::addLParen() {
    irCodes += "(";
}
void IR::addRParen() {
    irCodes += ")";
}
void IR::addLBrace() {
    irCodes += "{";
    IR::addNewLine();
}
void IR::addRBrace() {
    irCodes += "}";
    IR::addNewLine();
}
void IR::addCommaSpace() {
    irCodes += ", ";
}
void IR::addNewLine() {
    irCodes += "\n";
    write2File();
    if (!temp.empty()) {
        irCodes = temp;
        temp.clear();
        addNewLine();
    }
}

//string IR::generateLabel() {
//    return REG_PREFIX + to_string(index++);
//}

string IR::generateRegister() {
    return REG_PREFIX + to_string(index++);
}


void IR::resetIndex() {
    index = 0;
}

void IR::write2File() {
    irFile << irCodes;
    irCodes.clear();
}

void IR::addFuncDecl() {
    irFile << "declare i32 @getch()\n"
              "declare void @putch(i32)\n"
              "declare i32 @getint()\n"
              "declare void @putint(i32)\n"
              "declare i32 @getarray(i32*)\n"
              "declare void @putarray(i32, i32*)\n"
              "declare void @memset(i32*, i32, i32)\n";
}


