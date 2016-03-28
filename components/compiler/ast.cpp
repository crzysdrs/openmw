#include "ast.hpp"
#include "astvisitor.hpp"

namespace AST {
    char convertASTType(const AST::Type & t) {
        switch (t) {
        case AST::FLOAT: return 'f';
        case AST::LONG: return 'l';
        case AST::SHORT: return 's';
        default:
            return ' ';
        }
    }

    char convertType(const AST::TypeSig & t)
    {
        const AST::TypePrimitive & sig = dynamic_cast<const AST::TypePrimitive &>(t);
        return convertASTType(sig.getPrim());
    }
    char convertLogicOp(const AST::BinOp op)
    {
        switch (op) {
        case AST::EQ:
            return 'e';
        case AST::NEQ:
            return 'n';
        case AST::LT:
            return 'l';
        case AST::LTE:
            return 'L';
        case AST::GT:
            return 'g';
        case AST::GTE:
            return 'G';
        default:
            assert(0);
            return ' ';
        }
    }
    void LongLit::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void FloatLit::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void StringLit::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void RefExpr::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void LogicExpr::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void MathExpr::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void NegateExpr::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void ExprItems::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void CallArgs::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void CallExpr::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void CastExpr::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void MemberVar::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void GlobalVar::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void LocalVar::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }
    void Journal::accept(Compiler::ExprVisitor & v) {
        v.visit(*this);
    }

    void ReturnStatement::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void TypeDecl::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void SetStatement::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void WhileStatement::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void IfStatement::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void StatementExpr::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void NoOp::accept(Compiler::StmtVisitor & v) {
        v.visit(*this);
    }
    void Module::accept(Compiler::ModuleVisitor & v) {
        v.visit(*this);
    }

    const char * getTypeString(Type t) {
        switch(t) {
#define MW_TYPE(_name_, _print_) case _name_: return _print_;
#include "ast_defines.hpp"
        default:
            return "Error";
        }
    }

    const char * getBinOpString(BinOp t) {
        switch(t) {
#define BIN_OP(_name_, _print_) case _name_: return _print_;
#include "ast_defines.hpp"
        default:
            return "Error";
        }
    }
}
