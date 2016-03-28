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

/* change the name of the scanner class. results in "ExampleFlexLexer" */
%option prefix="Example"

/* the manual says "somewhat more optimized" */
%option batch

/* enable scanner to generate debug output. disable this for release
 * versions. */
%option debug

/* no support for include files is planned */
%option yywrap nounput

/* enables the use of start condition stacks */
%option stack

/* The following paragraph suffices to track locations accurately. Each time
 * yylex is invoked, the begin position is moved onto the end position. */
%{
#define YY_USER_ACTION  yylloc->columns(yyleng);

%}

%% /*** Regular Expressions Part ***/

 /* code to place at the beginning of yylex() */
%{
    // reset location
    yylloc->step();
%}

 /*** BEGIN EXAMPLE - Change the example lexer rules below ***/

 /* gobble up end-of-lines, comments, whitespace, non-alphanumeric pre-text */
([ \r\t]*(;[^\n]*)?\n[^-a-zA-Z0-9"]*)+ {
    bool eol = false;
    for (int i = 0; i < yyleng; i++) {
        if (yytext[i] == '\n') {
             eol = true;
             yylloc->lines(1);
        }
    }
    if (eol) {
        return token::EOL;
    } else {
        yylloc->step();
    }
}
[ \r\t]*;[^\n]* {
/* get the last comment in a file */
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
 /* " */

"]" { return (NewParser::token_type)')'; }
"[" { return (NewParser::token_type)'('; }

":" { yylloc->step(); }

 /*
[0-9]+ {
    yylval->longVal = atoi(yytext);
    return token::LONG_LIT;
}
*/

end[ \t]*if {
    if (mKeywordContext) {
        return token::ENDIF;
    } else {
        unputstr(yytext + 3, yyleng -3);
        yylval->stringVal = new boost::shared_ptr<std::string>(new std::string("end"));
        return token::IDENT;
    }
}

else[ \t]*if {
    if (mKeywordContext) {
        return token::ELSEIF;
    } else {
        unputstr(yytext + 4, yyleng -4);
        yylval->stringVal = new boost::shared_ptr<std::string>(new std::string("else"));
        return token::IDENT;
    }
}

 /* note that this does not allow - at the end of idents, to allow catching of -> */
[A-Za-z0-9][-A-Za-z0-9_]*-? {
    char * bad = NULL;
    for (int i = 0; i < yyleng && !bad; i++) {
        char * c = &yytext[i];
        switch(*c) {
        case '-':
            bad = c;
            break;
        default:
            break;
        }
    }
    if (!mLegacy && bad) {
        if (yytext[0] >= '0' && yytext[0] <= '9') {
            char * c = yytext;
            while (c < bad && *c >= '0' && *c <= '9') {
                c++;
            }
            if (c == bad) {
                yylval->longVal = atoi(yytext);
                return token::LONG_LIT;
            }
        }
        int goodlen = (bad - yytext);
        int badlen = yyleng - goodlen;
        if (bad == yytext) {
            unputstr(&bad[1], badlen - 1);
            return (NewParser::token_type)'-';
        } else if (getKeywordToken(yytext, goodlen) != token::UNDEFINED && mKeywordContext) {
            return getKeywordToken(yytext, goodlen);
        } else {
            yylval->stringVal = new boost::shared_ptr<std::string>(new std::string(yytext, goodlen));
            unputstr(&bad[1], badlen - 1);
            return token::IDENT;
        }
    } else if (yytext[yyleng - 1] == '-') {
        yylval->stringVal = new boost::shared_ptr<std::string>(new std::string(yytext, yyleng - 1));
        unput('-');
        return token::IDENT;
    } else if (getKeywordToken(yytext, yyleng) != token::UNDEFINED && mKeywordContext) {
        return getKeywordToken(yytext, yyleng);
    } else {
        yylval->stringVal = new boost::shared_ptr<std::string>(new std::string(yytext, yyleng));
        return token::IDENT;
    }
}

\"[^\n"]*\" {
    yylval->stringVal = new boost::shared_ptr<std::string>(new std::string(yytext + 1, yyleng - 2));
    return token::STRING_LIT;
}

"," {
    yylloc->step();
    //return token::COMMA;
}
 /* " */

 /* gobble up white-spaces */
[ \t\r]+ {
    yylloc->step();
}

 /* pass all other characters up to bison */
. {
    return static_cast<token_type>(*yytext);
}
 /*** END EXAMPLE - Change the example lexer rules above ***/

%% /*** Additional Code ***/

namespace Compiler {

    NewScanner::NewScanner(std::istream* in,
        std::ostream* out)
        : ExampleFlexLexer(in, out), mLegacy(true), mKeywordContext(true)
    {
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
        //printf("Keyword Mode: %i\n", keyword);
        mKeywordContext = keyword;
    }

    Compiler::NewParser::token_type NewScanner::getKeywordToken(char * text, int len) {
        char * keywords[] = {
            "if",
            "else",
            "elseif",
            "endif",
            "while",
            "endwhile",
            "set",
            "to",
            "return",
            "short",
            "float",
            "long",
            "begin",
            "end"
        };
        Compiler::NewParser::token_type tokens[] = {
            token::IF,
            token::ELSE,
            token::ELSEIF,
            token::ENDIF,
            token::WHILE,
            token::ENDWHILE,
            token::SET,
            token::TO,
            token::RETURN,
            token::SHORT,
            token::FLOAT,
            token::LONG,
            token::BEGIN_BLOCK,
            token::END_BLOCK
        };
        for (int i = 0; i < sizeof(keywords) / sizeof(char *); i++) {
            bool mismatch = false;
            for (int j = 0; j < len && j < strlen(keywords[i]); j++) {
                if (tolower(keywords[i][j]) != tolower(text[j])) {
                    mismatch = true;
                }
            }
            if (!mismatch && strlen(keywords[i]) == len) {
                return tokens[i];
            }
        }
        return token::UNDEFINED;
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
