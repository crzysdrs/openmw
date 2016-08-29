/* $Id: parser.yy 48 2009-09-05 08:07:10Z tb $ -*- mode: c++ -*- */
/** \file parser.yy Contains the example Bison parser source */

%code requires { /*** C/C++ Declarations ***/

#include <stdio.h>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <components/compiler/ast.hpp>
#include <components/misc/stringops.hpp>

typedef boost::shared_ptr<AST::Expression> shared_expr;
typedef boost::shared_ptr<AST::Statement> shared_stmt;

}

/*** yacc/bison Declarations ***/

/* Require bison 2.3 or later */
%require "2.3"

/* add debug output code to generated parser. disable this for release
 * versions. */
%debug

/* start symbol is named "start" */
%start start

/* write out a header file containing the token defines */
%defines

/* use newer C++ skeleton file */
%skeleton "lalr1.cc"

/* namespace to enclose parser in */
%name-prefix "Compiler"

//%glr-parser

/* set the parser's class identifier */
%define "parser_class_name" {NewParser}

/* keep track of the current position within the input */
%locations
%initial-action
{
    // initialize the initial location object
    @$.begin.filename = @$.end.filename = &driver.streamname;
};

/* The driver is passed by reference to the parser and to the scanner. This
 * provides a simple but effective pure interface, not relying on global
 * variables. */
%parse-param { class Driver& driver }

/* verbose error messages */
%error-verbose

 /*** BEGIN EXAMPLE - Change the example grammar's tokens below ***/

%union {
    int             shortVal;
    int  			longVal;
    float 			floatVal;
    std::string * stringLitVal;
    boost::shared_ptr<std::string>*	stringVal;
    boost::shared_ptr<AST::IfStatement>*  ifVal;
    boost::shared_ptr<AST::Module>*  moduleVal;
    boost::shared_ptr<AST::StringLit>*  strExprVal;
    boost::shared_ptr<AST::BinExpr>*  binExprVal;
    shared_expr* exprVal;
    shared_stmt* stmtVal;
    AST::Type       typeVal;
    AST::BinOp       opVal;
    std::vector<shared_expr>* exprList;
    std::vector<shared_stmt>* stmtList;
}

%token          UNDEFINED
%token			EOF_T     0	"end of file"
%token			EOL		"end of line"
%token          IF
%token          ELSE
%token          ELSEIF
%token          ENDIF
%token          WHILE
%token          ENDWHILE
%token          SET
%token          TO
%token          ARROW
%token          RETURN
%token          FLOAT
%token          SHORT
%token          LONG
%token          BEGIN_BLOCK
%token          END_BLOCK
%token          EQ
%token          NEQ
%token          LT
%token          LTE
%token          GT
%token          GTE
%token          DOT
%token          COMMA

%left LOW
%left EQ NEQ LTE GTE LT GT
%left '+' '-'
%right '*' '/'
%nonassoc UMINUS
%nonassoc DOT ARROW
%left HIGH
%left error
%nonassoc IDENT STRING_LIT
%left ELSEIF ELSE ENDIF

%token <shortVal>   SHORT_LIT	"short"
%token <longVal>    LONG_LIT	"long"
%token <floatVal> 	FLOAT_LIT	"float"
%token <stringVal> 	STRING_LIT	"string"
%token <stringVal> 	IDENT		"ident"


%type <opVal> bool_oper arrow_dot
%type <stringVal> string_ident
%type <typeVal> type
%type <binExprVal> bin_expr
%type <strExprVal> string_expr
%type <exprVal> expr singleton fn_call ref simple_expr  paren_expr
%type <stmtVal> statement inner_statement  control_flow set_statement while_statement if_statement conditional_statement line_statement inner_line_statement type_decl
%type <ifVal> elseif_statement elseifs
%type <stmtList> statement_list inner_statement_list optional_else else_statement branches
%type <exprList> arg_list
%type <moduleVal> start block

/*
%destructor { } <shortVal>
%destructor { } <longVal>
%destructor { } <floatVal>
%destructor { } <typeVal>
%destructor { } <opVal>
%destructor { delete ($$); } <*>
*/

 /*** END EXAMPLE - Change the example grammar's tokens above ***/

%{

#include <components/compiler/driver.hpp>
#include <components/compiler/newscanner.hpp>
#include <components/compiler/ast.hpp>

/* this "connects" the bison parser in the driver to the flex scanner class
 * object. it defines the yylex() function call to pull the next token from the
 * current lexer object of the driver context. */
#undef yylex
#define yylex driver.lexer->lex

%}

%% /*** Grammar Rules ***/

 /*** BEGIN EXAMPLE - Change the example grammar rules below ***/

statement_list : statement_list line_statement { $1->push_back(*$2); $$ = $1; }
| %empty { $$ = new std::vector<shared_stmt>(); }
;

inner_statement_list : inner_statement_list inner_line_statement { $1->push_back(*$2); $$ = $1; }
| %empty { $$ = new std::vector<shared_stmt>(); }
;

eol : EOL
| eol EOL
;

maybe_eol : eol
| %empty
;

line_statement : statement eol {$$ = $1;}
| error eol {
  driver.reportDeferredAsWarning();
  driver.warning(@1, "Discarded invalid statement");
  $$ = new shared_stmt(new AST::NoOp(driver.tokenLoc(@1))); }
;

inner_line_statement : inner_statement eol {$$ = $1;}
| error eol {
  driver.reportDeferredAsWarning();
  driver.warning(@1, "Discarded invalid statement");
  $$ = new shared_stmt(new AST::NoOp(driver.tokenLoc(@1))); }
;


inner_statement:
 type_decl
| control_flow
| set_statement
| expr  { $$ = new shared_stmt(new AST::StatementExpr(driver.tokenLoc(@1), *$1)); }
;

statement :
inner_statement
| else_statement endif {
  driver.warning(@1, "Unexpected Else Statement. Discarding all sub statements.");
  $$ = new shared_stmt(new AST::NoOp(driver.tokenLoc(@1)));
}
| elseif_statement branches endif {
  driver.warning(@1, "Unexpected Else If Statement. Treating as an if statement.");
  (*$1)->setElse(*$2);
  $$ = new shared_stmt(boost::static_pointer_cast<AST::Statement>(*$1));

}
| endif {
  driver.warning(@1, "Unexpected End If Statement.");
  $$ = new shared_stmt(new AST::NoOp(driver.tokenLoc(@1)));
  }
;

set_statement :
  SET ref string_ident expr {
    std::string to("to");
    if (::Misc::StringUtils::lowerCase(**$3) != to) {
      error(@3, "Must use TO in SET statement.");
    }
    $$ = new shared_stmt(new AST::SetStatement(driver.tokenLoc(@1), *$2, *$4));
  }
;

type_decl :
type string_ident { $$ = new shared_stmt(new AST::TypeDecl(driver.tokenLoc(@1), $1, *$2)); }
;

type :
  SHORT {$$ = AST::SHORT;}
| LONG  {$$ = AST::LONG; }
| FLOAT {$$ = AST::FLOAT;}
;

string_ident:
 STRING_LIT
 | IDENT {$$ = $1;}
;

optional_idents :
optional_idents string_ident
| %empty;
;

string_expr :
string_ident { $$ = new  boost::shared_ptr<AST::StringLit>(new AST::StringLit(driver.tokenLoc(@1), **$1)); }
;

singleton :
LONG_LIT { $$ = new shared_expr(new AST::LongLit(driver.tokenLoc(@1), $1));}
| SHORT_LIT { $$ = new shared_expr(new AST::LongLit(driver.tokenLoc(@1), $1)); }
| FLOAT_LIT { $$ = new shared_expr(new AST::FloatLit(driver.tokenLoc(@1), $1)); }
/* | expr_ref {$$ = $1;}; */
;

paren_expr :
'(' expr ')' {$$ = $2;}
| string_expr arrow_dot '(' expr ')' {
  /* hi there, at this point you have noticed this doesn't look like a paren expr
     but unfortunately it acts just like one. Reference exprs can contain an arbitrarily
     placed open paren right after the arrow or dot, which then requires a transformation
     to get it back in the right "shape" */
  boost::shared_ptr<AST::StringLit> left_e = *$1;
  shared_expr right_e = *$4;
  boost::shared_ptr<AST::BinExpr> bin;
  boost::shared_ptr<AST::ExprItems> items;
  if ((bin = boost::dynamic_pointer_cast<AST::BinExpr>(right_e))) {
    std::string s;
    if (bin->getLeft()->coerceString(s)) {
      boost::shared_ptr<AST::StringLit> offset_str(new AST::StringLit(driver.tokenLoc(@1), s));
      shared_expr real_ref(new AST::RefExpr(driver.tokenLoc(@1), $2, left_e, offset_str));
      std::vector<shared_expr> itemvec;
      itemvec.push_back(real_ref);
      boost::shared_ptr<AST::Expression> items(new AST::ExprItems(driver.tokenLoc(@1), itemvec));
      bin->setLeft(items);
      $$ = new shared_expr(boost::static_pointer_cast<AST::Expression>(bin));
    }
  } else if ((items = boost::dynamic_pointer_cast<AST::ExprItems>(right_e))) {
    std::string s;
    if (items->getItems()[0]->coerceString(s)) {
      boost::shared_ptr<AST::StringLit> offset_str(new AST::StringLit(driver.tokenLoc(@1), s));
      shared_expr real_ref(new AST::RefExpr(driver.tokenLoc(@1), $2, left_e, offset_str));
      items->getItems()[0] = real_ref;
      $$ = new shared_expr(boost::static_pointer_cast<AST::Expression>(items));
    }
  } else {
    /* There is a possibilty that other expressions types could exist here, but it's not worth handling the rest. */
    error(@3, "Syntax error: Unexpected '('.");
  }
}
;

arrow_dot :
ARROW { $$ = AST::ARROW; }
| DOT { $$ = AST::DOT; }
;

ref :
string_expr %prec LOW { $$ = new shared_expr(new AST::RefExpr(driver.tokenLoc(@1), *$1)) ; }
| DOT string_expr { std::string empty_str("");
  boost::shared_ptr<AST::StringLit> empty(new AST::StringLit(driver.tokenLoc(@1),empty_str));
  $$ = new shared_expr(new AST::RefExpr(driver.tokenLoc(@1), AST::DOT, empty, *$2)) ; }
| string_expr arrow_dot string_expr { $$ = new shared_expr(new AST::RefExpr(driver.tokenLoc(@1), $2, *$1, *$3)) ; }
;

bool_oper :
  EQ  { $$ = AST::EQ; }
| NEQ { $$ = AST::NEQ; }
| LTE { $$ = AST::LTE; }
| GTE { $$ = AST::GTE; }
| LT  { $$ = AST::LT; }
| GT  { $$ = AST::GT; }
;

bin_expr :
expr '+' expr %prec '+' { $$ = new boost::shared_ptr<AST::BinExpr>(new AST::MathExpr(driver.tokenLoc(@1), AST::PLUS, *$1, *$3)); }
| expr '-' expr %prec '-' { $$ = new boost::shared_ptr<AST::BinExpr>(new AST::MathExpr(driver.tokenLoc(@1), AST::MINUS, *$1, *$3)); }
| expr '*' expr %prec '*' { $$ = new boost::shared_ptr<AST::BinExpr>(new AST::MathExpr(driver.tokenLoc(@1), AST::MULT, *$1, *$3)); }
| expr '/' expr %prec '/' { $$ = new boost::shared_ptr<AST::BinExpr>(new AST::MathExpr(driver.tokenLoc(@1), AST::DIVIDE, *$1, *$3)); }
| expr bool_oper expr %prec EQ { $$ = new boost::shared_ptr<AST::BinExpr>(new AST::LogicExpr(driver.tokenLoc(@1), $2, *$1, *$3)); }
;

expr : paren_expr
| bin_expr { $$ = new shared_expr(boost::static_pointer_cast<AST::Expression>(*$1)); }
| '-' expr %prec UMINUS { $$ = new shared_expr(new AST::NegateExpr(driver.tokenLoc(@1), *$2)); }
| fn_call { $$ = $1; } %prec HIGH
| singleton {$$ = $1; }
;

else_if : ELSEIF
;

elseif_statement :
else_if expr eol statement_list %prec ENDIF { $$ = new boost::shared_ptr<AST::IfStatement>(new AST::IfStatement(driver.tokenLoc(@1), *$2, *$4)); }
;

elseifs :
elseif_statement elseifs {
    std::vector<shared_stmt> * l = new std::vector<shared_stmt>();
    l->push_back(*$2);
    (*$1)->setElse(*l);
    $$ = $1;
}
| elseif_statement optional_else {
    (*$1)->setElse(*$2);
    $$ = $1;
}
;

branches :
elseifs {
  $$ = new std::vector<shared_stmt>();
  $$->push_back(boost::static_pointer_cast<AST::Statement>(*$1));
}
| optional_else
;


else_statement : ELSE eol statement_list { $$ = $3; }
| ELSE expr eol statement_list { $$ = $4; /* should I treat an else with a condition like an else if? */}
;

optional_else : else_statement %prec ENDIF
| %empty { $$ = new std::vector<shared_stmt>(); }
;

endif : ENDIF
;

conditional_statement:
 if_statement endif
;

if_statement :
IF expr eol inner_statement_list branches {
  $$ = new shared_stmt(new AST::IfStatement(driver.tokenLoc(@1), *$2, *$4, *$5));
}
;

;
while_statement :
WHILE expr eol statement_list ENDWHILE { $$ = new shared_stmt(new AST::WhileStatement(driver.tokenLoc(@1), *$2, *$4)); }
;


simple_expr :
 '-' ref %prec '-'  { $$ = new shared_expr(new AST::NegateExpr(driver.tokenLoc(@1), *$2)); }
| ref
| paren_expr
;

arg_list :
arg_list simple_expr optional_comma { $1->push_back(*$2); $$ = $1; }
| optional_comma { $$  = new std::vector<shared_expr>(); }
;

optional_comma :
COMMA
| %empty;

fn_call :
ref arg_list %prec LOW {
  $2->insert($2->begin(), *$1);
  $$ = new shared_expr(new AST::ExprItems(driver.tokenLoc(@1), *$2));
}
| ref arg_list error %prec LOW {
  driver.reportDeferredAsWarning();
  driver.warning(@3, "Junk at end of line discarded");
  $2->insert($2->begin(), *$1);
  $$ = new shared_expr(new AST::ExprItems(driver.tokenLoc(@1), *$2)); }
;

control_flow : conditional_statement
| while_statement
| RETURN { $$ = new shared_stmt(new AST::ReturnStatement(driver.tokenLoc(@1))); }
;

after_block :
optional_idents maybe_eol {}
;

block :
maybe_eol BEGIN_BLOCK string_ident eol statement_list END_BLOCK after_block  {
    $$ = new boost::shared_ptr<AST::Module>(new AST::Module(driver.tokenLoc(@1), *$3, *$5));
    //printf("%s parsed\n", (*$4)->c_str());
    driver.setResult(*$$);
}

start : block
| block error EOF_T {
  driver.resetDeferred();
  driver.warning(@2, "Extra script data outside after END statement.");
}

 /*** END EXAMPLE - Change the example grammar rules above ***/

%% /*** Additional Code ***/

void Compiler::NewParser::error(const NewParser::location_type& l,
			    const std::string& m)
{
  driver.deferredError(l, m);
}
