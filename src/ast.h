#ifndef MANIQ_AST_H
#define MANIQ_AST_H
#include <memory>
#include <ostream>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::unique_ptr;

enum class Types { Int, Bool, Void, Unknown };

Types to_type(const string& name);
string to_string(Types type);

struct Expression {
    virtual ~Expression() = default;
    int line = 0;
    int col = 0;
    Types resultType = Types::Unknown;
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
    int line = 0;
    int col = 0;
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
    int line = 0;
    int col = 0;
};

struct Parameter {
    string type;
    string name;
    // Future addition:
    // unique_ptr<Expression> defaultValue; // optional. e.g.: int a = 1
    int line = 0;
    int col = 0;
};

struct Function : Declaration {
    string name;
    vector<Parameter> parameters;
    string returnType;
    unique_ptr<Block> body;
};

using Program = vector<unique_ptr<Function>>;

void dump(std::ostream& out, const Expression& node, int depth = 0);
void dump(std::ostream& out, const Statement& node, int depth = 0);
void dump(std::ostream& out, const Function& node, int depth = 0);
void dump(std::ostream& out, const Program& program);

#endif //MANIQ_AST_H
