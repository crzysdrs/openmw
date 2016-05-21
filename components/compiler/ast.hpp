#ifndef AST_H
#define AST_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include "tokenloc.hpp"
#include "extensions.hpp"

namespace Compiler
{
    class ExprVisitor;
    class StmtVisitor;
    class ModuleVisitor;
}

namespace AST
{
    enum BinOp
    {
#define BIN_OP(_name_, _print_) _name_,
#include "ast_defines.hpp"
        BIN_OP_LENGTH
    };

    enum Type
    {
#define MW_TYPE(_name_, _print_) _name_,
#include "ast_defines.hpp"
        TYPE_LEN
    };

    class TypeSig {
    public:
        virtual bool operator==(const TypeSig & sig) = 0;
        virtual bool operator!=(const TypeSig & sig) = 0;
    };
    class TypePrimitive : public TypeSig
    {
        AST::Type mPrim;
        bool isGlobal; /* used for membervars */
    public:
        TypePrimitive(AST::Type prim) : mPrim(prim) {};
        virtual bool operator==(const TypeSig & sig) {
            if (this == &sig) {
                return true;
            } else if (typeid(*this) != typeid(sig)) {
                return false;
            } else {
                return this->mPrim == static_cast<const TypePrimitive &>(sig).mPrim;
            }
        };
        virtual bool operator!=(const TypeSig & sig) {
            return !(*this == sig);
        };
        virtual AST::Type getPrim() const { return mPrim; };
        virtual void setGlobal() { isGlobal = true; };
        virtual bool getGlobal() { return isGlobal; };

    };

    class TypeArgs : public TypeSig
    {
        Compiler::ScriptArgs mArgs;
        bool mIsMessageBox;
        int mOptionals;
    protected:
        TypeArgs(const Compiler::ScriptArgs & args, bool msgbox = false) : mArgs(args), mIsMessageBox(msgbox) {};
    public:
        virtual bool operator==(const TypeSig & sig) {
            /* can't really test equality.... not sure why we would need to */
            return (this == &sig);
        };
        virtual bool operator!=(const TypeSig & sig) {
            return !(*this == sig);
        };
        const Compiler::ScriptArgs & getArgs() { return mArgs; };
        void setArgs(const Compiler::ScriptArgs & args) { mArgs = args; };
        void setOptionals(int optionals) { mOptionals = optionals; };
        int getOptionals() { return mOptionals; };
        void setMessageBox() {
            mIsMessageBox = true;
        };
        bool isMessageBox() {
            return mIsMessageBox;
        };
    };

    class TypeFunction : public TypeArgs
    {
        Compiler::ScriptReturn mRet;
    public:
        TypeFunction(const Compiler::ScriptArgs & args, const Compiler::ScriptReturn & ret) : TypeArgs(args), mRet(ret)  {};
        Compiler::ScriptReturn getRet() { return mRet; };
    };

    class TypeInstruction : public TypeArgs
    {
    public:
        TypeInstruction(const std::string & args, bool msgbox) : TypeArgs(args, msgbox) {};
    };

    class FileToken {
        const Compiler::TokenLoc mLoc;
    protected:
        FileToken(const Compiler::TokenLoc & t) : mLoc(t) {};
    public:
        virtual const Compiler::TokenLoc & getLoc() const { return mLoc; };
    };
    const char * getTypeString(Type t);
    const char * getBinOpString(BinOp t);
    class Expression : public FileToken
    {
        boost::shared_ptr<Expression> mReplace;
        boost::shared_ptr<TypeSig> mSig;
    protected:
        Expression(const Compiler::TokenLoc & t) : FileToken(t) {}
    public:
        virtual boost::shared_ptr<Expression> replaceMe() {
            return mReplace;
        };
        virtual void setReplace(boost::shared_ptr<Expression> replace) {
            mReplace = replace;
        };
        virtual void accept(Compiler::ExprVisitor & visitor) = 0;
        virtual bool coerceString(std::string & str) {
            return false;
        };
        virtual void setSig(boost::shared_ptr<TypeSig> sig) {
            mSig = sig;
        };
        virtual boost::shared_ptr<TypeSig> getSig() {
            return mSig;
        };
    };

    class Literal : public Expression
    {
    protected:
        Literal(const Compiler::TokenLoc & t) : Expression(t) {}
    };
    class FloatLit : public Literal
    {
        float mValue;
    public:
        FloatLit(const Compiler::TokenLoc & t, float f) : Literal(t), mValue(f) {};
        float getValue() {return mValue;}
        virtual void accept(Compiler::ExprVisitor & visitor);
    };
    class LongLit : public Literal
    {
        int mValue;
    public:
        LongLit(const Compiler::TokenLoc & t, int l) : Literal(t), mValue(l) {};
        int getValue() {return mValue;}
        virtual void accept(Compiler::ExprVisitor & visitor);
    };
    class StringLit : public Literal
    {
        std::string mValue;
    public:
        StringLit(const Compiler::TokenLoc & t, const std::string & s) : Literal(t),  mValue(s) {};
        std::string getValue() {return mValue;}
        virtual bool coerceString(std::string & str) {
            str = mValue;
            return true;
        }
        virtual void accept(Compiler::ExprVisitor & visitor);
    };
    /* memberaccess, localaccess, fn, instr?, journalid?, */
    class Var : public Expression {
    protected:
        Var(const Compiler::TokenLoc & t) : Expression(t) {};
    };

    class GlobalVar : public Var {
        std::string mGlobal;
    public:
        GlobalVar(const Compiler::TokenLoc & t, const std::string & global) : Var(t), mGlobal(global) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
        std::string getGlobal() { return mGlobal; };
    };

    class LocalVar : public Var {
        std::string mLocal;
    public:
        LocalVar(const Compiler::TokenLoc & t, const std::string & local) :  Var(t), mLocal(local) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
        std::string getLocal() { return mLocal; };
    };

    class MemberVar : public Var {
        std::string mModule;
        std::string mName;
    public:
        MemberVar(const Compiler::TokenLoc & t, const std::string & mod, const std::string & member) :  Var(t),  mModule(mod), mName(member) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
        std::string getModule() { return mModule; };
        std::string getName() { return mName; };
    };
    class Journal : public Var {
        std::string mJournal;
    public:
        Journal(const Compiler::TokenLoc & t, const std::string & journal) :  Var(t), mJournal(journal) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
        std::string getJournal() { return mJournal; };
    };

    class UnaryExpr: public Expression
    {
    protected:
        boost::shared_ptr<Expression> mExpr;
    public:
        UnaryExpr(const Compiler::TokenLoc & t, boost::shared_ptr<Expression> expr) : Expression(t), mExpr(expr) {};
        boost::shared_ptr<Expression> & getExpr() { return mExpr; };
    };
    class CastExpr : public UnaryExpr
    {
    public:
        CastExpr(const Compiler::TokenLoc & t, boost::shared_ptr<Expression> expr) : UnaryExpr(t, expr) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
    };
    class NegateExpr: public UnaryExpr
    {
    public:
        NegateExpr(const Compiler::TokenLoc & t, boost::shared_ptr<Expression> expr) : UnaryExpr(t, expr) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
    };
    class BinExpr : public Expression
    {
    protected:
        BinOp mOp;
        boost::shared_ptr<Expression> mLeft, mRight;
    public:
        BinExpr(const Compiler::TokenLoc & t, BinOp op,
            boost::shared_ptr<Expression> l,
            boost::shared_ptr<Expression> r)
            : Expression(t), mOp(op), mLeft(l), mRight(r) {};
        boost::shared_ptr<Expression> & getLeft() { return mLeft; };
        void setLeft(boost::shared_ptr<Expression> l) { mLeft = l; };
        boost::shared_ptr<Expression> & getRight() { return mRight; };
        BinOp getOp() { return mOp; };
    };
    class RefExpr : public Expression
    {
        BinOp mOp;
        boost::shared_ptr<StringLit> mBase;
        boost::shared_ptr<StringLit> mOffset;
    public:
        RefExpr(const Compiler::TokenLoc & t, BinOp op,
            boost::shared_ptr<StringLit> l,
            boost::shared_ptr<StringLit> r)
            : Expression(t), mOp(op), mBase(l), mOffset(r) {};
        RefExpr(const Compiler::TokenLoc & t,
            const boost::shared_ptr<StringLit> & r)
            : Expression(t), mOp(), mBase(), mOffset(r) {};
        virtual bool isExplicit() {
            return mBase;
        }
        virtual void accept(Compiler::ExprVisitor & visitor);
        void setBase(boost::shared_ptr<StringLit> newBase) { mBase = newBase; };
        boost::shared_ptr<StringLit> getBase() { return mBase; };
        boost::shared_ptr<StringLit> getOffset() { return mOffset; };
        BinOp getOp() { return mOp; };
        virtual bool coerceString(std::string & str) {
            if (!isExplicit()) {
                return getOffset()->coerceString(str);
            } else {
                return false;
            }
        }
    };
    class LogicExpr : public BinExpr
    {
    public:
        LogicExpr(const Compiler::TokenLoc & t, BinOp op,
            boost::shared_ptr<Expression> l,
            boost::shared_ptr<Expression> r)
            : BinExpr(t, op, l, r) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
    };

    class MathExpr : public BinExpr
    {
    public:
        MathExpr(const Compiler::TokenLoc & t, BinOp op,
            boost::shared_ptr<Expression> l,
            boost::shared_ptr<Expression> r)
            : BinExpr(t, op, l, r) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
    };
    class ExprItems : public Expression
    {
        std::vector<boost::shared_ptr<Expression> > mList;
    public:
        ExprItems(const Compiler::TokenLoc & t, const std::vector<boost::shared_ptr<Expression> > & list)
            : Expression(t), mList(list) {}
        virtual void accept(Compiler::ExprVisitor & visitor);
        std::vector<boost::shared_ptr<Expression> > & getItems() { return mList; };

        virtual bool coerceString(std::string & str) {
            if (mList.size() == 1) {
                return mList[0]->coerceString(str);
            } else {
                return false;
            }
        }

    };

    class CallArgs : public ExprItems
    {
    public:
        CallArgs(const Compiler::TokenLoc & t, const std::vector<boost::shared_ptr<Expression> > & list) : ExprItems(t, list) {};
        virtual void accept(Compiler::ExprVisitor & visitor);
    };

    class CallExpr : public Expression
    {
        boost::shared_ptr<Expression> mFn;
        boost::shared_ptr<CallArgs> mArgs;
    public:
        CallExpr(const Compiler::TokenLoc & t, boost::shared_ptr<AST::Expression> fn,  boost::shared_ptr<AST::CallArgs> args)
            : Expression(t), mFn(fn), mArgs(args) {};
        virtual void accept(Compiler::ExprVisitor & visitor);

        virtual boost::shared_ptr<Expression> & getFn() { return mFn; };
        virtual boost::shared_ptr<CallArgs> & getArgs() { return mArgs; };
    };
    class Statement : public FileToken
    {
    protected:
        Statement(const Compiler::TokenLoc & t) : FileToken(t) {}
    public:
        virtual void accept(Compiler::StmtVisitor & v) = 0;
    };

    class NoOp : public Statement
    {
    public:
        virtual void accept(Compiler::StmtVisitor & v);
        NoOp(const Compiler::TokenLoc & t) : Statement(t) {};
    };

    class TypeDecl : public Statement
    {
        Type mType;
        boost::shared_ptr<std::string> mName;
    public:
        TypeDecl(const Compiler::TokenLoc & t, Type type, boost::shared_ptr<std::string> name) : Statement(t), mType(type), mName(name) {};
        virtual void accept(Compiler::StmtVisitor & v);
        Type getType() { return mType; };
        std::string getName() { return *mName; }
    };

    class SetStatement : public Statement
    {
        boost::shared_ptr<Expression> mTarget;
        boost::shared_ptr<Expression> mExpr;
        bool mSet;
    public:
        SetStatement(const Compiler::TokenLoc & t, boost::shared_ptr<Expression> target,
            boost::shared_ptr<Expression> expr)
            : Statement(t), mTarget(target), mExpr(expr), mSet(true) {};
        virtual void accept(Compiler::StmtVisitor & v);
        boost::shared_ptr<Expression> & getTarget() { return mTarget; }
        boost::shared_ptr<Expression> & getExpr() { return mExpr; }
        void ignoreSet() { mSet = false; }
        bool isValid() { return mSet; }
    };

    class WhileStatement : public Statement
    {
        boost::shared_ptr<Expression> mCondition;
        std::vector<boost::shared_ptr<Statement> > mStmts;
    public:
        WhileStatement(const Compiler::TokenLoc & t,
            boost::shared_ptr<Expression> cond,
            const std::vector<boost::shared_ptr<Statement> >& stmts)
            : Statement(t), mCondition(cond), mStmts(stmts) {};
        virtual void accept(Compiler::StmtVisitor & v);
        boost::shared_ptr<Expression> & getCond() { return mCondition; };
        std::vector<boost::shared_ptr<Statement> > & getStmts() { return mStmts; };
    };

    class IfStatement : public Statement
    {
        boost::shared_ptr<Expression> mCondition;
        std::vector<boost::shared_ptr<Statement> > mIfCase;
        std::vector<boost::shared_ptr<Statement> > mElseCase;
        bool mIsElseIf;
    public:
        IfStatement(
            const Compiler::TokenLoc & t,
            boost::shared_ptr<Expression> cond,
            std::vector<boost::shared_ptr<Statement> >& branch_true)
            : Statement(t), mCondition(cond), mIfCase(branch_true), mIsElseIf(false), mElseCase() {};
        IfStatement(
            const Compiler::TokenLoc & t,
            boost::shared_ptr<Expression> cond,
            std::vector<boost::shared_ptr<Statement> >& branch_true,
            std::vector<boost::shared_ptr<Statement> > & branch_false)
            : Statement(t), mCondition(cond), mIfCase(branch_true), mElseCase(branch_false), mIsElseIf(false) {};
        void setElseIf() {
            mIsElseIf = true;
        };
        bool isElseIf() {
            return mIsElseIf;
        };
        void setElse(std::vector<boost::shared_ptr<Statement> >& branch_else) {
            mElseCase = branch_else;
        }

        boost::shared_ptr<Expression> & getCond() { return mCondition; };
        std::vector<boost::shared_ptr<Statement> > & getTrueStmts() { return mIfCase; };
        std::vector<boost::shared_ptr<Statement> > & getFalseStmts() { return mElseCase; };

        virtual void accept(Compiler::StmtVisitor & v);
    };

    class StatementExpr : public Statement
    {
        boost::shared_ptr<Expression> mExpr;
    public:
        StatementExpr(const Compiler::TokenLoc & t, boost::shared_ptr<Expression> expr) : Statement(t), mExpr(expr) {}
        virtual void accept(Compiler::StmtVisitor & v);
        boost::shared_ptr<Expression> & getExpr() { return mExpr; };
    };
    class ReturnStatement : public Statement
    {
    public:
        ReturnStatement(const Compiler::TokenLoc & t) : Statement(t) {};
        virtual void accept(Compiler::StmtVisitor & v);
    };

    class Module : public FileToken
    {
        boost::shared_ptr<std::string> mName;
        std::vector<boost::shared_ptr<Statement> > mStmts;
    public:
        Module(const Compiler::TokenLoc & t, boost::shared_ptr<std::string> name, const std::vector<boost::shared_ptr<Statement> > & stmts)
            : FileToken(t), mName(name), mStmts(stmts) {};
        virtual void accept(Compiler::ModuleVisitor & v);
        std::string getName() { return *mName; };
        std::vector<boost::shared_ptr<Statement> > & getStmts() { return mStmts; };
    };

    typedef boost::shared_ptr<AST::TypeSig> shared_typesig;

    char convertASTType(const AST::Type & t);
    char convertType(const AST::TypeSig & t);
    char convertLogicOp(const AST::BinOp op);
}

#endif
