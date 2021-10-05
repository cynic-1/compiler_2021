//
// Created by song on 2021/10/5.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstring>

using namespace std;

enum TokenType {
    Ident, Number, If, Else, While, Break, Continue, Return,
    Assign, Semicolon, LPar, RPar, LBrace, RBrace, Plus, Mult,
    Div, Lt, Gt, Eq
};

const string tokenTypeStr[32] = {
        "Ident", "Number", "If", "Else", "While", "Break", "Continue", "Return",
        "Assign", "Semicolon", "LPar", "RPar", "LBrace", "RBrace", "Plus", "Mult",
        "Div", "Lt", "Gt", "Eq"
};

string in_buf = "";
string out_buf = "";
char cur;
char next;

const map<string, TokenType> keyWordMap = {
        {"if", If},
        {"else", Else},
        {"while", While},
        {"break", Break},
        {"continue", Continue},
        {"return", Return}
};

const map<string, TokenType> separatorMap = {
        {"=", Assign},
        {";", Semicolon},
        {"(", LPar},
        {")", RPar},
        {"{", LBrace},
        {"}", RBrace},
        {"+", Plus},
        {"*", Mult},
        {"/", Div},
        {"<", Lt},
        {">", Gt},
        {"==", Eq}
};

void skipBlank() {
    while (cur == ' ' || cur == '\n' || cur == '\t' || cur == '\r') {
        cur = cin.get();
    }
}

bool isDigit(const char ch) {
    if (ch >= '0' && ch <= '9')
        return true;
    else
        return false;
}

bool isIdentNonDigit(const char ch) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
        return true;
    else
        return false;
}

void getIdent() {
    in_buf.clear();
    out_buf.clear();
    while (isDigit(cur) || isIdentNonDigit(cur)) {
        in_buf += cur;
        cur = cin.get();
    }

    // check if it's a reserved word
    auto itor = reservedMap.find(in_buf);
    if (itor != reservedMap.end()) {
        out_buf = tokenTypeStr[itor->second] + " " + in_buf;
    } else {
        out_buf = tokenTypeStr[IDENFR] + " " + in_buf;
    }
}

void getIntConst() {
    in_buf.clear();
    out_buf.clear();
    value = 0;
    while (isDigit(cur)) {
        in_buf += cur;
        cur = cin.get();
        value = value * 10 + cur - '0';
    }
    out_buf = tokenTypeStr[INTCON] + " " + in_buf;
}

void getSeparator() {
    in_buf.clear();
    out_buf.clear();

    // store the first char
    in_buf += cur;
    cur = cin.get();
    if (cur == EOF) {
        // error check
    }
    // check if it's a two-character separator
    string temp(in_buf + cur);
    auto iter = separatorMap.find(temp);
    if (iter != separatorMap.end()) {
        in_buf = temp;
        out_buf = tokenTypeStr[iter->second];
        cur = cin.get();
        return;
    }

    // it's a one-character separator
    iter = separatorMap.find(in_buf);
    if (iter != separatorMap.end()) {
        out_buf = tokenTypeStr[iter->second];
        return;
    }

    // could't find the separator
}


int main() {
    cur = cin.get();
    while (cur != EOF) {
        skipBlank(cin);
        if (isIdentNonDigit(cur)) {
            getIdent(cin);
        } else if (isDigit(cur)) {
            getIntConst(cin);
        } else {  // the peek is safe because of the error-checker in getSeparator()
            getSeparator(cin);
        }
        cout << out_buf << endl;
    }
    return 0;
}
