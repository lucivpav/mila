#ifndef SYMTAB
#define SYMTAB

#include <map>
#include <string>
#include <memory>

#include "llvm/ADT/APFloat.h"
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

using namespace llvm;
using namespace std;

class Object;
class CallableObj;

class SymbolTable {
public:
  SymbolTable(IRBuilder<> & builder,
              LLVMContext & context,
              Module & module);

  enum Modifier { Var, Const };

  struct Symbol {
    shared_ptr<Object> obj;
    Modifier type;
    Value * val;

    Symbol(Object * o, Modifier t, Value * v);
    Symbol(shared_ptr<Object> & o, Modifier t, Value * v);
  private:
    Symbol(const Symbol & s);
    Symbol& operator=(const Symbol & s);
  };

  void setLocalScope(Function * f, const string & fIdent);
  void setGlobalScope();
  bool isLocalScope();
  string getFIdent();

  const Symbol & get(const string & ident); // does not check whether exists
  bool exists(const string & ident) const;
  bool existsGlobal(const string & ident) const;
  bool existsLocal(const string & ident) const;
  bool existsForward(const string & ident) const;

  /* methods print error on fail */
  void declConst(string ident, Value *val, Object * o);

  // first: if we declare several vars at the same time, we need to let
  // the table know when the first Object* was declared and the rest will
  // be treated as refs ... shared_ptr<Object> obj
  void declVar(string ident, Object * o, bool first); // todo: this could return the stored Symbol
  void declCallable(bool forward, string ident, CallableObj * o, Function * f);

  void ensureDeclared(const string & ident) const;
  void ensureNotDeclared(const string & ident) const;
  void ensureNotDeclaredForward(const string & ident) const;
  void ensureConst(const string & ident);
  void ensureNotConst(const string & ident);
private:
  typedef map<string, unique_ptr<Symbol>> Table;
  Table &curTable();
private:
  struct StoredSymbol {
    Modifier type;
    AllocaInst * alloca;
  };

  Table mTable; // globals
  Table mLocals;
  Table mForward; // forward (callable) declarations
  Function * mFunction;
  string mFIdent;
  bool mLocalScope;

  IRBuilder<> & mBuilder;
  LLVMContext & mContext;
  Module & mModule;
};

#endif // SYMTAB
