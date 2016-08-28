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
    typedef void * ExprArgType;
    typedef std::pair<boost::shared_ptr<AST::Expression>, boost::shared_ptr<AST::TypeSig> > ExprReturnType;

    class ExprTypeCheck : public Compiler::ExprVisitorArgs<ExprReturnType, ExprArgType>
    {
        ModuleTypeCheck & mModule;
        bool mIgnoreFunctions;
        bool mIgnoreInstructions;
        bool mMutable;
    public:
        ExprTypeCheck(ModuleTypeCheck & m);
        boost::shared_ptr<AST::CallArgs> processArgs(
            expr_iter & cur_expr, expr_iter & end_expr,
            arg_iter & cur_arg, arg_iter & end_arg, bool & optional);
        boost::shared_ptr<AST::Expression> processFn(
            expr_iter & cur_expr, expr_iter & end_expr,
            arg_iter & cur_arg, arg_iter & end_arg, bool toplevel, int optional);
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
        virtual ExprReturnType visit(AST::FloatLit & e, ExprArgType a);
        virtual ExprReturnType visit(AST::LongLit & e, ExprArgType a);
        virtual ExprReturnType visit(AST::StringLit & e, ExprArgType a);
        virtual ExprReturnType visit(AST::RefExpr & e, ExprArgType a);
        virtual ExprReturnType visit(AST::MathExpr & e, ExprArgType a);
        virtual ExprReturnType visit(AST::LogicExpr & e, ExprArgType a);
        virtual ExprReturnType visit(AST::NegateExpr & e, ExprArgType a);
        virtual ExprReturnType visit(AST::ExprItems & e, ExprArgType a);
        virtual ExprReturnType visit(AST::CastExpr & f, ExprArgType a);
        virtual ExprReturnType visit(AST::GlobalVar & e, ExprArgType a);
        virtual ExprReturnType visit(AST::LocalVar & e, ExprArgType a);
        virtual ExprReturnType visit(AST::MemberVar & e, ExprArgType a);
        virtual ExprReturnType visit(AST::Journal & e, ExprArgType a);
        virtual ExprReturnType visit(AST::CallExpr & f, ExprArgType a);
        virtual ExprReturnType visit(AST::CallArgs & f, ExprArgType a);
        ExprReturnType error_pair();

        virtual ExprReturnType acceptArgs(boost::shared_ptr<AST::Expression> & n, ExprArgType arg) {
            ExprReturnType ret = ValueGetter::acceptArgs(n, arg);
            if (mMutable && ret.first) {
                n = ret.first;
            }
            ret.first = n;
            if (ret.second) {
                n->setSig(ret.second);
            }
            assert(n->getSig());
            return ret;
        }
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
        Locals & mLocals;
        ErrorHandler & mError;
        std::map<AST::Type, boost::shared_ptr<AST::TypeSig> > mSigs;
    public:
        Compiler::ErrorHandler & getErrorHandler();
        Compiler::Context & getContext();
        Compiler::Locals & getLocals();
        boost::shared_ptr<AST::TypeSig> getSig(AST::Type t);
        ModuleTypeCheck(Compiler::Locals & l, Compiler::ErrorHandler & h, Compiler::Context & c);
        virtual void visit(AST::Module & m);
    };
}

#endif
