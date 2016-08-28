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

namespace Compiler {
    template<typename Visitor, typename VisitablePtr, typename ResultType, typename ArgType>
    class ValueGetter : public Visitor {
    public:
        void Return(ResultType value_) {
            value = value_;
        }
        ArgType getArg() {
            return arg;
        }
        virtual ResultType acceptArgs(boost::shared_ptr<VisitablePtr> & n, ArgType arg) {
            this->arg = arg;
            n->accept(*this);
            return value;
        }
    private:
        ArgType arg;
        ResultType value;
    };

#define ARGVISITOR(_type_) \
    virtual void visit(_type_ t) { \
        this->Return(visit(t, this->getArg())); \
    };\
    virtual R visit(_type_ t, A arg) = 0

    template <typename R, typename A>
    class ExprVisitorArgs : public ValueGetter<ExprVisitor, AST::Expression, R, A> {
    public:
        ARGVISITOR(AST::FloatLit &);
        ARGVISITOR(AST::LongLit &);
        ARGVISITOR(AST::StringLit &);
        ARGVISITOR(AST::RefExpr &);
        ARGVISITOR(AST::MathExpr &);
        ARGVISITOR(AST::LogicExpr &);
        ARGVISITOR(AST::NegateExpr &);
        ARGVISITOR(AST::ExprItems &);

        ARGVISITOR(AST::CastExpr &);
        ARGVISITOR(AST::CallExpr &);
        ARGVISITOR(AST::CallArgs &);
        ARGVISITOR(AST::GlobalVar &);
        ARGVISITOR(AST::LocalVar &);
        ARGVISITOR(AST::MemberVar &);
        ARGVISITOR(AST::Journal &);
    };
    template <typename R, typename A>
    class StmtVisitorArgs : public ValueGetter<StmtVisitor, AST::Expression &, R, A> {
    public:

        ARGVISITOR(AST::TypeDecl &);
        ARGVISITOR(AST::SetStatement &);
        ARGVISITOR(AST::WhileStatement &);
        ARGVISITOR(AST::IfStatement &);
        ARGVISITOR(AST::ReturnStatement &);
        ARGVISITOR(AST::StatementExpr &);
        ARGVISITOR(AST::NoOp &);
    };
    template <typename R, typename A>
    class ModuleVisitorArgs : public ValueGetter<ModuleVisitor, AST::Expression &, R, A> {
    public:
        ARGVISITOR(AST::Module &);
    };
}
#endif
