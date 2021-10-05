//
// Created by song on 2021/10/5.
//

#include <iostream>
#include <map>


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

bool isLegalChar(const char ch) {
    return (ch <= 45 && ch >= 40) || ch == 47
    || (ch <= 62 && ch >= 59) || ch == 123 || ch == 125;
}

void getIdent() {
    in_buf.clear();
    out_buf.clear();
    while (isDigit(cur) || isIdentNonDigit(cur)) {
        in_buf += cur;
        cur = cin.get();
    }

    // check if it's a reserved word
    auto itor = keyWordMap.find(in_buf);
    if (itor != keyWordMap.end()) {
        out_buf = tokenTypeStr[itor->second];
    } else {
        out_buf = tokenTypeStr[Ident] + "(" + in_buf + ")";
    }
}

void getIntConst() {
    in_buf.clear();
    out_buf.clear();
    while (isDigit(cur)) {
        in_buf += cur;
        cur = cin.get();
    }
    out_buf = tokenTypeStr[Number] + "(" + in_buf + ")";
}

void getSeparator() {
    in_buf.clear();
    out_buf.clear();

    // store the first char
    in_buf += cur;
    cur = cin.get();

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

}


int main() {
    cur = cin.get();
    while (cur != EOF) {
        skipBlank();
        if (isIdentNonDigit(cur)) {
            getIdent();
        } else if (isDigit(cur)) {
            getIntConst();
        } else if (isLegalChar(cur)){  // the peek is safe because of the error-checker in getSeparator()
            getSeparator();
        } else {
            cout << "Err";
            return 0;
        }
        cout << out_buf << endl;
    }
    return 0;
}
