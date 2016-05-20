/* $Id: scanner.ll 44 2008-10-23 09:03:19Z tb $ -*- mode: c++ -*- */
/** \file scanner.ll Define the example Flex lexical scanner */

%{ /*** C/C++ Declarations ***/

#include <string>

#include <components/compiler/newscanner.hpp>

/* import the parser's token type into a local typedef */
typedef Compiler::NewParser::token token;
typedef Compiler::NewParser::token_type token_type;

/* By default yylex returns int, we use token_type. Unfortunately yyterminate
 * by default returns 0, which is not of token_type. */
#define yyterminate() return token::EOF_T

/* This disables inclusion of unistd.h, which is not available under Visual C++
 * on Win32. The C++ scanner uses STL streams instead. */
#define YY_NO_UNISTD_H

%}

/*** Flex Declarations and Options ***/

/* enable c++ scanner class generation */
%option c++

%pointer

/* change the name of the scanner class. results in "ExampleFlexLexer" */
%option prefix="Example"

/* the manual says "somewhat more optimized" */
%option batch

/* enable scanner to generate debug output. disable this for release
 * versions. */
%option debug

/* no support for include files is planned */
%option yywrap nounput

%option align

%option full

%option warn

%option 8bit

/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
%{
#define YY_USER_ACTION  yylloc->columns(yyleng);

%}

%s LEGACY
%s KEYWORDS
%x COMMENT

%% /*** Regular Expressions Part ***/

 /* code to place at the beginning of yylex() */
%{
    // reset location
    yylloc->step();
%}

 /*** BEGIN EXAMPLE - Change the example lexer rules below ***/

<KEYWORDS>{
    (?i:if) { BEGIN(INITIAL); return token::IF; }
    (?i:else) { BEGIN(INITIAL); return token::ELSE; }
    (?i:else[ \t]*if) { BEGIN(INITIAL); return token::ELSEIF; }
    (?i:end[ \t]*if) { BEGIN(INITIAL); return token::ENDIF; }
    (?i:while) { BEGIN(INITIAL); return token::WHILE; }
    (?i:end[ \t]*while) {  return token::ENDWHILE; }
    (?i:set) { BEGIN(INITIAL);  return token::SET; }
    (?i:to) { return token::TO; }
    (?i:return) {return token::RETURN; }
    (?i:short) { BEGIN(INITIAL); return token::SHORT; }
    (?i:float) { BEGIN(INITIAL);return token::FLOAT; }
    (?i:long) { BEGIN(INITIAL); return token::LONG; }
    (?i:begin) { BEGIN(INITIAL); return token::BEGIN_BLOCK; }
    (?i:end) { BEGIN(INITIAL); return token::END_BLOCK; }
}

";" {
    BEGIN(COMMENT);
}

"\n" {
    yylloc->lines(1);
    BEGIN(KEYWORDS);
    //printf("KEYWORD STATE\n");
    return token::EOL;
}

<COMMENT>{
    "\n" {
        yylloc->lines(1);
        BEGIN(KEYWORDS);
        //printf("KEYWORD STATE\n");
        return token::EOL;
    }
    [^\n]+ {
        yylloc->step();
    }
}

[ \t\r]+ {
    /* whitespace */
    yylloc->step();
}

==? { return token::EQ;}
!==? { return token::NEQ;}
">" { return token::GT;}
"<" { return token::LT;}
">"==? { return token::GTE;}
"<"==? { return token::LTE;}

"." { return token::DOT;}
"->" { return token::ARROW; }

"]" { return (NewParser::token_type)')'; }
"[" { return (NewParser::token_type)'('; }
"-" { return (NewParser::token_type)'-'; }

 /* ignored characters */
"[!@#$%^&|'?`~:^]" { yylloc->step(); }

"," {
    yylloc->step();
    //return token::COMMA;
}

 /* these are "identifiers" that can contain "-" anywhere in middle and numbers. */
[A-Za-z0-9_]([-A-Za-z0-9_]*[A-Za-z0-9_]|[A-Za-z0-9_])? {
    BEGIN(INITIAL);
    yylval->stringVal = new boost::shared_ptr<std::string>(new std::string(yytext, yyleng));
    return token::IDENT;
}

\"[^\n"]*\" {
    BEGIN(INITIAL);
    yylval->stringVal = new boost::shared_ptr<std::string>(new std::string(yytext + 1, yyleng - 2));
    return token::STRING_LIT;
}

 /* " */
 /* pass all other characters up to bison */
. {
    return static_cast<token_type>(*yytext);
}

 /*** END EXAMPLE - Change the example lexer rules above ***/

%% /*** Additional Code ***/

namespace Compiler {

    NewScanner::NewScanner(std::istream &in,
        std::ostream & out)
        : ExampleFlexLexer(in, out), mLegacy(true), mKeywordContext(true)
    {
        BEGIN(KEYWORDS);
    }

    NewScanner::~NewScanner()
    {
    }

    void NewScanner::set_debug(bool b)
    {
        yy_flex_debug = b;
    }

    void NewScanner::set_legacy(bool b)
    {
        mLegacy = b;
    }

    void NewScanner::unputstr(char * str, int len)
    {
        char * yycopy = strdup(str);
        while (len--) {
            unput(str[len]);
        }
        free(yycopy);
    }
    void NewScanner::set_keyword_context(bool keyword) {
        if (keyword) {
            BEGIN(KEYWORDS);
        } else {
            BEGIN(INITIAL);
        }
        //printf("Keyword Mode: %i\n", keyword);
        mKeywordContext = keyword;
    }

}

/* This implementation of ExampleFlexLexer::yylex() is required to fill the
 * vtable of the class ExampleFlexLexer. We define the scanner's main yylex
 * function via YY_DECL to reside in the NewScanner class instead. */

#ifdef yylex
#undef yylex
#endif

int ExampleFlexLexer::yylex()
{
    std::cerr << "in ExampleFlexLexer::yylex() !" << std::endl;
    return 0;
}

/* When the scanner receives an end-of-file indication from YY_INPUT, it then
 * checks the yywrap() function. If yywrap() returns false (zero), then it is
 * assumed that the function has gone ahead and set up `yyin' to point to
 * another input file, and scanning continues. If it returns true (non-zero),
 * then the scanner terminates, returning 0 to its caller. */

int ExampleFlexLexer::yywrap()
{
    return 1;
}
