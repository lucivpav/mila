#include "symtab.h"

#include "util.h"
#include "ast.h"

SymbolTable::SymbolTable(IRBuilder<> & builder, LLVMContext & context,
                         Module & module)
  : mBuilder(builder),
    mContext(context),
    mModule(module)
{
  setGlobalScope();
}

void SymbolTable::setLocalScope(Function *f, const string & fIdent)
{
  mLocalScope = true;
  mFunction = f;
  mFIdent = fIdent;
}

void SymbolTable::setGlobalScope()
{
  mLocalScope = false;
  mFunction = nullptr;
  mLocals.clear();
}

bool SymbolTable::isLocalScope()
{
  return mLocalScope;
}

string SymbolTable::getFIdent()
{
  return mFIdent;
}

SymbolTable::Symbol::Symbol(Object *o, SymbolTable::Modifier t, Value * v)
  :obj(o),type(t), val(v)
{
}

SymbolTable::Symbol::Symbol(shared_ptr<Object> &o, SymbolTable::Modifier t, Value *v)
  :obj(o),type(t), val(v)
{

}

void SymbolTable::declConst(string ident, Value * val, Object *o) {
  ensureNotDeclared(ident);
  assert ( o );
  GlobalVariable * gvar;
  switch (o->getType()) {
  case Object::Integer:{
    gvar = new GlobalVariable(mModule,
                              llvm::Type::getInt32Ty(mContext),
                              true,
                              GlobalValue::ExternalLinkage,
                              0, ident);
    gvar->setInitializer((Constant*)val);
    break;
  }
  case Object::Array:
    error("array cannot be const");
  default: assert ( false );
  }
  curTable()[ident] = unique_ptr<Symbol>(new Symbol(o,Modifier::Const, gvar));
}

void SymbolTable::declVar(string ident, Object * o, bool first) { // todo: refactor
  ensureNotDeclared(ident);
  assert ( o );
  Value * val;
  switch (o->getType()) {
  case Object::Integer:
    if ( mLocalScope ) {
      IRBuilder<> tmp(&mFunction->getEntryBlock(), mFunction->getEntryBlock().begin());
      val = tmp.CreateAlloca(Type::getInt32Ty(mContext),
                              0, ident.c_str());
    } else {
      GlobalVariable * gvar = new GlobalVariable(mModule,
                               llvm::Type::getInt32Ty(mContext),
                               false,
                               GlobalValue::ExternalLinkage,
                               0, ident);
      gvar->setInitializer(ConstantInt::get(mContext, APInt(32, 0, true)));
      val = gvar;
    }
    break;
  case Object::Array:{
    Array * arr = ((Array*)o);
    int from, to;
    arr->initLimits();
    arr->getLimits(from, to);
    assert ( from < to );

    ArrayType * arr_ty = ArrayType::get(
          Type::getInt32Ty(mContext),
          to-from+1);
    GlobalVariable * gvar = new GlobalVariable(mModule,
                              arr_ty,
                              false,
                              GlobalValue::CommonLinkage,
                              0, ident);
    ConstantAggregateZero* agreg = ConstantAggregateZero::get(arr_ty);
    gvar->setInitializer(agreg); // todo: default values are not 0
    val = gvar;
    break;
  }
  default:
    assert ( false );
  }

  static shared_ptr<Object> * base;
  unique_ptr<Symbol> ptr;
  if ( first )
    ptr = unique_ptr<Symbol>(new Symbol(o, Modifier::Var, val));
  else
    ptr = unique_ptr<Symbol>(new Symbol(*base, Modifier::Var, val));
  if ( first ) base = &ptr->obj;
  curTable()[ident] = move(ptr);
}

void SymbolTable::declCallable(bool forward, string ident,
                               CallableObj *o, Function *f)
{
  Table * t;
  if ( forward ) {
    t = &mForward;
    ensureNotDeclaredForward(ident);
  } else {
    t = &mTable;
    ensureNotDeclared(ident);
  }
  (*t)[ident] = unique_ptr<Symbol>(new Symbol(o, Modifier::Var, f));
}

const SymbolTable::Symbol & SymbolTable::get(const string & ident) {
  assert ( exists(ident) );
  /* local */
  if ( mLocalScope ) {
    auto it = mLocals.find(ident);
    if ( it != mLocals.end() ) return *(it->second);
  }
  /* forward */
  auto it = mForward.find(ident);
  if ( it != mForward.end() ) return *(it->second);

  /* global */
  auto & res = mTable.find(ident)->second;
  return *res;
}

bool SymbolTable::exists(const string & ident) const
{
  return existsGlobal(ident) ||
      existsLocal(ident) ||
      existsForward(ident);
}

bool SymbolTable::existsGlobal(const string &ident) const
{
  return mTable.find(ident) != mTable.end();
}

bool SymbolTable::existsLocal(const string &ident) const
{
  if ( mLocalScope ) return mLocals.find(ident) != mLocals.end();
  else return mTable.find(ident) != mTable.end();
}

bool SymbolTable::existsForward(const string &ident) const
{
  return mForward.find(ident) != mForward.end();
}

void SymbolTable::ensureNotDeclared(const string &ident) const
{
  if ( existsLocal(ident) )
    error("Var \'" + ident + "\' already declared");
}

void SymbolTable::ensureConst(const string &ident)
{
  if ( get(ident).type != Modifier::Const )
    error("Var \'" + ident + "\' is not constant");
}

void SymbolTable::ensureNotConst(const string &ident)
{
  if ( get(ident).type == Modifier::Const )
    error("Var \'" + ident + "\' is constant");
}

SymbolTable::Table &SymbolTable::curTable()
{
  return mLocalScope ? mLocals : mTable;
}

void SymbolTable::ensureDeclared(const string & ident) const
{
  if ( !exists(ident) )
    error("Var \'" + ident + "\' not declared");
}

void SymbolTable::ensureNotDeclaredForward(const string &ident) const
{
  if ( existsForward(ident) )
    error("Var \'" + ident + "\' already declared");
}
