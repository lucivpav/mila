#ifndef PARSER_H
#define PARSER_H

#include <fstream>

#include "ast.h"
#include "lexer.h"
#include "symtab.h"

class Parser {
private:
  std::ifstream mInputStream;
  Lexer mLexer;
  SymbolTable mSymbolTable;
public:
  Parser(const char * file,
         LLVMContext & context, Module & module, IRBuilder<> & builder);

  StatmList * getStatements();
private:
  void CompareError(Token::Type s);
  void CompareError(Token::Type expect, Token::Type get);
  void ExpansionError(const char* nonterminal, Token::Type s);
  void Compare(Token::Type s, bool noNext = false);
  void Compare(Token::Type expect, Token::Type get);
  void Compare_IDENT(std::string *id);
  void Compare_NUMB(int *h);

  /* body statements of a callable consist of two parts:
   * 1) decl statements - declarations of variables/constants (and callables if main)
   * 2) action statements - assignments, loops, callables being called
   */
  StatmList * BodyStatements(bool main = false);

  StatmList * DeclStatements(StatmList *& last, bool main, bool firstStatm = false);
  Statm * MainDeclStatement(bool firstStatm = false);
  Statm * DeclStatement();

  StatmList * ActionStatements();
  Statm * ActionStatement(Loop * parentLoop = 0);

  /* statements */

  StatmList * BlockStatements(Loop * parentLoop = 0);
  StatmList * BlockStatementsImpl(Loop * parentLoop = 0);
  Statm * IfStatement(Loop * parentLoop = 0);
  Statm * ElseStatement(Loop * parentLoop = 0);
  Statm * WhileStatement();
  Statm * ForStatement();
  Statm * ProgramStatement();
  Call * CallStatement(const string & ident, bool noParams);
  Call * WriteStatement();

  Statm *SimpleStatement(Loop * parentLoop = 0); // wrapper to handle SEMICOLON
  Statm *SimpleStatementImpl(Loop * parentLoop = 0);

  Statm *AssignStatement(); // integer or array element assign
  Assign * IntegerAssignStatement(std::string ident);
  Assign * ArrayAssignStatement(std::string ident);

  Expr *Expression(bool inBoolExpr = false);
  Expr *ExpressionPrimed(Expr *du, bool inBoolExpr);
  Expr *Term(bool inBoolExpr);
  Expr *TermPrimed(Expr *du, bool inBoolExpr);
  Expr *Factor(bool inBoolExpr);

  Expr * AssignableExpression();

  Expr *BoolExpression(); // todo?
  Expr *BoolExpressionPrimed(Expr *du);
  Expr *BoolTerm();
  Expr *BoolTermPrimed(Expr *du);
  Expr *BoolFactor();
  Expr *BoolRelation(Expr *du);

  /* decl var */
  StatmList *DeclVarStatement(bool ensureTailDelim = true);
  StatmList *DeclVarStatementImpl(bool optional, bool ensureTailDelim);
  Object * DataTypeExpression(bool expectOrdinary = false);
  Decl * VariableList();

  /* decl const */
  StatmList *DeclConstStatement();
  StatmList *DeclConstStatementImpl(bool optional);

  /* decl callable */
  Statm * DeclCallableStatement(Token::Type type);
};

#endif // PARSER_H
