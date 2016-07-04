/* ast.h */

#ifndef _TREE_
#define _TREE_

#include <string>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "llvm/IR/CFG.h" // pred_begin

#include "symtab.h"
#include "lexer.h"

using namespace llvm;

void ast_init(LLVMContext & context, Module & module, IRBuilder<> & builder,
              SymbolTable & symTab);

class Object;
class StatmList;

class Node {
public:
   virtual Value* Translate() = 0; // if returns nullptr -> break
   virtual void Print() = 0;
   virtual ~Node() {}
};

class Expr : public Node {
private:
  bool mExpectedConstExpr;
protected:
  bool expectedConstExpr() const;
public:
  Expr();
  void expectConstExpr(bool expect);
};

class Statm : public Node {
public:
   virtual void Print(); // block indent
};

class Var : public Expr {
   std::string name;
public:
   Var(std::string name);
   virtual Value* Translate();
   virtual void Print();

   Value * Pointer();
   const SymbolTable::Symbol &Symbol();
   const string & getName() const;
};

class Numb : public Expr {
   int value;
public:
   Numb(int);
   virtual Value* Translate();
   virtual void Print();
   int NumbValue();
};

class String : public Expr {
  string value;
public:
  String(const string & value);
  virtual Value * Translate();
  virtual void Print();
};

class Bop : public Expr {
   Token::Type op;
   unique_ptr<Expr> left, right;
public:
   Bop(Token::Type, Expr*, Expr*);
   virtual Value* Translate();
   virtual void Print();
};

class UnMinus : public Expr {
   unique_ptr<Expr> expr;
public:
   UnMinus(Expr *e);
   virtual Value* Translate();
   virtual void Print();
};

class Not : public Expr {
   unique_ptr<Expr> expr;
public:
   Not(Expr *e);
   virtual Value* Translate();
   virtual void Print();
};

class Decl: public Statm {
  string ident;
  unique_ptr<Decl> next;
  Object * obj; // only root contains obj
public:
  Decl(const string & ident, Decl * n, Object * o = 0);
  virtual Value* Translate();
  virtual void Print();

  friend class DeclCallable;
};

class DeclConst: public Statm {
  string ident;
  unique_ptr<Expr> expr;
  Object * obj;
public:
   DeclConst(const string & ident, Expr * expr, Object * o);
   virtual Value* Translate();
   virtual void Print();
};

class DeclCallable: public Statm {
  string ident;

  vector<string> paramIdents;
  vector<Type*> paramTypes;

  unique_ptr<Object> returnType;
  unique_ptr<StatmList> body;
private:
  std::string CreateReturnSymbol();
  void CreateArgSymbols(Function * f);
public:
  DeclCallable(string ident, StatmList * params,
               Object * returnType, /* null ? procedure : function */
               StatmList * body /* null ? declaration : definition */
               );
  virtual Value* Translate();
  virtual void Print();
};

class Call: public Statm, public Expr { // multiple inheritance, phhhh :/
  string ident;
  vector<unique_ptr<Expr>> params;
public:
   Call(string ident, const vector<Expr *> &params);

   virtual Value* Translate();
   virtual void Print();
};

class Assign : public Statm {
   unique_ptr<Var> var;
   unique_ptr<Expr> expr;
public:
   Assign(Var*, Expr*);
   virtual Value* Translate();
   virtual void Print();
   Var * getVar();
};

/* set an array element */
class ArraySet : public Statm {
  string ident;
  unique_ptr<Expr> index;
  unique_ptr<Expr> expr;
public:
   ArraySet(string ident, Expr * index, Expr * expr);
   virtual Value* Translate();
   virtual void Print();
};

/* get an array element */
class ArrayGet : public Expr {
  string ident;
  unique_ptr<Expr> index;
public:
   ArrayGet(string ident, Expr * index);
   virtual Value* Translate();
   virtual void Print();
};

class StatmList : public Statm {
  unique_ptr<Statm> statm;
  unique_ptr<StatmList> next;
public:
  StatmList(Statm*, StatmList*);
  virtual  Value* Translate();
  virtual void Print();

  static void merge(StatmList * tailA, StatmList * rootB);
  friend class DeclCallable;
};

class If: public Statm {
  unique_ptr<Expr> ifExpr;
  unique_ptr<Statm> thenStmt;
  unique_ptr<Statm> elseStmt;
public:
  If(Expr*,Statm*,Statm*);
  virtual Value* Translate();
  virtual void Print();
};

class Loop : public Statm {
protected:
  BasicBlock * nextBlock;
public:
  virtual Value* Translate() = 0;
  virtual void Print() = 0;
  BasicBlock * getNextBlock() const;
};

class While: public Loop {
  unique_ptr<Expr> condExpr;
  unique_ptr<Statm> doStmt;
public:
  While();
  void init(Expr*,Statm*);

  virtual Value* Translate();
  virtual void Print();
};

class For: public Loop {
  bool downto;
  unique_ptr<Assign> initStmt;
  unique_ptr<Expr> limitExpr;
  unique_ptr<Statm> doStmt;
public:
  For();
  void init(Assign * initStatm, bool downto,
            Expr * limitExpr, Statm * doStmt);

  virtual Value* Translate();
  virtual void Print();
};

class Break: public Statm {
  const Loop & parent;
public:
  Break(const Loop & parent);
  virtual Value* Translate();
  virtual void Print();
};

class Program: public Statm {
  string name;
public:
  Program(string name);
  virtual Value* Translate();
  virtual void Print();
};

/* pre-defined functions */

class WriteLn : public Statm {
private:
  friend class Write;
  static Function * f;
  static Constant * fmt;
public:
  static Value * call(Expr * e);
  static void declare();
};

class ReadLn : public Statm {
private:
  static Function * f;
  static Constant * fmt;
public:
  static Value * call(Var *v);
  static void declare();
};

class Write : public Statm {
private:
  static Constant * fmt;
public:
  static Value * call(String *s);
  static void declare();
};

class Dec : public Statm { // decrement a variable
public:
  static Value * call(Var *v);
  static void declare();
};

class Exit : public Statm {
private:
  static Function * f;
public:
  static Value * call();
  static void declare();
};

/* objects define a data type*/

class Object {
public:
  enum Type { Invalid, Integer, Array, Callable };
private:
  Type mType;
public:
  Object(Type type);

  static Type ident2type(const char * id);

  Type getType() const;
  virtual void Print() = 0;
};

class Integer : public Object {
public:
  Integer();
  virtual void Print();
};

class Array : public Object {
private:
  unique_ptr<Expr> mFromExpr;
  unique_ptr<Expr> mToExpr;
  int mFrom;
  int mTo;
public:
  Value * getElementPtr(Value *arr, Expr * index);
public:
  Array(Expr * from, Expr * to);

  /* we first need to get int values of limits,
   * therefore we translate the limits expressions first
   */
  void initLimits();

  void getLimits(int & from, int & to);
  virtual void Print();
};

class CallableObj : public Object {
private:
  unsigned mParamCount;
  bool mReturnVoid;
public:
  CallableObj(int paramCount, bool returnVoid);
  unsigned getParamCount();
  bool returnsVoid();
  virtual void Print();
};

#endif
