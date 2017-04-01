#include <cstdio>
#include <string>
#include <cstdlib>
#include <fstream>

#include "ast.h"
#include "parser.h"
#include "symtab.h"
#include "util.h"

Token Symb;

void Parser::CompareError(Token::Type s) {
  CompareError(s, Symb.type);
}

void Parser::CompareError(Token::Type expect, Token::Type get)
{
  error(std::string("excpected ") + Token::TypeStr[expect]
        + ", got " + Token::TypeStr[get]);
}

void Parser::ExpansionError(const char* nonterminal, Token::Type s) {
  // todo
  error(std::string("expanding nonterminal ") + nonterminal
        + ", expected " + Token::TypeStr[s]
        + ", got " + Token::TypeStr[Symb.type]);
}

void Parser::Compare(Token::Type s, bool noNext) {
  if (Symb.type == s) {
    if ( !noNext )
      Symb = mLexer.nextToken();
  } else
    CompareError(s);
}

void Parser::Compare(Token::Type expect, Token::Type get)
{
   if (expect != get)
     CompareError(expect, get);
}

void Parser::Compare_IDENT(std::string *id)
{
   if (Symb.type == Token::IDENT) {
      *id = Symb.ident;
      Symb = mLexer.nextToken();
   } else
      CompareError(Token::IDENT);
}

void Parser::Compare_NUMB(int *h)
{
   if (Symb.type == Token::NUMB) {
      *h = Symb.number;
      Symb = mLexer.nextToken();
   } else
     CompareError(Token::NUMB);
}

StatmList *Parser::BodyStatements(bool main)
{
  StatmList * initTail;
  StatmList * init = DeclStatements(initTail, main, true);
  Compare(Token::kwBEGIN);

  StatmList * next = ActionStatements();
  if ( !next ) next = new StatmList(new Program("no-op") ,0);
  if ( main ) {
    Statm * declMain = new DeclCallable("main", 0, new Integer(), next);
    next = new StatmList(declMain, 0);
  }

  Compare(Token::kwEND);
  Compare(main ? Token::DOT : Token::SEMICOLON);

  if ( !init ) return next;
  StatmList::merge(initTail, next);
  return init;
}

StatmList *Parser::DeclStatements(StatmList *&last, bool main,
                                      bool firstStatm)
{
  Statm * p;
  if ( main ) p = MainDeclStatement(firstStatm);
  else p = DeclStatement();
  if ( !p ) return nullptr;
  StatmList * next = DeclStatements(last, main);
  StatmList *su = new StatmList(p, next);
  if ( !next ) last = su;
  return su;
}

StatmList *Parser::ActionStatements()
{
  Statm *p = ActionStatement();
  if ( !p ) return nullptr;
  return new StatmList(p, ActionStatements());
}

Statm * Parser::MainDeclStatement(bool firstStatm) {
  switch (Symb.type) {
  case Token::kwPROGRAM: if ( firstStatm ) return ProgramStatement();
  case Token::kwFUNCTION:
  case Token::kwPROCEDURE: return DeclCallableStatement(Symb.type);
  default: return DeclStatement();
  }
}

Statm * Parser::DeclStatement() {
  switch (Symb.type) {
  case Token::kwVAR: return DeclVarStatement();
  case Token::kwCONST: return DeclConstStatement();
  default: return nullptr;
  }
}

Statm * Parser::ActionStatement(Loop * parentLoop) {
  switch (Symb.type) {
  case Token::kwIF: return IfStatement(parentLoop);
  case Token::kwWHILE: return WhileStatement();
  case Token::kwFOR: return ForStatement();
  case Token::kwBEGIN: return BlockStatements(parentLoop);
  default: return SimpleStatement(parentLoop);
  }
}

StatmList *Parser::BlockStatements(Loop *parentLoop)
{
  Symb = mLexer.nextToken();
  return BlockStatementsImpl(parentLoop);
}

StatmList *Parser::BlockStatementsImpl(Loop *parentLoop)
{
  if( Symb.type == Token::kwEND ) {
    Symb = mLexer.nextToken();
    if ( Symb.type == Token::kwEND ) return nullptr;
    Compare(Token::SEMICOLON);
    return nullptr;
  }

  Statm *p = ActionStatement(parentLoop);
  return new StatmList(p, BlockStatementsImpl(parentLoop));
}

Statm *Parser::IfStatement(Loop *parentLoop)
{
  Symb = mLexer.nextToken();
  Expr * e = BoolExpression();
  Compare(Token::kwTHEN);
  // ; may not be present after If's action stmt
  Statm * s = ActionStatement(parentLoop);
  return new If(e, s, ElseStatement(parentLoop));
}

Statm *Parser::ElseStatement(Loop *parentLoop)
{
  switch (Symb.type) {
  case Token::kwELSE: {
    Symb = mLexer.nextToken();
    return ActionStatement(parentLoop);
  }
  default:
    return nullptr;
  }
}

Statm *Parser::WhileStatement()
{
  Symb = mLexer.nextToken();
  Expr * e = BoolExpression();
  Compare(Token::kwDO);
  While * whileStmt = new While();
  whileStmt->init(e, ActionStatement(whileStmt));
  return whileStmt;
}

Statm *Parser::ForStatement()
{
  Symb = mLexer.nextToken();
  if ( Symb.type != Token::IDENT )
    error("expected valid for-loop init expression");
  string ident = Symb.ident;
  Symb = mLexer.nextToken();
  Compare(Token::ASSIGN);
  Assign * initStmt = IntegerAssignStatement(ident);
  bool downto;
  if ( Symb.type == Token::kwTO ) downto = false;
  else if ( Symb.type == Token::kwDOWNTO ) downto = true;
  else error("expected 'to' or 'downto'");
  Symb = mLexer.nextToken();
  Expr * limitExpr = Expression();
  Compare(Token::kwDO);
  For * forStmt = new For();
  forStmt->init(initStmt, downto, limitExpr, ActionStatement(forStmt));
  return forStmt;
}

Statm *Parser::ProgramStatement()
{
  Symb = mLexer.nextToken();
  string id;
  Compare_IDENT(&id);
  Compare(Token::SEMICOLON);
  return new Program(id);
}

Call *Parser::CallStatement(const string & ident, bool noParams)
{
  if ( ident == "write" ) return WriteStatement();
  bool requireAssignable = false;
  if ( ident == "readln" || ident == "dec" ) requireAssignable = true;
  vector<Expr*> params;
  if ( noParams ) return new Call(ident, params);

  Symb = mLexer.nextToken();
  unsigned cnt;
  for ( cnt = 0; Symb.type != Token::RPAR ; ++cnt ) {
    if ( cnt ) Compare(Token::COMMA);
    params.push_back( requireAssignable ?
                        AssignableExpression() : Expression() );
  }
  Compare(Token::RPAR);
  if ( !cnt ) error("calls without arguemts should be without ()");
  return new Call(ident, params);
}

Call * Parser::WriteStatement() {
  vector<Expr*> params;
  string str;
  Symb = mLexer.nextToken();
  Compare(Token::APOSTROPHE, true);
  mLexer.readString(str);
  Symb = mLexer.nextToken();
  Compare(Token::APOSTROPHE);
  Compare(Token::RPAR);
  params.push_back(new String(str));
  return new Call("write", params);
}

Statm *Parser::SimpleStatement(Loop * parentLoop)
{
  Statm * res = SimpleStatementImpl(parentLoop);
  if ( !res ) return nullptr;

  // ; not provided, as it is the last stmt before kwEND or kwELSE
  if ( Symb.type == Token::kwEND ||
       Symb.type == Token::kwELSE ) return res;

  if ( res ) Compare(Token::SEMICOLON);
  return res;
}

Statm *Parser::SimpleStatementImpl(Loop *parentLoop)
{
  string id;
  switch (Symb.type) {

  case Token::IDENT: // assign to var
    return AssignStatement();

  case Token::kwBREAK:{
    Symb = mLexer.nextToken();
    if ( !parentLoop )
      error("no loop to break");
    return new Break(*parentLoop);
  }
  default: return nullptr;
  }
}

Statm *Parser::AssignStatement() // todo: rename
{
  string ident = Symb.ident;
  Symb = mLexer.nextToken();
  switch (Symb.type) {
  case Token::ASSIGN:
    Symb = mLexer.nextToken();
    return IntegerAssignStatement(ident);
  case Token::LBR: return ArrayAssignStatement(ident);
  case Token::LPAR: return CallStatement(ident, false);
  case Token::SEMICOLON:
  case Token::kwEND: return CallStatement(ident, true);
  default: error("invalid assignment");
  }
  assert ( false );
}

Assign *Parser::IntegerAssignStatement(std::string ident)
{
  return new Assign(new Var(ident), Expression());
}

Assign *Parser::ArrayAssignStatement(std::string ident)
{
  Symb = mLexer.nextToken();
  Expr * index = Expression();
  Compare(Token::RBR);
  Compare(Token::ASSIGN);
  return new Assign(new ArrayElement(ident, index), Expression());
}

Expr *Parser::Expression(bool inBoolExpr)
{
   if (Symb.type == Token::MINUS) {
      Symb = mLexer.nextToken();
      return ExpressionPrimed(new UnMinus(Term(inBoolExpr)), inBoolExpr);
   }
   return ExpressionPrimed(Term(inBoolExpr), inBoolExpr);
}

Expr *Parser::ExpressionPrimed(Expr *du, bool inBoolExpr)
{
   switch (Symb.type) {
   case Token::PLUS:
      Symb = mLexer.nextToken();
      return ExpressionPrimed(new Bop(Token::PLUS, du, Term(inBoolExpr)), inBoolExpr);
   case Token::MINUS:
      Symb = mLexer.nextToken();
      return ExpressionPrimed(new Bop(Token::MINUS, du, Term(inBoolExpr)), inBoolExpr);
   default:
      return du;
   }
}

Expr *Parser::Term(bool inBoolExpr)
{
   return TermPrimed(Factor(inBoolExpr), inBoolExpr);
}

Expr *Parser::TermPrimed(Expr *du, bool inBoolExpr)
{
  // why TermPrimed and ExpressionPrimed?
  Token::Type type = Symb.type;
  switch (Symb.type) {
  case Token::TIMES:
  case Token::kwDIV:
  case Token::kwMOD:
    Symb = mLexer.nextToken();
    return TermPrimed(new Bop(type, du, Factor(inBoolExpr)), inBoolExpr);
  default:
    return du;
  }
}

Expr *Parser::Factor(bool inBoolExpr)
{
   switch (Symb.type) {
   case Token::IDENT: {
      std::string id;
      Compare_IDENT(&id);
      if ( Symb.type == Token::LBR ) {
        Symb = mLexer.nextToken();
        Expr * index = Expression();
        Compare(Token::RBR);
        return new ArrayElement(id, index);
      } else if ( Symb.type == Token::LPAR )
        return CallStatement(id, false);
      else return new Var(id); // either var or callable

   }
   case Token::NUMB: {
      int hodn;
      Compare_NUMB(&hodn);
      return new Numb(hodn);
      }
   case Token::LPAR: {
      Symb = mLexer.nextToken();
      Expr *su = inBoolExpr ? BoolExpression() : Expression();
      Compare(Token::RPAR);
      return su;
   }
   default:
      ExpansionError("Factor", Symb.type);
      return 0;
   }
}

Expr *Parser::AssignableExpression() // either var or arr[idx]
{
  string ident;
  Compare_IDENT(&ident);
  switch (Symb.type) {
  case Token::LBR: {
    Symb = mLexer.nextToken();
    Expr * index = Expression();
    Compare(Token::RBR);
    return new ArrayElement(ident, index);
  }
  default:
    return new Var(ident);
  }
  assert ( false );
}

Expr *Parser::BoolExpression()
{
  Expr * e = BoolExpressionPrimed(BoolTerm());
  return e;
}

Expr *Parser::BoolExpressionPrimed(Expr *du)
{
  switch (Symb.type) {
   case Token::kwOR:
      Symb = mLexer.nextToken();
      return BoolExpressionPrimed(new Bop(Token::kwOR, du, BoolTerm()));
   default:
      return du;
   }
}

Expr *Parser::BoolTerm()
{
  return BoolTermPrimed(BoolFactor());
}

Expr *Parser::BoolTermPrimed(Expr *du)
{
  switch (Symb.type) {
  case Token::kwAND:
    Symb = mLexer.nextToken();
    return BoolTermPrimed(new Bop(Token::kwAND, du, BoolFactor()));
  default:
    return du;
  }
}

Expr *Parser::BoolFactor()
{
  switch (Symb.type) {
  case Token::kwNOT:
    Symb = mLexer.nextToken();
    return new Not(BoolFactor());
  default:
    return BoolRelation(Expression(true));
  }
}

Expr *Parser::BoolRelation(Expr *du)
{
  Token::Type type = Symb.type;
  switch (Symb.type) {
  case Token::EQ:
  case Token::NEQ:
  case Token::LT:
  case Token::LTE:
  case Token::GT:
  case Token::GTE:
    Symb = mLexer.nextToken();
    return new Bop(type, du, Expression(true));
  default:
    return du;
  }
}

Decl * Parser::VariableList()
{
  string id;
  switch (Symb.type) {
  case Token::COMMA:{
      Symb = mLexer.nextToken();
      Compare_IDENT(&id);
      return new Decl(id, VariableList());
  }
  default:
    return 0;
  }
}

StatmList *Parser::DeclVarStatement(bool ensureTailDelim)
{
  Symb = mLexer.nextToken();
  return DeclVarStatementImpl(false, ensureTailDelim);
}

StatmList *Parser::DeclVarStatementImpl(bool optional, bool ensureTailDelim)
{ // todo: simplify
  string id;
  Token backup = Symb;

  if ( optional &&
       !ensureTailDelim &&
       Symb.type != Token::SEMICOLON )
    return nullptr;

  if ( optional ) Symb = mLexer.nextToken();

  if ( optional && Symb.type != Token::IDENT ) {
    if ( ensureTailDelim ) Compare(Token::SEMICOLON, backup.type);
    return nullptr;
  }
  else Compare_IDENT(&id);

  if ( optional ) Compare(Token::SEMICOLON, backup.type);

  Decl * list = VariableList();

  if ( optional && !list && Symb.type != Token::COLON ) {
    mLexer.returnToken(Symb);
    mLexer.returnToken(backup);
    Symb = mLexer.nextToken();
    return nullptr;
  }

  Compare(Token::COLON);
  Object * o = DataTypeExpression();

  StatmList * next = DeclVarStatementImpl(true, ensureTailDelim);
  return new StatmList(new Decl(id, list, o), next);
}

Object * Parser::DataTypeExpression(bool expectOrdinary)
{
  Object * obj = nullptr;
  string err_str = "invalid data type";
  if ( Symb.type != Token::IDENT ) error(err_str);
  Object::Type t = Object::ident2type(Symb.ident);
  switch (t) {
  case Object::Invalid: error(err_str);
  case Object::Integer:
    obj = new Integer();
    Symb = mLexer.nextToken();
    break;
  case Object::Array:{
    if ( expectOrdinary ) error("expected ordinary type");
    Expr *from, *to;
    Symb = mLexer.nextToken();
    Compare(Token::LBR);
    from = Expression();
    Compare(Token::DOT);
    Compare(Token::DOT);
    to = Expression();
    Compare(Token::RBR);
    Compare(Token::kwOF);
    DataTypeExpression(true);
    obj = new Array(from, to);
    break;
  default:
      assert ( false );
  }
  }
  assert ( obj );
  return obj;
}

StatmList *Parser::DeclConstStatement()
{
  Symb = mLexer.nextToken();
  return DeclConstStatementImpl(false);
}

StatmList *Parser::DeclConstStatementImpl(bool optional)
{
  string id;
  Token backup = Symb;

  if ( optional && Symb.type != Token::IDENT ) return nullptr;
  else Compare_IDENT(&id);

  if ( optional && Symb.type != Token::EQ ) {
    mLexer.returnToken(Symb);
    mLexer.returnToken(backup);
    Symb = mLexer.nextToken();
    return nullptr;
  }

  Compare(Token::EQ);
  Expr * expr = Expression();
  Compare(Token::SEMICOLON);
  return new StatmList(new DeclConst(id, expr, new Integer()), DeclConstStatementImpl(true));
}

Statm *Parser::DeclCallableStatement(Token::Type type)
{
  bool procedure = (type == Token::kwPROCEDURE);
  string ident;
  StatmList * params = nullptr;
  Object * returnType = nullptr;
  StatmList * body = nullptr;

  Symb = mLexer.nextToken();
  Compare_IDENT(&ident);
  if ( Symb.type == Token::LPAR ) {
    params = DeclVarStatement(false);
    Compare(Token::RPAR);
  }
  if ( !procedure ) {
    Compare(Token::COLON);
    returnType = DataTypeExpression(true);
  }
  Compare(Token::SEMICOLON);
  if ( Symb.type == Token::kwFORWARD ) {
    Compare(Token::kwFORWARD);
    Compare(Token::SEMICOLON);
  } else {
    body = BodyStatements();
  }
  return new DeclCallable(ident, params, returnType, body);
}

Parser::Parser(const char *fileName,
               LLVMContext & context, Module & module, IRBuilder<> & builder)
  : mInputStream(fileName),
    mLexer(mInputStream),
    mSymbolTable(builder, context, module)
{
  ast_init(context, module, builder, mSymbolTable);
  Symb = mLexer.nextToken();
}

StatmList *Parser::getStatements()
{
  return BodyStatements(true);
}


