#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include "ast.hpp"
#include <stack>
#include <boost/make_shared.hpp>
#include "assert.h"

namespace Compiler {
    class ExprVisitor {
    public:
        virtual void visit(AST::FloatLit & f) = 0;
        virtual void visit(AST::LongLit & l) = 0;
        virtual void visit(AST::StringLit & s) = 0;
        virtual void visit(AST::RefExpr & t) = 0;
        virtual void visit(AST::MathExpr & b) = 0;
        virtual void visit(AST::LogicExpr & b) = 0;
        virtual void visit(AST::NegateExpr & b) = 0;
        virtual void visit(AST::ExprItems & f) = 0;
        virtual void visit(AST::CallExpr & f) = 0;
        virtual void visit(AST::CallArgs & f) = 0;
        virtual void visit(AST::CastExpr & f) = 0;
        virtual void visit(AST::GlobalVar & e) = 0;
        virtual void visit(AST::LocalVar & e) = 0;
        virtual void visit(AST::MemberVar & e) = 0;
        virtual void visit(AST::Journal & e) = 0;
    };
    class StmtVisitor {
    public:
        virtual void visit(AST::TypeDecl & s) = 0;
        virtual void visit(AST::SetStatement & s) = 0;
        virtual void visit(AST::WhileStatement & w) = 0;
        virtual void visit(AST::IfStatement & i) = 0;
        virtual void visit(AST::ReturnStatement & f) = 0;
        virtual void visit(AST::StatementExpr & e) = 0;
        virtual void visit(AST::NoOp & e) = 0;
    };

    class ModuleVisitor {
    public:
        virtual void visit(AST::Module & m) = 0;
    };
}
#endif
