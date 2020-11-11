#pragma once

#include "location.hh"
#include "utils.h"
#include <map>
#include <string>
#include <vector>

#include "types.h"
#include "usdt.h"

namespace bpftrace {
namespace ast {

class Visitor;

#define DEFINE_ACCEPT void accept(Visitor &v) override;

/**
 * Copy the node but leave all it's child members uninitialized, effecitvely
 * turning it into a leaf node
 */
#define DEFINE_LEAFCOPY(T)                                                     \
  T *leafcopy()                                                                \
  {                                                                            \
    return new T(*this);                                                       \
  };

class Node {
public:
  Node() = default;
  Node(location loc) : loc(loc){};
  Node(const Node &other) = default;
  virtual ~Node() = default;

  virtual void accept(Visitor &v) = 0;

  location loc;
};

class Map;
class Variable;
class Expression : public Node {
public:
  Expression() = default;
  Expression(location loc) : Node(loc){};
  Expression(const Expression &other);
  // NB: do not free any of non-owned pointers we store
  virtual ~Expression() = default;

  SizedType type;
  Map *key_for_map = nullptr;
  Map *map = nullptr; // Only set when this expression is assigned to a map
  Variable *var = nullptr; // Set when this expression is assigned to a variable
  bool is_literal = false;
  bool is_variable = false;
  bool is_map = false;
};
using ExpressionList = std::vector<Expression *>;

class Integer : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Integer)

  explicit Integer(long n, location loc);

  long n;

private:
  Integer(const Integer &other) = default;
};

class PositionalParameter : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(PositionalParameter)

  explicit PositionalParameter(PositionalParameterType ptype,
                               long n,
                               location loc);
  ~PositionalParameter() = default;

  PositionalParameterType ptype;
  long n;
  bool is_in_str = false;

private:
  PositionalParameter(const PositionalParameter &other) = default;
};

class String : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(String)

  explicit String(const std::string &str, location loc);
  ~String() = default;

  std::string str;

private:
  String(const String &other) = default;
};

class StackMode : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(StackMode)

  explicit StackMode(const std::string &mode, location loc);
  ~StackMode() = default;

  std::string mode;

private:
  StackMode(const StackMode &other) = default;
};

class Identifier : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Identifier)

  explicit Identifier(const std::string &ident, location loc);
  ~Identifier() = default;

  std::string ident;

private:
  Identifier(const Identifier &other) = default;
};

class Builtin : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Builtin)

  explicit Builtin(const std::string &ident, location loc);
  ~Builtin() = default;

  std::string ident;
  int probe_id;

private:
  Builtin(const Builtin &other) = default;
};

class Call : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Call)

  explicit Call(const std::string &func, location loc);
  Call(const std::string &func, ExpressionList *vargs, location loc);
  ~Call()
  {
    if (vargs)
      for (Expression *expr : *vargs)
        delete expr;

    delete vargs;
    vargs = nullptr;
  }

  std::string func;
  ExpressionList *vargs = nullptr;

private:
  Call(const Call &other);
};

class Map : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Map)

  explicit Map(const std::string &ident, location loc);
  Map(const std::string &ident, ExpressionList *vargs, location loc);
  ~Map()
  {
    if (vargs)
      for (Expression *expr : *vargs)
        delete expr;

    delete vargs;
    vargs = nullptr;
  }

  std::string ident;
  ExpressionList *vargs = nullptr;
  bool skip_key_validation = false;

private:
  Map(const Map &other);
};

class Variable : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Variable)

  explicit Variable(const std::string &ident, location loc);
  ~Variable() = default;

  std::string ident;

private:
  Variable(const Variable &other) = default;
};

class Binop : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Binop)

  Binop(Expression *left, int op, Expression *right, location loc);

  ~Binop()
  {
    delete left;
    delete right;
    left = nullptr;
    right = nullptr;
  }

  Expression *left = nullptr;
  Expression *right = nullptr;
  int op;

private:
  Binop(const Binop &other);
};

class Unop : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Unop)

  Unop(int op, Expression *expr, location loc = location());
  Unop(int op,
       Expression *expr,
       bool is_post_op = false,
       location loc = location());

  ~Unop()
  {
    delete expr;
    expr = nullptr;
  }

  Expression *expr = nullptr;
  int op;
  bool is_post_op;

private:
  Unop(const Unop &other);
};

class FieldAccess : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(FieldAccess)

  FieldAccess(Expression *expr, const std::string &field);
  FieldAccess(Expression *expr, const std::string &field, location loc);
  FieldAccess(Expression *expr, ssize_t index, location loc);
  ~FieldAccess()
  {
    delete expr;
    expr = nullptr;
  }

  Expression *expr = nullptr;
  std::string field;
  ssize_t index = -1;

private:
  FieldAccess(const FieldAccess &other);
};

class ArrayAccess : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(ArrayAccess)

  ArrayAccess(Expression *expr, Expression *indexpr);
  ArrayAccess(Expression *expr, Expression *indexpr, location loc);
  ~ArrayAccess()
  {
    delete expr;
    delete indexpr;
    expr = nullptr;
    indexpr = nullptr;
  }

  Expression *expr = nullptr;
  Expression *indexpr = nullptr;

private:
  ArrayAccess(const ArrayAccess &other) : Expression(other){};
};

class Cast : public Expression {
public:
  DEFINE_LEAFCOPY(Cast)
  DEFINE_ACCEPT

  Cast(const std::string &type,
       bool is_pointer,
       bool is_double_pointer,
       Expression *expr);
  Cast(const std::string &type,
       bool is_pointer,
       bool is_double_pointer,
       Expression *expr,
       location loc);
  ~Cast()
  {
    delete expr;
    expr = nullptr;
  }

  std::string cast_type;
  bool is_pointer;
  bool is_double_pointer;
  Expression *expr = nullptr;

private:
  Cast(const Cast &other);
};

class Tuple : public Expression
{
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Tuple)

  Tuple(ExpressionList *elems, location loc);
  ~Tuple()
  {
    for (Expression *expr : *elems)
      delete expr;
    delete elems;
  }

  ExpressionList *elems = nullptr;

private:
  Tuple(const Tuple &other);
};

class Statement : public Node {
public:
  Statement() = default;
  Statement(location loc) : Node(loc){};
  Statement(const Statement &other) = default;
};

using StatementList = std::vector<Statement *>;

class ExprStatement : public Statement {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(ExprStatement)

  explicit ExprStatement(Expression *expr, location loc);
  ~ExprStatement()
  {
    delete expr;
    expr = nullptr;
  }

  Expression *expr = nullptr;

private:
  ExprStatement(const ExprStatement &other) : Statement(other){};
};

class AssignMapStatement : public Statement {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(AssignMapStatement)

  AssignMapStatement(Map *map,
                     Expression *expr,
                     bool compound = false,
                     location loc = location());
  ~AssignMapStatement()
  {
    // In a compound assignment, the expression owns the map so
    // we shouldn't free
    if (!compound)
      delete map;
    delete expr;
    map = nullptr;
    expr = nullptr;
  }

  Map *map = nullptr;
  Expression *expr = nullptr;
  bool compound;

private:
  AssignMapStatement(const AssignMapStatement &other) : Statement(other){};
};

class AssignVarStatement : public Statement {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(AssignVarStatement)

  AssignVarStatement(Variable *var,
                     Expression *expr,
                     bool compound = false,
                     location loc = location());
  ~AssignVarStatement()
  {
    // In a compound assignment, the expression owns the map so
    // we shouldn't free
    if (!compound)
      delete var;
    delete expr;
    var = nullptr;
    expr = nullptr;
  }

  Variable *var = nullptr;
  Expression *expr = nullptr;
  bool compound;

private:
  AssignVarStatement(const AssignVarStatement &other) : Statement(other){};
};

class If : public Statement {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(If)

  If(Expression *cond, StatementList *stmts);
  If(Expression *cond, StatementList *stmts, StatementList *else_stmts);
  ~If()
  {
    delete cond;
    cond = nullptr;

    if (stmts)
      for (Statement *s : *stmts)
        delete s;
    delete stmts;
    stmts = nullptr;

    if (else_stmts)
      for (Statement *s : *else_stmts)
        delete s;
    delete else_stmts;
    else_stmts = nullptr;
  }

  Expression *cond = nullptr;
  StatementList *stmts = nullptr;
  StatementList *else_stmts = nullptr;

private:
  If(const If &other);
};

class Unroll : public Statement {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Unroll)

  Unroll(Expression *expr, StatementList *stmts, location loc);
  ~Unroll()
  {
    if (stmts)
      for (Statement *s : *stmts)
        delete s;
    delete stmts;
    stmts = nullptr;
  }

  long int var = 0;
  Expression *expr = nullptr;
  StatementList *stmts = nullptr;

private:
  Unroll(const Unroll &other);
};

class Jump : public Statement
{
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Jump)

  Jump(int ident, location loc = location()) : Statement(loc), ident(ident)
  {
  }
  ~Jump() = default;

  int ident = 0;

private:
  Jump(const Jump &other) = default;
};

class Predicate : public Node {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Predicate)

  explicit Predicate(Expression *expr, location loc);
  ~Predicate()
  {
    delete expr;
    expr = nullptr;
  }

  Expression *expr = nullptr;

private:
  Predicate(const Predicate &other) : Node(other){};
};

class Ternary : public Expression {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Ternary)

  Ternary(Expression *cond, Expression *left, Expression *right, location loc);
  ~Ternary()
  {
    delete cond;
    delete left;
    delete right;
    cond = nullptr;
    left = nullptr;
    right = nullptr;
  }

  Expression *cond = nullptr;
  Expression *left = nullptr;
  Expression *right = nullptr;
};

class While : public Statement
{
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(While)

  While(Expression *cond, StatementList *stmts, location loc)
      : Statement(loc), cond(cond), stmts(stmts)
  {
  }
  ~While()
  {
    delete cond;
    for (auto *stmt : *stmts)
      delete stmt;
    delete stmts;
  }

  Expression *cond = nullptr;
  StatementList *stmts = nullptr;

private:
  While(const While &other);
};

class AttachPoint : public Node {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(AttachPoint)

  explicit AttachPoint(const std::string &raw_input, location loc = location());
  ~AttachPoint() = default;

  // Raw, unparsed input from user, eg. kprobe:vfs_read
  std::string raw_input;

  std::string provider;
  std::string target;
  std::string ns;
  std::string func;
  usdt_probe_entry usdt; // resolved USDT entry, used to support arguments with wildcard matches
  int freq = 0;
  uint64_t len = 0; // for watchpoint probes, the width of watched addr
  std::string mode; // for watchpoint probes, the watch mode
  bool need_expansion = false;
  uint64_t address = 0;
  uint64_t func_offset = 0;

  std::string name(const std::string &attach_point) const;
  std::string name(const std::string &attach_target,
                   const std::string &attach_point) const;

  int index(std::string name);
  void set_index(std::string name, int index);

private:
  AttachPoint(const AttachPoint &other) = default;

  std::map<std::string, int> index_;
};
using AttachPointList = std::vector<AttachPoint *>;

class Probe : public Node {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Probe)

  Probe(AttachPointList *attach_points, Predicate *pred, StatementList *stmts);
  ~Probe()
  {
    if (attach_points)
      for (AttachPoint *ap : *attach_points)
        delete ap;
    delete attach_points;
    attach_points = nullptr;

    delete pred;
    pred = nullptr;

    if (stmts)
      for (Statement *s : *stmts)
        delete s;
    delete stmts;
    stmts = nullptr;
  }

  AttachPointList *attach_points = nullptr;
  Predicate *pred = nullptr;
  StatementList *stmts = nullptr;

  std::string name() const;
  bool need_expansion = false;        // must build a BPF program per wildcard match
  bool need_tp_args_structs = false;  // must import struct for tracepoints

  int index();
  void set_index(int index);

private:
  Probe(const Probe &other);
  int index_ = 0;
};
using ProbeList = std::vector<Probe *>;

class Program : public Node {
public:
  DEFINE_ACCEPT
  DEFINE_LEAFCOPY(Program)

  Program(const std::string &c_definitions, ProbeList *probes);

  ~Program()
  {
    if (probes)
      for (Probe *p : *probes)
        delete p;
    delete probes;
    probes = nullptr;
  }

  std::string c_definitions;
  ProbeList *probes = nullptr;

private:
  Program(const Program &other);
};

std::string opstr(Binop &binop);
std::string opstr(Unop &unop);
std::string opstr(Jump &jump);

#undef DEFINE_ACCEPT

} // namespace ast
} // namespace bpftrace
