#ifndef MANIQ_AST_H
#define MANIQ_AST_H
#include <memory>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::unique_ptr;

struct Expression {
    virtual ~Expression() = default;
};

struct Int : Expression {
    int value;
};

struct Bool : Expression {
    bool value;
};

struct Name : Expression {
    string name;
};

struct Unary : Expression {
    char op;
    unique_ptr<Expression> operand;
};

struct Binary : Expression {
    string op;
    unique_ptr<Expression> left;
    unique_ptr<Expression> right;
};

struct Call : Expression {
    string name;
    vector<unique_ptr<Expression>> args;
};

struct Statement {
    virtual ~Statement() = default;
};

struct ExprStmt : Statement {
    unique_ptr<Expression> expr;
};

struct Variable : Statement {
    string type;
    string name;
    unique_ptr<Expression> value;
};

struct Assign : Statement {
    string name;
    unique_ptr<Expression> value;
};

struct Block : Statement {
    vector<unique_ptr<Statement>> body;
};

struct If : Statement {
    unique_ptr<Expression> condition;
    unique_ptr<Block> body;
    unique_ptr<Statement> elseBody;
};

struct While : Statement {
    unique_ptr<Expression> condition;
    unique_ptr<Block> body;
};

struct Return : Statement {
    unique_ptr<Expression> value;
};

struct Declaration {
    virtual ~Declaration() = default;
};

struct Parameter {
    string type;
    string name;
    // Future addition:
    // unique_ptr<Expression> defaultValue; // optional. e.g.: int a = 1
};

struct Function : Declaration {
    string name;
    vector<Parameter> parameters;
    string returnType;
    unique_ptr<Block> body;
};

#endif //MANIQ_AST_H
