#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "context.hpp"
#include "locals.hpp"
#include "literals.hpp"
#include "astvisitor.hpp"
#include "errorhandler.hpp"
#include "output.hpp"
#include <map>
#include <boost/shared_ptr.hpp>
#include <components/interpreter/types.hpp>

namespace Compiler
{
    class ModuleCodegen;

    class ExprCodegen : public Compiler::ExprVisitor
    {
        ModuleCodegen & mModule;
        bool mLHS;
    public:
        ExprCodegen(ModuleCodegen & m, bool lhs);
        virtual void visit(AST::FloatLit & e);
        virtual void visit(AST::LongLit & e);
        virtual void visit(AST::StringLit & e);
        virtual void visit(AST::RefExpr & e);
        virtual void visit(AST::MathExpr & e);
        virtual void visit(AST::LogicExpr & e);
        virtual void visit(AST::NegateExpr & e);
        virtual void visit(AST::ExprItems & e);
        virtual void visit(AST::CastExpr & f);
        virtual void visit(AST::GlobalVar & e);
        virtual void visit(AST::LocalVar & e);
        virtual void visit(AST::MemberVar & e);
        virtual void visit(AST::Journal & e);
        virtual void visit(AST::CallExpr & f);
        virtual void visit(AST::CallArgs & f);
        virtual void acceptThis(boost::shared_ptr<AST::Expression> & e);
    };

    class StmtCodegen : public Compiler::StmtVisitor
    {
        ModuleCodegen & mModule;
        ExprCodegen mExprLHS, mExprRHS;
    public:
        StmtCodegen(ModuleCodegen & m);
        virtual void visit(AST::TypeDecl & s);
        virtual void visit(AST::SetStatement & s);
        virtual void visit(AST::WhileStatement & w);
        virtual void visit(AST::IfStatement & i);
        virtual void visit(AST::ReturnStatement & f);
        virtual void visit(AST::StatementExpr & e);
        virtual void visit(AST::NoOp & e);
    };

    class ModuleCodegen : public Compiler::ModuleVisitor
    {
        Compiler::Context & mContext;
        Compiler::Output & mOutput;
        bool mConsole;
        StmtCodegen mStmt;

    public:
        Compiler::Output & getOutput();
        Compiler::Context & getContext();
        bool isConsole();
        ModuleCodegen(Compiler::Context & c, Compiler::Output & o);
        virtual void visit(AST::Module & m);
    };
}

#endif
