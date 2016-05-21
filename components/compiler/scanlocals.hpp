#ifndef SCANLOCALS_HPP
#define SCANLOCALS_HPP

#include "context.hpp"
#include "locals.hpp"
#include "astvisitor.hpp"
#include "errorhandler.hpp"
#include "output.hpp"
#include "typecheck.hpp"
#include <map>
#include <boost/shared_ptr.hpp>

namespace Compiler
{
    class ModuleScanLocals;

    class StmtScanLocals : public Compiler::StmtVisitor
    {
        ModuleScanLocals & mModule;
    public:
        StmtScanLocals(ModuleScanLocals & m);
        virtual void visit(AST::TypeDecl & s);
        virtual void visit(AST::SetStatement & s);
        virtual void visit(AST::WhileStatement & w);
        virtual void visit(AST::IfStatement & i);
        virtual void visit(AST::ReturnStatement & f);
        virtual void visit(AST::StatementExpr & e);
        virtual void visit(AST::NoOp & e);
    };

    class ModuleScanLocals : public Compiler::ModuleVisitor
    {
        StmtScanLocals mStmt;
        Compiler::Output & mOutput;
        Compiler::ErrorHandler & mError;
        Compiler::Context & mContext;
    public:
        Compiler::ErrorHandler & getErrorHandler();
        Compiler::Context & getContext();
        Compiler::Locals & getLocals();
        ModuleScanLocals(Compiler::Context & c, Compiler::ErrorHandler & e, Compiler::Output & o);
        virtual void visit(AST::Module & m);
    };
}

#endif
