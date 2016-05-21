#include "typecheck.hpp"
#include "scanlocals.hpp"
#include "context.hpp"
#include "extensions.hpp"
#include "extensions0.hpp"
#include "errorhandler.hpp"

#include <string>
#include <iostream>
#include <boost/regex.hpp>
#include <components/misc/stringops.hpp>

namespace Compiler {
    Locals & ModuleScanLocals::getLocals() {
        return mOutput.getLocals();
    }
    ErrorHandler & ModuleScanLocals::getErrorHandler() {
        return mError;
    }
    Context & ModuleScanLocals::getContext() {
        return mContext;
    }
    ModuleScanLocals::ModuleScanLocals(Compiler::Context & c, Compiler::ErrorHandler & e,Output & o)
        : mStmt(*this), mOutput(o), mError(e), mContext(c) {}

    void ModuleScanLocals::visit(AST::Module & m) {
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = m.getStmts().begin();
            it != m.getStmts().end();
            ++it) {
            (*it)->accept(mStmt);
        }
    }

    StmtScanLocals::StmtScanLocals(ModuleScanLocals & m)
        : mModule(m) {
    }

    void StmtScanLocals::visit(AST::TypeDecl & s) {
        Locals & l = mModule.getLocals();
        l.declare(convertASTType(s.getType()), s.getName());
    }
    void StmtScanLocals::visit(AST::SetStatement & s) {
        /* do nothing */
    }
    void StmtScanLocals::visit(AST::NoOp & n) {
        /* nothing to do */
    }
    void StmtScanLocals::visit(AST::WhileStatement & w) {
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = w.getStmts().begin();
            it != w.getStmts().end();
            ++it) {
            (*it)->accept(*this);
        }
    }
    void StmtScanLocals::visit(AST::IfStatement & i) {
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = i.getTrueStmts().begin();
            it != i.getTrueStmts().end();
            ++it) {
            (*it)->accept(*this);
        }
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = i.getFalseStmts().begin();
            it != i.getFalseStmts().end();
            ++it) {
            (*it)->accept(*this);
        }
    }
    void StmtScanLocals::visit(AST::ReturnStatement & f) {
        /* do nothing */
    }
    void StmtScanLocals::visit(AST::StatementExpr & e) {
        /* do nothing */
    }
}
