#ifndef COMPILER_PRINTAST
#define COMPILER_PRINTAST

#include "ast.hpp"
#include "astvisitor.hpp"

#include <iostream>
#include <iomanip>

namespace Compiler
{

    class ExprPrinter : public Compiler::ExprVisitor
    {
        bool mVerbose;
    public:
        virtual void visit(AST::CallExpr & b) {
            if (mVerbose) {
                std::cout << "(CALL)";
            }
            b.getFn()->accept(*this);
            b.getArgs()->accept(*this);
        };
        virtual void visit(AST::CallArgs & a) {
            visit(dynamic_cast<AST::ExprItems &>(a));
        }
        virtual void visit(AST::Journal & f) {
            if (mVerbose) {
                std::cout << "(JOURNAL)";
            }
            std::cout << f.getJournal();
        };
        virtual void visit(AST::MemberVar & f) {
            std::cout << f.getModule() << "." << f.getName();
        };
        virtual void visit(AST::GlobalVar & f) {
            if (mVerbose) {
                std::cout << "(GLOBAL)";
            }
            std::cout << f.getGlobal();
        };
        virtual void visit(AST::LocalVar & f) {
            if (mVerbose) {
                std::cout << "(LOCAL)";
            }
            std::cout << f.getLocal();
        };
        virtual void visit(AST::CastExpr & e) {
            char c = ' ';
            boost::shared_ptr<AST::TypePrimitive> prim = boost::dynamic_pointer_cast<AST::TypePrimitive>(e.getSig());
            if (prim) {
                switch(prim->getPrim()) {
                case AST::FLOAT: c = 'f'; break;
                case AST::SHORT: c = 's'; break;
                case AST::STRING: c = 'S'; break;
                case AST::UNDEFINED: c = 'U'; break;
                case AST::LONG: c = 'l'; break;
                default:
                    c = ' ';
                    break;
                }
            }
            if (mVerbose) {
                std::cout << "cast<" << c << ">(";
                e.getExpr()->accept(*this);
                std::cout << ")";
            } else {
                e.getExpr()->accept(*this);
            }
        };
        virtual void visit(AST::FloatLit & f) { std::cout << f.getValue();};
        virtual void visit(AST::LongLit & l) { std::cout << l.getValue();};
        virtual void visit(AST::StringLit & s){ std::cout << "\"" << s.getValue() << "\"";};
        virtual void visit(AST::RefExpr & b) {
            std::cout << "(";
            if (b.isExplicit()) {
                b.getBase()->accept(*this);
                std::cout << AST::getBinOpString(b.getOp());
            }
            b.getOffset()->accept(*this);
            std::cout << ")";
        };
        virtual void visit(AST::NegateExpr & n) {
            std::cout << "(-";
            n.getExpr()->accept(*this);
            std::cout << ")";
        };
        virtual void visit(AST::ExprItems & n) {
            std::cout << "(";
            for(std::vector<boost::shared_ptr<AST::Expression> >::iterator it = n.getItems().begin();
                it != n.getItems().end();
                ++it) {
                (*it)->accept(*this);
                std::cout << ", ";
            }
            std::cout << ")";
        };
        virtual void visit(AST::MathExpr & b) {
            std::cout << "(";
            b.getLeft()->accept(*this);
            std::cout << AST::getBinOpString(b.getOp());
            b.getRight()->accept(*this);
            std::cout << ")";
        };
        virtual void visit(AST::LogicExpr & b) {
            std::cout << "(";
            b.getLeft()->accept(*this);
            std::cout << AST::getBinOpString(b.getOp());
            b.getRight()->accept(*this);
            std::cout << ")";
        };


    };

    class StmtPrinter : public Compiler::StmtVisitor {
        ExprPrinter mExpr;
        int mIndent;
    public:
        void printIndent()
        {
            std::cout << std::setfill('X') << std::setw(3) << "";
            for (int i = 0; i < mIndent; i++) {
                std::cout << "    ";
            }
        }
        void printIndent(const AST::FileToken & t)
        {
            const Compiler::TokenLoc & loc = t.getLoc();
            std::cout << std::setfill('0') << std::setw(3) << loc.mLine;;
            for (int i = 0; i < mIndent; i++) {
                std::cout << "    ";
            }
        }
        void setIndent(int indent) {
            mIndent = indent;
        }
        StmtPrinter() : mIndent(0) {}
        virtual void visit(AST::NoOp & s) {
            printIndent(s);
            std::cout << "; Invalid Operation Ignored" << std::endl;
        }
        virtual void visit(AST::SetStatement & s) {
            printIndent(s);
            std::cout << "SET ";
            s.getTarget()->accept(mExpr);
            std::cout << " TO ";
            s.getExpr()->accept(mExpr);
            std::cout << std::endl;
        }
        virtual void visit(AST::WhileStatement & w) {
            printIndent(w);
            std::cout << "WHILE";
            w.getCond()->accept(mExpr);
            std::cout << std::endl;
            mIndent++;
            for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = w.getStmts().begin();
                it != w.getStmts().end();
                ++it) {
                (*it)->accept(*this);
            }
            mIndent--;
            printIndent();
            std::cout << "ENDWHILE" << std::endl;
        }
        bool isElseIf(const std::vector<boost::shared_ptr<AST::Statement> > & else_stmts) {
            if (else_stmts.size() == 1) {
                boost::shared_ptr<AST::IfStatement> ifstmt = boost::dynamic_pointer_cast<AST::IfStatement>(else_stmts[0]);
                if (ifstmt) {
                    ifstmt->setElseIf();
                }
                return ifstmt;
            } else {
                return false;
            }
        }

        virtual void visit(AST::IfStatement & i)
        {
            printIndent(i);
            if (i.isElseIf()) {
                std::cout << "ELSE IF ";
            } else {
                std::cout << "IF ";
            }
            i.getCond()->accept(mExpr);
            std::cout << std::endl;
            mIndent++;
            for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = i.getTrueStmts().begin();
                it != i.getTrueStmts().end();
                ++it) {
                (*it)->accept(*this);
            }
            mIndent--;
            if (i.getFalseStmts().size() > 0) {
                if (isElseIf(i.getFalseStmts())) {
                    mIndent--;
                } else {
                    printIndent();
                    std::cout << "ELSE " << std::endl;
                }
                mIndent++;
                for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = i.getFalseStmts().begin();
                    it != i.getFalseStmts().end();
                    ++it) {
                    (*it)->accept(*this);
                }
                mIndent--;
                if (isElseIf(i.getFalseStmts())) {
                    mIndent++;
                }
            }
            if (!i.isElseIf()) {
                printIndent();
                std::cout << "END IF" << std::endl;
            }
        };
        virtual void visit(AST::ReturnStatement & f) {
            printIndent(f);
            std::cout << "RETURN" << std::endl;
        };
        virtual void visit(AST::StatementExpr & e) {
            printIndent(e);
            e.getExpr()->accept(mExpr);
            std::cout << std::endl;
        }
        virtual void visit(AST::TypeDecl & t) {
            printIndent(t);
            std::cout << AST::getTypeString(t.getType()) << " ";
            std::cout << t.getName() << std::endl;
        }
    };

    class ModulePrinter : public Compiler::ModuleVisitor {
        StmtPrinter mStmt;
    public:
        ModulePrinter()  {}
        virtual void visit(AST::Module & m) {
            std::cout << "BEGIN ";
            std::cout << m.getName() << std::endl;
            mStmt.setIndent(1);
            for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = m.getStmts().begin();
                it != m.getStmts().end();
                ++it) {
                (*it)->accept(mStmt);
            }
            std::cout << "END" << std::endl;
        }
    };
}

#endif
