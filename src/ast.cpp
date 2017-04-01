/* ast.cpp */

#include "ast.h"

#include <cstdio>
#include <vector>
#include <iostream>

#include "util.h"

using namespace std;

static LLVMContext* TheContext;
static Module* TheModule;
static IRBuilder<>* Builder;
static SymbolTable* symbolTable;

/*
 * used for print debugging. means how 'burried' we are currently in a block,
 * e.g. number of tabs inside
 */
static int printIndent;

static bool foundExit; // the last translated statm was exit

void ast_init(LLVMContext & context, Module & module, IRBuilder<> & builder,
              SymbolTable & symTab)
{
  TheContext = &context;
  TheModule = &module;
  Builder = &builder;
  symbolTable = &symTab;
  printIndent = 0;
  foundExit = false;

  WriteLn::declare();
  ReadLn::declare();
  Write::declare();
  Dec::declare();
  Exit::declare();
}

Var::Var(std::string a)
{ name = a; }

Numb::Numb(int v)
{ value = v; }

int Numb::NumbValue()
{ return value; }

Bop::Bop(Token::Type o, Expr *l, Expr *r)
  :op(o), left(l), right(r)
{}

UnMinus::UnMinus(Expr *e)
  :expr(e)
{}

Assign::Assign(Var *v, Expr *e)
  :var(v), expr(e)
{}

StatmList::StatmList(Statm *s, StatmList *n)
  :statm(s), next(n)
{
}

// print methods

void Var::Print()
{
  printf("%s", name.c_str());
}

void Numb::Print()
{
	printf("%d", value);
}

void Bop::Print()
{
  printf("(");
   left->Print();
   switch (op) {
   case Token::PLUS: /* int op(int,int) */
	   printf("+");
      break;
   case Token::MINUS:
	   printf("-");
      break;
   case Token::TIMES:
	   printf("*");
      break;
   case Token::kwDIV:
     printf(" div ");
      break;
   case Token::kwMOD:
     printf(" mod ");
      break;
   case Token::EQ: /* bool op(int,int) */
     printf("=");
      break;
   case Token::NEQ:
     printf("<>");
      break;
   case Token::LT:
     printf("<");
      break;
   case Token::GT:
     printf(">");
      break;
   case Token::LTE:
     printf("<=");
      break;
   case Token::GTE:
     printf(">=");
      break;
   case Token::kwAND: /* bool op(bool,bool) */
     printf(" and ");
      break;
   case Token::kwOR:
     printf(" or ");
      break;
   default:
     assert ( false );
   }
   right->Print();
  printf(")");
}

void UnMinus::Print()
{
	printf("-");
	expr->Print();
}

void Assign::Print()
{
  Statm::Print();
  var->Print();
  printf(" := ");
  expr->Print();
}

Var *Assign::getVar()
{
  return var.get();
}

void StatmList::Print()
{
   StatmList *s = this;
   do {
      s->statm->Print();
      printf("\n");
      s = s->next.get();
   } while (s);
}

void StatmList::merge(StatmList *tail, StatmList *root)
{
  tail->next = unique_ptr<StatmList>(root);
}

Value* Var::Translate() // return dereferenced val
{
  if ( expectedConstExpr() ) {
    symbolTable->ensureConst(name);
    // todo:
    error("constant as a constexpr not yet implemented");
  }
  symbolTable->ensureDeclared(name);
  const SymbolTable::Symbol & s = symbolTable->get(name);
  switch ( s.obj->getType() ) {
  case Object::Integer:
    return new LoadInst(Pointer(), name, false, Builder->GetInsertBlock());
  case Object::Callable: // todo: what if we want to access return val?
    if ( ((CallableObj*)s.obj.get())->getParamCount() != 0 )
      return Var(name+"_return").Translate();
    else
      return Call(name, vector<Expr*>()).Translate();
  default: assert ( false );
  }
}

Value *Var::Pointer() // return pointer to value
{
  symbolTable->ensureDeclared(name);
  return symbolTable->get(name).val;
}

const SymbolTable::Symbol & Var::Symbol()
{
  symbolTable->ensureDeclared(name);
  return symbolTable->get(name);
}

const string &Var::getName() const
{
  return name;
}

Value* Numb::Translate()
{
  return ConstantInt::get(*TheContext, APInt(32, value, true));
}

Value* Bop::Translate()
{
   Value* l = left->Translate();
   Value* r = right->Translate();
   switch (op) {
   case Token::PLUS:
     return Builder->CreateAdd(l, r, "addtmp");
   case Token::MINUS:
     return Builder->CreateSub(l, r, "subtmp");
   case Token::TIMES:
     return Builder->CreateMul(l, r, "multmp");
   case Token::kwDIV:
     return Builder->CreateSDiv(l, r, "divtmp");
   case Token::kwMOD:
     return Builder->CreateSRem(l, r, "modtmp");
   case Token::EQ: // todo below
     return Builder->CreateICmpEQ(l, r, "eqtmp");
   case Token::NEQ:
     return Builder->CreateICmpNE(l, r, "netmp");
   case Token::LT:
     return Builder->CreateICmpSLT(l, r, "lttmp");
   case Token::GT:
     return Builder->CreateICmpSGT(l, r, "gttmp");
   case Token::LTE:
     return Builder->CreateICmpSLE(l, r, "ltetmp");
   case Token::GTE:
     return Builder->CreateICmpSGE(l, r, "gtetmp");
   case Token::kwAND: /* bool op(bool,bool) */
     return Builder->CreateAnd(l, r, "andtmp");
   case Token::kwOR:
     return Builder->CreateOr(l, r, "ortmp");
   default:
     assert ( false );
   }
}

Value* UnMinus::Translate()
{
  return Builder->CreateSub(Numb(0).Translate(), expr->Translate(), "unsubtmp");
}

Value* Assign::Translate()
{
  Value * v = var->Pointer();
  Value * e = expr->Translate();
  symbolTable->ensureNotConst(var->getName());

  Object::Type type = var->Symbol().obj->getType();
  if ( type == Object::Callable )
    v = Var(var->getName()+"_return").Pointer();

  Builder->CreateStore(e, v);
  return v;
}

Value* StatmList::Translate()
{
   StatmList *s = this;
   do {
     s->statm->Translate();
     if ( foundExit ) {
       foundExit = false;
       return nullptr;
     }
      s = s->next.get();
   } while (s);
  // the returned value has a meaning
  return Constant::getNullValue(Type::getInt32Ty(*TheContext));
}

Decl::Decl(const string &ident, Decl *n, Object *o)
  :ident(ident), next(n), obj(o)
{
}

Value *Decl::Translate()
{
  Decl *d = this;
  bool first = true;
  do {
    symbolTable->declVar(d->ident, obj, first);
    first = false;
    d = d->next.get();
  } while ( d );
  return nullptr;
}

void Decl::Print()
{
  Statm::Print();
   Decl *d = this;
   cout << "var";
   do {
     cout << " " << d->ident << ": ";
     obj->Print();
     d = d->next.get();
   } while (d);
   cout << endl; // should not be when params
}

DeclConst::DeclConst(const string &ident, Expr *expr, Object * o)
  :ident(ident), expr(expr), obj(o)
{
}

Value *DeclConst::Translate()
{
  symbolTable->declConst(ident, expr->Translate(), obj);
  return Constant::getNullValue(Type::getInt32Ty(*TheContext));
}

void DeclConst::Print()
{
  Statm::Print();
  cout << "const";
  cout << " " << ident << " = "; expr->Print();
  cout << endl;
}

Not::Not(Expr *e)
  :expr(e)
{
}

Value *Not::Translate()
{
  return Builder->CreateXor(expr->Translate(),
                            ConstantInt::get(*TheContext, APInt(1, 1, true)));
}

void Not::Print()
{
  //todo
  cout << "not ";
  expr->Print();
}

If::If(Expr *a, Statm *b, Statm *c)
  :ifExpr(a), thenStmt(b), elseStmt(c)
{
}

Value *If::Translate()
{
  Value * cond = ifExpr->Translate();
  assert ( cond );
  cond = Builder->CreateICmpNE(
        cond, ConstantInt::get(*TheContext, APInt(1, 0, true)) );

  Function * f = Builder->GetInsertBlock()->getParent();
  BasicBlock * Then = BasicBlock::Create(*TheContext, "then", f);
  BasicBlock * Else;
  Else = BasicBlock::Create(*TheContext, "else", f);
  BasicBlock * Merge = BasicBlock::Create(*TheContext, "ifcont", f);
  Builder->CreateCondBr(cond, Then, Else);

  /* then */
  Builder->SetInsertPoint(Then);
  Value * thenV = thenStmt->Translate();
  foundExit = false;
  if ( thenV ) // break has yet to be generated
    Builder->CreateBr(Merge);
  Then = Builder->GetInsertBlock();

  /* else */
  Value * elseV;
  Builder->SetInsertPoint(Else);
  if ( elseStmt ) {
  elseV = elseStmt->Translate();
  foundExit = false;
  }
  if ( elseV ) // break has yet to be generated
    Builder->CreateBr(Merge);
  Else = Builder->GetInsertBlock();

  /* merge */
  Builder->SetInsertPoint(Merge);

  return Merge;
}

void If::Print()
{
  Statm::Print();
  cout << "if "; ifExpr->Print(); cout << " then\n";
  printIndent++;
  thenStmt->Print(); cout << endl;
  printIndent--;
  if ( elseStmt ) {
    Statm::Print();
    cout << "else\n";
    printIndent++;
    elseStmt->Print();
    printIndent--;
  }
}

void Statm::Print()
{
  for ( int i = 0 ; i < printIndent ; ++i )
    cout << " ";
}

While::While()
  :condExpr(nullptr), doStmt(nullptr)
{

}

void While::init(Expr *cond, Statm *statm)
{
  condExpr = unique_ptr<Expr>(cond);
  doStmt = unique_ptr<Statm>(statm);
}

Value *While::Translate()
{
  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  BasicBlock *condBB =
      BasicBlock::Create(*TheContext, "condblock", TheFunction);
  BasicBlock *LoopBB =
      BasicBlock::Create(*TheContext, "loop", TheFunction);
  nextBlock =
      BasicBlock::Create(*TheContext, "afterloop", TheFunction);

  Builder->CreateBr(condBB);

  /* condition */
  Builder->SetInsertPoint(condBB);

  assert ( condExpr && doStmt );
  Value * condV = condExpr->Translate();
  assert ( condV );

  condV = Builder->CreateICmpNE(
        condV, ConstantInt::get(*TheContext, APInt(1, 0, true)) , "cond");
  Builder->CreateCondBr(condV, LoopBB, nextBlock);

  /* loop */
  Builder->SetInsertPoint(LoopBB);
  if ( doStmt->Translate() ) // break has yet to be generated
    Builder->CreateBr(condBB);

  /* next */
  Builder->SetInsertPoint(nextBlock);

  foundExit = false;
  return Constant::getNullValue(Type::getInt32Ty(*TheContext));
}

void While::Print()
{
  Statm::Print();
  cout << "while "; condExpr->Print(); cout << " do\n";
  printIndent++;
  doStmt->Print(); cout << endl;
  printIndent--;
}

BasicBlock * Loop::getNextBlock() const
{
  return nextBlock;
}

For::For()
  :limitExpr(nullptr), doStmt(nullptr)
{

}

void For::init(Assign *initStatm, bool dwnto, Expr *lim, Statm *doStatm)
{
  initStmt = unique_ptr<Assign>(initStatm);
  limitExpr = unique_ptr<Expr>(lim);
  doStmt = unique_ptr<Statm>(doStatm);
  downto = dwnto;
}

Value *For::Translate()
{
  assert ( initStmt && limitExpr && doStmt );

  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  BasicBlock *condBB =
      BasicBlock::Create(*TheContext, "condblock", TheFunction);
  BasicBlock *LoopBB =
      BasicBlock::Create(*TheContext, "loop", TheFunction);
  nextBlock =
      BasicBlock::Create(*TheContext, "afterloop", TheFunction);

  /* init */

  initStmt->Translate();
  Builder->CreateBr(condBB);

  /* condition */
  Builder->SetInsertPoint(condBB);

  Value * limitV = limitExpr->Translate();
  assert ( limitV );

  Var * var = initStmt->getVar();

  Value * condV;
  if ( downto )
    condV = Builder->CreateICmpSGE(var->Translate(), limitV, "gtetmp");
  else
    condV = Builder->CreateICmpSLE(var->Translate(), limitV, "ltetmp");

  Builder->CreateCondBr(condV, LoopBB, nextBlock);

  /* loop */
  Builder->SetInsertPoint(LoopBB);
  bool genBreak = doStmt->Translate();

  /* iterate */
  Numb* one = new Numb(1);
  Value * updated = Bop( downto ? Token::MINUS : Token::PLUS, new Var(*var), one).Translate();
  Builder->CreateStore(updated, var->Pointer());

  /* loop end */
  if ( genBreak ) // break has yet to be generated
    Builder->CreateBr(condBB);

  /* next */
  Builder->SetInsertPoint(nextBlock);

  foundExit = false;
  return Constant::getNullValue(Type::getInt32Ty(*TheContext));

}

void For::Print()
{
  assert ( initStmt && limitExpr && doStmt );
  // todo
}

Break::Break(const Loop &parent)
  :parent(parent)
{
}

Value *Break::Translate()
{
  Builder->CreateBr(parent.getNextBlock());
  return nullptr;
}

void Break::Print()
{
  Statm::Print();
  cout << "break\n";
}

/* object */

const struct {const char* word; Object::Type type;} dataTypeTable[] = {
  {"integer", Object::Integer},
  {"array", Object::Array},
  {NULL, Object::Invalid}
};

Object::Object(Object::Type type)
  :mType(type)
{
}

Object::Type Object::ident2type(const char *id)
{
  for ( int i = 0 ; dataTypeTable[i].word ; ++i )
    if ( strcmp(id,dataTypeTable[i].word) == 0 )
      return dataTypeTable[i].type;
  return Invalid;
}

Object::Type Object::getType() const
{
  return mType;
}

Integer::Integer()
  :Object(Type::Integer)
{
}

void Integer::Print()
{
  cout << "integer";
}

Value *Array::getElementPtr(Value * arr, Expr *index)
{
  Value * v = index->Translate();
  Value * real_idx = Builder->CreateAdd(v, Numb(-mFrom).Translate());

  vector<Value*> indices = {
    /* first index is a pointer offset. this is typically zero */
    ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0),
    real_idx };

  return GetElementPtrInst::Create((Constant*)arr, indices,
                                   "", Builder->GetInsertBlock());
}

Array::Array(Expr *from, Expr *to)
  : Object(Type::Array),
    mFromExpr(from), mToExpr(to)
{
  assert ( mFromExpr && mToExpr );
}

void Array::initLimits()
{
  mFromExpr->expectConstExpr(true);
  mToExpr->expectConstExpr(true);
  Value * l = mFromExpr->Translate();
  Value * r = mToExpr->Translate();

  // todo: change to cast<>
  ConstantInt * lptr = (dyn_cast<ConstantInt>(l));
  ConstantInt * rptr = (dyn_cast<ConstantInt>(r));

  assert ( lptr && rptr );

  mFrom = lptr->getSExtValue();
  mTo = rptr->getSExtValue();
}

void Array::getLimits(int &from, int &to)
{
  from = mFrom;
  to = mTo;
}

void Array::Print()
{
  cout << "array [" << mFrom <<
          ".." << mTo << "] of integer";
}

bool Expr::expectedConstExpr() const
{
  return mExpectedConstExpr;
}

Expr::Expr()
  :mExpectedConstExpr(false)
{
}

void Expr::expectConstExpr(bool expect)
{
  mExpectedConstExpr = expect;
}

Program::Program(string name)
  :name(name)
{
}

Value *Program::Translate()
{
  /* program statm is currently NO-OP */
  return nullptr;
}

void Program::Print()
{
  // todo
}

void DeclCallable::CreateArgSymbols(Function *f)
{
  Function::arg_iterator it = f->arg_begin();
  for ( unsigned idx = 0, e = paramIdents.size();
        idx != e;
        ++idx, ++it) {
    symbolTable->declVar(paramIdents[idx], new Integer(), true); // todo
    const SymbolTable::Symbol & s = symbolTable->get(paramIdents[idx]);
    Builder->CreateStore(it, s.val);
  }
}
DeclCallable::DeclCallable(string ident, StatmList *params, Object *returnType, StatmList *body)
  :ident(ident),
    returnType(returnType),
    body(body)
{
  unique_ptr<StatmList> ptr(params);
  for ( StatmList * argList = params;
        argList;
        argList = argList->next.get() )
    for ( Decl * arg = ((Decl*)argList->statm.get());
          arg;
          arg = arg->next.get() ) {
      paramIdents.push_back(arg->ident);
      paramTypes.push_back(Type::getInt32Ty(*TheContext));
    }
}

Value *DeclCallable::Translate()
{
  Type * returnTy;
  if ( returnType ) returnTy = Type::getInt32Ty(*TheContext);
  else returnTy = Type::getVoidTy(*TheContext);
  FunctionType * fTy = FunctionType::get(returnTy, paramTypes, false);

  Function * f = nullptr;

  if ( symbolTable->exists(ident) )
    f = (Function*)symbolTable->get(ident).val;
  else
    f = Function::Create(fTy, Function::ExternalLinkage, ident, TheModule);

  symbolTable->declCallable(!body, ident,
                            new CallableObj(paramIdents.size(),!returnType), f);
  if ( !body ) return nullptr;

  BasicBlock * b = BasicBlock::Create(*TheContext, "body", f);
  Builder->SetInsertPoint(b);

  string retIdent;
  symbolTable->setLocalScope(f, ident);
  if ( returnType ) retIdent = CreateReturnSymbol();
  CreateArgSymbols(f);

  unsigned idx = 0;
  for ( auto & a : f->args() )
    a.setName(paramIdents[idx++]);

  body->Translate();

  BasicBlock * bb = Builder->GetInsertBlock();
  assert ( bb );

  /* if last instruction was not ret */
  if ( bb->empty() || !dyn_cast<ReturnInst>(&b->back()) ) {
    if ( returnType ) Builder->CreateRet(Var(retIdent).Translate());
    else Builder->CreateRetVoid();
    symbolTable->setGlobalScope();
  }

  symbolTable->setGlobalScope();
  return nullptr;
}

std::string DeclCallable::CreateReturnSymbol()
{
  string ret_ident = ident+"_return";
  Decl(ret_ident, 0, new Integer()).Translate();
  Assign(new Var(ret_ident), new Numb(0)).Translate();
  return ret_ident;
}

void DeclCallable::Print()
{
  Statm::Print();
  bool procedure = !returnType;
  cout << (procedure ? "procedure " : "function ") << ident;

  if ( paramIdents.size() ) {
    cout << "(";
    bool first = true;
    for ( const auto & ident : paramIdents ) {
      if ( !first ) cout << ", ";
      first = false;
      cout << ident;
    }
    cout << ")";
  }

  if ( !procedure ) cout << ": integer";
  if ( body ) {
    cout << "\nbegin\n";
    body->Print();
    cout << "end;\n";
  }
  else cout << "forward;";
}

CallableObj::CallableObj(int paramCount, bool returnVoid)
  :Object(Type::Callable),
    mParamCount(paramCount),
    mReturnVoid(returnVoid)
{

}

unsigned CallableObj::getParamCount()
{
  return mParamCount;
}

bool CallableObj::returnsVoid()
{
  return mReturnVoid;
}

void CallableObj::Print()
{
  cout << "callable with " + to_string(mParamCount)
          + " parameters";
}
Call::Call(string ident, const vector<Expr *>& params)
  :ident(ident)
{
  for ( auto & e : params )
    this->params.push_back(move(unique_ptr<Expr>(e)));
}

Value *Call::Translate()
{
  symbolTable->ensureDeclared(ident);
  const SymbolTable::Symbol & s = symbolTable->get(ident);
  if ( s.obj->getType() != Object::Callable )
    error(ident + " is not callable");

  CallableObj * co = ((CallableObj*)s.obj.get());
  unsigned paramCount = co->getParamCount();

  if ( params.size() != paramCount )
    error(ident + " does not take "
          + to_string(params.size()) + " arguments(s)");

  if ( ident == "writeln" ) return WriteLn::call(params.back().get());
  if ( ident == "readln" ) return ReadLn::call((Var*)params.back().get());
  if ( ident == "write" ) return Write::call((String*)params.back().get());
  if ( ident == "dec" ) return Dec::call((Var*)params.back().get());
  if ( ident == "exit" ) return Exit::call();

  std::vector<Value*> args;
  for ( auto & expr : params )
      args.push_back(expr->Translate());
  return Builder->CreateCall((Function*)s.val, args);
}

void Call::Print()
{

}

Function * WriteLn::f;
Constant * WriteLn::fmt;

Value *WriteLn::call(Expr * e)
{
  vector<Value*> args = {fmt, e->Translate()};
  return Builder->CreateCall(f, args);
}

void WriteLn::declare()
{
  /* f */
  vector<Type*> printf_arg_types;
  printf_arg_types.push_back(Type::getInt8PtrTy(*TheContext));

  FunctionType* printf_type =
      FunctionType::get(
        Type::getInt32Ty(*TheContext), printf_arg_types, true);

  f = Function::Create(
        printf_type, Function::ExternalLinkage,
        Twine("printf"),
        TheModule
        );
  f->setCallingConv(CallingConv::C);

  /* symbol */
  symbolTable->declCallable(false, "writeln", new CallableObj(1,false), f);

  /* fmt */
  Constant *format_const =
      ConstantDataArray::getString(*TheContext, "%d\n");
  GlobalVariable * var =
      new GlobalVariable(
        *TheModule, ArrayType::get(IntegerType::get(*TheContext, 8), 4),
        true, GlobalValue::PrivateLinkage, format_const, ".str");
  Constant *zero =
      Constant::getNullValue(IntegerType::getInt32Ty(*TheContext));
  std::vector<Constant*> indices;
  indices.push_back(zero);
  indices.push_back(zero);
  fmt = ConstantExpr::getGetElementPtr(var, indices);
}

Function * ReadLn::f;
Constant * ReadLn::fmt;

Value *ReadLn::call(Var *v)
{
  vector<Value*> args = {fmt, v->Pointer()};
  symbolTable->ensureNotConst(v->getName());
  Object::Type type = symbolTable->get(v->getName()).obj->getType();
  if ( type != Object::Integer &&
       type != Object::Array )
    error(v->getName() + " is not assignable");
  return Builder->CreateCall(f, args);
}

void ReadLn::declare()
{
  /* f */
  vector<Type*> scanf_arg_types;
  scanf_arg_types.push_back(Type::getInt8PtrTy(*TheContext));

  FunctionType* scanf_type =
      FunctionType::get(
        Type::getInt32Ty(*TheContext), scanf_arg_types, true);

  f = Function::Create(
        scanf_type, Function::ExternalLinkage,
        Twine("scanf"),
        TheModule
        );
  f->setCallingConv(CallingConv::C);

  /* symbol */
  symbolTable->declCallable(false, "readln", new CallableObj(1,false), f);

  /* fmt */
  Constant *format_const =
      ConstantDataArray::getString(*TheContext, "%d");
  GlobalVariable *var =
      new GlobalVariable(
        *TheModule, ArrayType::get(IntegerType::get(*TheContext, 8), 3),
        true, GlobalValue::PrivateLinkage, format_const, ".str");

  Constant *zero =
      Constant::getNullValue(IntegerType::getInt32Ty(*TheContext));
  std::vector<Constant*> indices;
  indices.push_back(zero);
  indices.push_back(zero);
  fmt = ConstantExpr::getGetElementPtr(var, indices);
}

String::String(const string &value)
  :value(value)
{

}

Value *String::Translate()
{
  Constant *format_const =
      ConstantDataArray::getString(*TheContext, value.c_str());
  GlobalVariable *var =
      new GlobalVariable(
        *TheModule, ArrayType::get(IntegerType::get(*TheContext, 8), value.size()+1),
        true, GlobalValue::PrivateLinkage, format_const, ".str");
  Constant *zero =
      Constant::getNullValue(IntegerType::getInt32Ty(*TheContext));
  std::vector<Constant*> indices;
  indices.push_back(zero);
  indices.push_back(zero);
  return ConstantExpr::getGetElementPtr(var, indices);
}

void String::Print()
{
  //todo
}

Constant * Write::fmt;

Value *Write::call(String *s)
{
  vector<Value*> args = {fmt, s->Translate()};
  return Builder->CreateCall(WriteLn::f, args);
}

void Write::declare()
{
  /* symbol */
  symbolTable->declCallable(false, "write", new CallableObj(1,true), WriteLn::f);

  /* fmt */
  Constant *format_const =
      ConstantDataArray::getString(*TheContext, "%s");
  GlobalVariable * var =
      new GlobalVariable(
        *TheModule, ArrayType::get(IntegerType::get(*TheContext, 8), 3),
        true, GlobalValue::PrivateLinkage, format_const, ".str");
  Constant *zero =
      Constant::getNullValue(IntegerType::getInt32Ty(*TheContext));
  std::vector<Constant*> indices;
  indices.push_back(zero);
  indices.push_back(zero);
  fmt = ConstantExpr::getGetElementPtr(var, indices);
}

Value *Dec::call(Var *v)
{
  if ( symbolTable->get(v->getName()).obj->getType() != Object::Integer )
    error(v->getName() + " is not a variable that can be decremented");
  symbolTable->ensureNotConst(v->getName());
  return Assign(new Var(*v), new Bop(Token::MINUS, new Var(*v), new Numb(1))).Translate();
}

void Dec::declare()
{
  symbolTable->declCallable(false, "dec", new CallableObj(1,true), nullptr);
}

Function * Exit::f;

Value *Exit::call()
{
  // ignore all following statements and insert Ret instr
  string fIdent = symbolTable->getFIdent();
  string retIdent = fIdent + "_return";
  const SymbolTable::Symbol & s = symbolTable->get(fIdent);
  bool returnVoid = ((CallableObj*)(s.obj.get()))->returnsVoid();
  if ( returnVoid ) Builder->CreateRetVoid();
  else Builder->CreateRet(Var(retIdent).Translate());

  foundExit = true;
  return nullptr;
}

void Exit::declare()
{
  symbolTable->declCallable(false, "exit", new CallableObj(0,true), f);
}

ArrayElement::ArrayElement(std::string ident, Expr *index)
  :Var(ident),
   index(index)
{
}

Value *ArrayElement::Translate()
{
  return Builder->CreateLoad(Pointer());
}

Value *ArrayElement::Pointer()
{
  string ident = getName();
  symbolTable->ensureDeclared(ident);
  const SymbolTable::Symbol & s = symbolTable->get(ident);
  if ( s.obj->getType() != Object::Array )
    error(ident + " is not an array");

  Array * arr = ((Array*)s.obj.get());
  Value * ret = arr->getElementPtr(s.val, index.get());
  return ret;
}

void ArrayElement::Print()
{
  // todo
}
