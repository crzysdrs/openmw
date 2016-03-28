#ifndef TYPECHECK_HPP
#define TYPECHECK_HPP

#include "context.hpp"
#include "locals.hpp"
#include "astvisitor.hpp"
#include "errorhandler.hpp"
#include <map>
#include <boost/shared_ptr.hpp>

namespace Compiler
{
    typedef std::string::const_iterator arg_iter;
    typedef std::vector<boost::shared_ptr<AST::Expression> >::iterator expr_iter;
    class ModuleTypeCheck;

    class ExprTypeCheck : public Compiler::ExprVisitor
    {
        ModuleTypeCheck & mModule;
        bool mIgnoreInstructions;
        bool mIgnoreFunctions;
        bool mMutable;
    public:
        ExprTypeCheck(ModuleTypeCheck & m);
        boost::shared_ptr<AST::CallArgs> processArgs(
            expr_iter & cur_expr, expr_iter & end_expr,
            arg_iter & cur_arg, arg_iter & end_arg, int optional);
        boost::shared_ptr<AST::Expression> processFn(
            expr_iter & cur_expr, expr_iter & end_expr,
            arg_iter & cur_arg, arg_iter & end_arg, int optional);
        void doReplace(AST::Expression & e, boost::shared_ptr<AST::Expression> replace);
        ExprTypeCheck(const ExprTypeCheck & old)
            : mModule(old.mModule), mIgnoreFunctions(old.mIgnoreFunctions), mIgnoreInstructions(old.mIgnoreInstructions), mMutable(old.mMutable) {
        };
        void setIgnoreInstructions() {
            mIgnoreInstructions = true;
        };
        void setIgnoreFunctions() {
            mIgnoreFunctions = true;
        };
        void setIgnoreCalls() {
            setIgnoreInstructions();
            setIgnoreFunctions();
        }
        void setImmutable() {
            mMutable = false;
        };
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

    class StmtTypeCheck : public Compiler::StmtVisitor
    {
        ModuleTypeCheck & mModule;
        ExprTypeCheck mExpr;
    public:
        StmtTypeCheck(ModuleTypeCheck & m);
        virtual void visit(AST::TypeDecl & s);
        virtual void visit(AST::SetStatement & s);
        virtual void visit(AST::WhileStatement & w);
        virtual void visit(AST::IfStatement & i);
        virtual void visit(AST::ReturnStatement & f);
        virtual void visit(AST::StatementExpr & e);
        virtual void visit(AST::NoOp & e);
    };

    class ModuleTypeCheck : public Compiler::ModuleVisitor
    {
        Compiler::Context & mContext;
        StmtTypeCheck mStmt;
        Locals mLocals;
        ErrorHandler & mError;
        std::map<AST::Type, boost::shared_ptr<AST::TypeSig> > mSigs;
    public:
        Compiler::ErrorHandler & getErrorHandler();
        Compiler::Context & getContext();
        Compiler::Locals & getLocals();
        boost::shared_ptr<AST::TypeSig> getSig(AST::Type t);
        ModuleTypeCheck(Compiler::ErrorHandler & h, Compiler::Context & c);
        virtual void visit(AST::Module & m);
    };
}

#endif
