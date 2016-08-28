#include "typecheck.hpp"
#include "context.hpp"
#include "extensions.hpp"
#include "extensions0.hpp"
#include "errorhandler.hpp"
#include "exception.hpp"

#include <string>
#include <iostream>
#include <boost/regex.hpp>
#include <components/misc/stringops.hpp>

namespace Compiler {
    static std::string formatMessageBox(const std::string & format)
    {
        std::string result("S");
        std::string::const_iterator start = format.begin();
        std::string::const_iterator end   = format.end();

        static const boost::regex pattern("%(%|(?:\\.[0-9]+)?[fFgG]|[sS])");
        boost::smatch regex_result;
        while (boost::regex_search(start, end, regex_result, pattern)) {
            std::string submatch(regex_result[1].first, regex_result[1].second);
            switch (*submatch.rbegin()) {
            case '%':
                break;
            case 'f':
            case 'F':
                result += 'f';
                break;
            case 'g':
            case 'G':
                result += 'l';
                break;
            case 's':
            case 'S':
                result += 'S';
                break;
            default:
                assert(0);
            }
            start = regex_result[0].second;
        }

        result += '/';
        result += std::string(256, 'S'); /* 256 optional buttons */

        return result;
    }
    static AST::Type convertCharType(char type)
    {
        switch (type) {
        case 'f': return AST::FLOAT;
        case 'l': return AST::LONG;
        case 's': return AST::SHORT;
        case 'c': /* fallthrough */
        case 'S': return AST::STRING;
        default:
            return AST::UNDEFINED;
        }
    }
    static AST::shared_typesig convertOldType(ModuleTypeCheck & mod, char type)
    {
        switch (type) {
        case 'f': return mod.getSig(AST::FLOAT);
        case 'l': return mod.getSig(AST::LONG);
        case 's': return mod.getSig(AST::SHORT);
        case 'c': /* fallthrough */
        case 'S': return mod.getSig(AST::STRING);
        default:
            return mod.getSig(AST::UNDEFINED);
        }
    }

    static bool isNumeric(const AST::TypeSig & t) {
        try {
            const AST::TypePrimitive & sig = dynamic_cast<const AST::TypePrimitive &>(t);
            switch (sig.getPrim()) {
            case AST::FLOAT:
            case AST::LONG:
            case AST::SHORT:
                return true;
            default:
                return false;
            }
        } catch (std::bad_cast exp) {
            return false;
        }
    }

    static AST::shared_typesig binCoerce(ModuleTypeCheck & m, AST::Expression & e1, AST::Expression & e2) {
        AST::shared_typesig t1 = e1.getSig();
        AST::shared_typesig t2 = e2.getSig();

        AST::shared_typesig floatsig = m.getSig(AST::FLOAT);
        AST::shared_typesig longsig = m.getSig(AST::LONG);
        AST::shared_typesig shortsig = m.getSig(AST::SHORT);

        if (!t1 || !t2 || !floatsig || !longsig || !shortsig) {
            m.getErrorHandler().error("Unable to typecheck expression.", e1.getLoc());
            return m.getSig(AST::UNDEFINED);
        } else if (!isNumeric(*t1) || !isNumeric(*t2)) {
            m.getErrorHandler().error("Attempting to use non-numeric types in binary expr.", e1.getLoc());
            return m.getSig(AST::UNDEFINED);
        } else if (*t1 == *t2) {
            return t1;
        } else if (*t1 == *floatsig || *t2 == *floatsig) {
            return floatsig;
        } else if (*t1 == *longsig || *t2 == *longsig) {
            return longsig;
        } else {
            return shortsig;
        }
    }

    static void castWarning(ModuleTypeCheck & m, AST::shared_typesig l, AST::Expression & e)
    {
        AST::shared_typesig r = e.getSig();

        AST::shared_typesig floatsig = m.getSig(AST::FLOAT);
        AST::shared_typesig longsig = m.getSig(AST::LONG);
        AST::shared_typesig shortsig = m.getSig(AST::SHORT);


        if (!l || !r || !floatsig || !longsig || !shortsig) {
            m.getErrorHandler().error("Unable to typecheck expression..", e.getLoc());
        } else if (!isNumeric(*l) || !isNumeric(*r)) {
            m.getErrorHandler().error("Attempting to use non-numeric types in cast expr.", e.getLoc());
        } else if (*l == *r) {
            /* no warning */
        } else if (*l == *floatsig) {
            /* no possible loss of precision */
        } else if ((*l == *longsig || *l == *shortsig) && *r == *floatsig) {
            m.getErrorHandler().warning("Casting float to long/short. Possible loss of precision.", e.getLoc());
        } else if (*l == *shortsig && (*r == *floatsig || *r == *longsig)) {
            m.getErrorHandler().warning("Casting long/float to short. Possible loss of precision.", e.getLoc());
        } else if (*l == *longsig && (*r == *longsig || *r == *shortsig)) {
            /* no possible loss of precision. */
        }
    }

    void argCoerce(ModuleTypeCheck & m, char c, boost::shared_ptr<AST::Expression> & e) {
        AST::shared_typesig argsig = convertOldType(m, c);

        switch(c) {
        case 'f':
        case 'l':
        case 's':
            if (*e->getSig() != *argsig) {
                boost::shared_ptr<AST::TypePrimitive> to = boost::dynamic_pointer_cast<AST::TypePrimitive>(argsig);
                boost::shared_ptr<AST::TypePrimitive> from = boost::dynamic_pointer_cast<AST::TypePrimitive>(e->getSig());
                if (!to || !from) {
                    m.getErrorHandler().error("Non primitive-type passed as argument", e->getLoc());
                } else if (from->getPrim() == AST::STRING || from->getPrim() == AST::UNDEFINED ) {
                    m.getErrorHandler().error("String or undefined type cannot be casted (non-numeric).", e->getLoc());
                } else {
                    castWarning(m, argsig, *e);
                    e = boost::shared_ptr<AST::Expression>(new AST::CastExpr(e->getLoc(), e));
                    e->setSig(argsig);
                }
            }
            break;
        case 'c':
        case 'S':
            /* these cannot be coerced, they will already be the proper format */
            break;
        default:
            break;
        }
    }

    static bool isNumberStr(const std::string & str) {
        for (std::string::const_iterator c = str.begin(); c!=str.end(); ++c) {
            if (*c < '0' || *c > '9') {
                return false;
            }
        }

        return true;
    }
    ErrorHandler & ModuleTypeCheck::getErrorHandler() {
        return mError;
    }
    Context & ModuleTypeCheck::getContext() {
        return mContext;
    }
    Locals & ModuleTypeCheck::getLocals() {
        return mLocals;
    }
    boost::shared_ptr<AST::TypeSig> ModuleTypeCheck::getSig(AST::Type t) {
        std::map<AST::Type, boost::shared_ptr<AST::TypeSig> >::iterator res = mSigs.find(t);
        if (res == mSigs.end()) {
            boost::shared_ptr<AST::TypeSig> newsig(new AST::TypePrimitive(t));
            mSigs[t] = newsig;
            return newsig;
        } else {
            return res->second;
        }
    }
    ModuleTypeCheck::ModuleTypeCheck(Compiler::Locals & l, Compiler::ErrorHandler & h, Context & c)
        : mLocals(l), mError(h), mContext(c), mStmt(*this) {}


    void ModuleTypeCheck::visit(AST::Module & m) {
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = m.getStmts().begin();
            it != m.getStmts().end();
            ++it) {
            (*it)->accept(mStmt);
        }
    }

    StmtTypeCheck::StmtTypeCheck(ModuleTypeCheck & m)
        : mModule(m), mExpr(m) {
    }

    void StmtTypeCheck::visit(AST::TypeDecl & s) {
        Locals & l = mModule.getLocals();
        l.declare(convertASTType(s.getType()), s.getName());
    }
    void StmtTypeCheck::visit(AST::SetStatement & s) {
        Locals & l = mModule.getLocals();
        std::string str;
        ExprTypeCheck exprIgnoreMethods(mExpr);
        exprIgnoreMethods.setIgnoreCalls();
        exprIgnoreMethods.acceptThis(s.getTarget());

        mExpr.acceptThis(s.getExpr());
        boost::shared_ptr<AST::TypePrimitive> prim_l = boost::dynamic_pointer_cast<AST::TypePrimitive>(s.getTarget()->getSig());
        boost::shared_ptr<AST::TypePrimitive> prim_r = boost::dynamic_pointer_cast<AST::TypePrimitive>(s.getExpr()->getSig());
        if (!prim_l || prim_l->getPrim() == AST::UNDEFINED) {
            mModule.getErrorHandler().error("Invalid set target. Must be a primitive.", s.getLoc());
        } else if (!prim_r || prim_r->getPrim() == AST::UNDEFINED) {
            mModule.getErrorHandler().error("Invalid set expression. Result must be a primitive.", s.getLoc());
        } else if (prim_l->getPrim() == AST::STRING ) {
            std::string newlocal;
            if (s.getTarget()->coerceString(newlocal)) {
                mModule.getErrorHandler().warning("Unknown target in set statement.", s.getLoc());
            } else {
                mModule.getErrorHandler().error("Unable to determine name for set statement.", s.getLoc());
            }
            s.ignoreSet();
        } else if (prim_l != prim_r) {
            argCoerce(mModule, convertASTType(prim_l->getPrim()), s.getExpr());
        }
    }
    void StmtTypeCheck::visit(AST::NoOp & n) {
        /* nothing to do */
    }
    void StmtTypeCheck::visit(AST::WhileStatement & w) {
        mExpr.acceptThis(w.getCond());
        AST::TypeSig & cond_type = *w.getCond()->getSig();

        if (cond_type != *mModule.getSig(AST::BOOL)) {
            mModule.getErrorHandler().warning("Using non-boolean result for condition in while.",w.getLoc());
        }
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = w.getStmts().begin();
            it != w.getStmts().end();
            ++it) {
            (*it)->accept(*this);
        }
    }
    void StmtTypeCheck::visit(AST::IfStatement & i) {
        mExpr.acceptThis(i.getCond());
        AST::TypeSig & cond_type = *i.getCond()->getSig();

        if (cond_type != *mModule.getSig(AST::BOOL) ) {
            mModule.getErrorHandler().warning("Using non-boolean result for condition in if.", i.getLoc());
        }
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
    void StmtTypeCheck::visit(AST::ReturnStatement & f) {
        /* type checks just fine */
    }
    void StmtTypeCheck::visit(AST::StatementExpr & e) {
        mExpr.acceptThis(e.getExpr());
    }

    ExprTypeCheck::ExprTypeCheck(ModuleTypeCheck & m)
        : mModule(m), mIgnoreFunctions(false), mIgnoreInstructions(false), mMutable(true) {}

    void ExprTypeCheck::visit(AST::CastExpr & e) {
        /* cast exprs must be set to their typesig, since that is how they know what to cast to */
        assert(e.getSig());
    }
    void ExprTypeCheck::visit(AST::GlobalVar & e) {
        if (!e.getSig()) {
            char global;
            if ((global = mModule.getContext().getGlobalType(e.getGlobal())) != ' ') {
                e.setSig(convertOldType(mModule, global));
            } else {
                assert(0); /* not a global */
            }
        }
    }

    void ExprTypeCheck::visit(AST::LocalVar & e) {
        if (!e.getSig()) {
            Locals & l = mModule.getLocals();
            char oldtype;
            if ((oldtype = l.getType(::Misc::StringUtils::lowerCase(e.getLocal()))) != ' ') {
                e.setSig(convertOldType(mModule, oldtype));
            } else {
                assert(0); /* not a local */
            }
        }
    }

    void ExprTypeCheck::visit(AST::Journal & e) {
        if (!e.getSig()) {
            e.setSig(mModule.getSig(AST::SHORT));
        }
    }
    void ExprTypeCheck::visit(AST::MemberVar & e) {
        if (!e.getSig()) {
            std::pair<char, bool> member = mModule.getContext().getMemberType(e.getName(), e.getModule());
            boost::shared_ptr<AST::TypePrimitive> type(new AST::TypePrimitive(convertCharType(member.first)));
            AST::shared_typesig sig = boost::dynamic_pointer_cast<AST::TypeSig>(type);
            type->setGlobal();
            e.setSig(sig);
        }
    }
    void ExprTypeCheck::visit(AST::FloatLit & e) {
        e.setSig(mModule.getSig(AST::FLOAT));
    }
    void ExprTypeCheck::visit(AST::LongLit & e) {
        if (e.getValue() > -(1 << 16) && e.getValue() < (1 << 16)) {
            e.setSig(mModule.getSig(AST::SHORT));
        } else {
            e.setSig(mModule.getSig(AST::LONG));
        }
    }

    int countRequiredArgs(const std::string & s) {
        std::string::const_iterator args_it = s.begin();
        int count = 0;
        while (*args_it) {
            if (*args_it == '/') {
                return count;
            }
            count++;
        }
        return count;
    }

    void ExprTypeCheck::doReplace(AST::Expression & e, boost::shared_ptr<AST::Expression> replace) {
        if (mMutable) {
            e.setReplace(replace);
        } else if (replace) {
            e.setSig(replace->getSig());
        }
    }

    void ExprTypeCheck::visit(AST::StringLit & e) {
        Compiler::ScriptReturn ret;
        Compiler::ScriptArgs args;
        bool explicitRef = false;
        Locals & l = mModule.getLocals();
        const Compiler::Extensions * ext = mModule.getContext().getExtensions();
        if (!ext) {
            e.setSig(mModule.getSig(AST::UNDEFINED));
            return;
        }
        int key = ext->searchKeyword(::Misc::StringUtils::lowerCase(e.getValue()));
        char oldtype;
        char global;
        if (key != 0 && !mIgnoreFunctions && ext->isFunction(key, ret, args, explicitRef)) {
            boost::shared_ptr<AST::TypeSig> f(new AST::TypeFunction(args, ret));
            e.setSig(f);
        } else if (key != 0 && !mIgnoreInstructions && ext->isInstruction(key, args, explicitRef)) {
            bool messagebox = false;
            if (::Misc::StringUtils::lowerCase(e.getValue()) == "messagebox") {
                messagebox = true;
            }
            boost::shared_ptr<AST::TypeSig> f(new AST::TypeInstruction(args, messagebox));
            e.setSig(f);
        } else if((oldtype = l.getType(::Misc::StringUtils::lowerCase(e.getValue()))) != ' ') {
            boost::shared_ptr<AST::Expression> local_expr(new AST::LocalVar(e.getLoc(), ::Misc::StringUtils::lowerCase(e.getValue())));
            acceptThis(local_expr);
            doReplace(e, local_expr);
        } else if ((global = mModule.getContext().getGlobalType(e.getValue())) != ' ') {
            boost::shared_ptr<AST::Expression> global_expr(new AST::GlobalVar(e.getLoc(), ::Misc::StringUtils::lowerCase(e.getValue())));
            acceptThis(global_expr);
            doReplace(e, global_expr);
        } else if (isNumberStr(e.getValue())) {
            int num = std::atoi(e.getValue().c_str());
            boost::shared_ptr<AST::Expression> f_expr(new AST::LongLit(e.getLoc(), num));

            acceptThis(f_expr);
            doReplace(e, f_expr);
        } else if (mModule.getContext().isJournalId(e.getValue())) {
            boost::shared_ptr<AST::Expression> f_expr(new AST::Journal(e.getLoc(), e.getValue()));
            acceptThis(f_expr);
            doReplace(e, f_expr);
        } else {
            e.setSig(mModule.getSig(AST::STRING));
        }
    }
    void ExprTypeCheck::visit(AST::RefExpr & e) {
        std::string lstr, rstr;
        if (!e.getOffset()->coerceString(rstr)) {
            mModule.getErrorHandler().error("Not a string type", e.getOffset()->getLoc());
            e.setSig(mModule.getSig(AST::UNDEFINED));
        } else if (!e.isExplicit()) {
            Compiler::ScriptReturn ret;
            Compiler::ScriptArgs args;
            bool explicitRef = false;
            const Compiler::Extensions * ext = mModule.getContext().getExtensions();
            int rkeyword = 0;
            char type;
            if (!ext) {
                mModule.getErrorHandler().error("Missing Extensions", e.getBase()->getLoc());
                e.setSig(mModule.getSig(AST::UNDEFINED));
            } else if (( rkeyword = ext->searchKeyword(::Misc::StringUtils::lowerCase(rstr))) == 0) {
                boost::shared_ptr<AST::Expression> expr = e.getOffset();
                acceptThis(expr);
                doReplace(e, expr);
            } else if (!mIgnoreFunctions && ext->isFunction(rkeyword, ret, args, explicitRef) ) {
                boost::shared_ptr<AST::TypeSig> f(new AST::TypeFunction(args, ret));
                e.setSig(f);
            } else if (!mIgnoreInstructions && ext->isInstruction(rkeyword, args, explicitRef)) {
                bool messagebox = false;
                if (::Misc::StringUtils::lowerCase(rstr) == "messagebox") {
                    messagebox = true;
                    if (e.getSig()) {
                        return; /* don't overwrite existing messagebox sig */
                    }
                }
                boost::shared_ptr<AST::TypeSig> f(new AST::TypeInstruction(args, messagebox));
                e.setSig(f);
            } else {
                boost::shared_ptr<AST::Expression> expr = e.getOffset();
                acceptThis(expr);
                doReplace(e, expr);
            }
        } else if (!e.getBase()->coerceString(lstr)) {
            mModule.getErrorHandler().error("Not a string type", e.getOffset()->getLoc());
            e.setSig(mModule.getSig(AST::UNDEFINED));
        } else if(e.getOp() == AST::DOT) {
            std::pair<char, bool> member;
            bool isMember = false;
            try{
                member = mModule.getContext().getMemberType(rstr, lstr);
                isMember = (member.first != ' ');
            } catch (...) {
                /* ignore */
            }
            if (isMember) {
                boost::shared_ptr<AST::Expression> m_expr(new AST::MemberVar(e.getLoc(), lstr, rstr));
                acceptThis(m_expr);
                doReplace(e, m_expr);
            } else if(isNumberStr(lstr) && isNumberStr(rstr)) {
                /* this is actually a floating point value */
                std::string new_float_str = lstr + "." + rstr;
                boost::shared_ptr<AST::Expression> f_expr(new AST::FloatLit(e.getLoc(), std::atof(new_float_str.c_str())));
                acceptThis(f_expr);
                doReplace(e, f_expr);
            } else {
                e.setSig(mModule.getSig(AST::UNDEFINED));
                mModule.getErrorHandler().error("Invalid Member Reference.", e.getBase()->getLoc());
            }
        } else if (e.getOp() == AST::ARROW) {
            Compiler::ScriptReturn ret;
            Compiler::ScriptArgs args;
            bool explicitRef = true;
            const Compiler::Extensions * ext = mModule.getContext().getExtensions();
            int rkeyword = 0;
            if (!ext) {
                mModule.getErrorHandler().error("Missing Extensions", e.getBase()->getLoc());
            } else if (!mModule.getContext().isId(lstr)) {
                mModule.getErrorHandler().error("Unknown Id of LHS of ->", e.getBase()->getLoc());
            } else if (( rkeyword = ext->searchKeyword(::Misc::StringUtils::lowerCase(rstr))) == 0) {
                mModule.getErrorHandler().error("Unknown Keyword RHS of ->", e.getOffset()->getLoc());
            } else if (ext->isFunction(rkeyword, ret, args, explicitRef)) {
                boost::shared_ptr<AST::TypeSig> f(new AST::TypeFunction(args, ret));
                e.setSig(f);
                if (!explicitRef) {
                    mModule.getErrorHandler().warning("Discarding uneeded explicit reference.", e.getLoc());
                    e.setBase(boost::shared_ptr<AST::StringLit>());
                }
            } else if (ext->isInstruction(rkeyword, args, explicitRef)) {
                boost::shared_ptr<AST::TypeSig> f(new AST::TypeInstruction(args, false));
                e.setSig(f);
                if (!explicitRef) {
                    mModule.getErrorHandler().warning("Discarding uneeded explicit reference.", e.getLoc());
                    e.setBase(boost::shared_ptr<AST::StringLit>());
                }
            } else {
                mModule.getErrorHandler().error("Unknown Reference Type.", e.getLoc());
                e.setSig(mModule.getSig(AST::UNDEFINED));
            }
        } else {
            mModule.getErrorHandler().error("Unhandled Expression Reference.", e.getLoc());
            e.setSig(mModule.getSig(AST::UNDEFINED));
        }
    }
    void ExprTypeCheck::visit(AST::MathExpr & e) {
        ExprTypeCheck noInstr(*this);
        noInstr.setIgnoreInstructions();
        noInstr.acceptThis(e.getLeft());
        noInstr.acceptThis(e.getRight());
        AST::TypeSig & ltype = *e.getLeft()->getSig();
        AST::TypeSig & rtype = *e.getRight()->getSig();

        AST::shared_typesig result_type = binCoerce(mModule, *e.getLeft(), *e.getRight());
        e.setSig(result_type);
    }
    void ExprTypeCheck::visit(AST::LogicExpr & e) {
        ExprTypeCheck noInstr(*this);
        noInstr.setIgnoreInstructions();
        noInstr.acceptThis(e.getLeft());
        noInstr.acceptThis(e.getRight());
        AST::TypeSig & ltype = *e.getLeft()->getSig();
        AST::TypeSig & rtype = *e.getRight()->getSig();

        AST::shared_typesig result_type = binCoerce(mModule, *e.getLeft(), *e.getRight());

        e.setSig(mModule.getSig(AST::BOOL));
    }
    void ExprTypeCheck::visit(AST::NegateExpr & e) {
        ExprTypeCheck noInstr(*this);
        noInstr.setIgnoreInstructions();
        noInstr.acceptThis(e.getExpr());
        AST::TypeSig & t = *e.getExpr()->getSig();

        if (!isNumeric(t)) {
            mModule.getErrorHandler().error("Negation of non-numeric type.", e.getLoc());
        }

        e.setSig(e.getExpr()->getSig());
    }

    boost::shared_ptr<AST::Expression> breakExprItems(boost::shared_ptr<AST::Expression> expr)
    {
        boost::shared_ptr<AST::NegateExpr> neg = boost::dynamic_pointer_cast<AST::NegateExpr>(expr);
        if (neg) {
            return neg->getExpr();
        } else {
            return neg;
        }
    }

     boost::shared_ptr<AST::Expression> ExprTypeCheck::processFn(
        expr_iter & cur_expr, expr_iter & end_expr,
        arg_iter & cur_arg, arg_iter & end_arg, bool toplevel, int optional)
     {
         ExprTypeCheck immute_expr(*this);
         immute_expr.setImmutable();
         immute_expr.acceptThis(*cur_expr);
         TokenLoc loc = (*cur_expr)->getLoc();
         AST::shared_typesig exprsig = (*cur_expr)->getSig();

         boost::shared_ptr<AST::TypeArgs> argtypesig = boost::dynamic_pointer_cast<AST::TypeArgs>(exprsig);
         boost::shared_ptr<AST::Expression> new_expr;
         bool remainder = false;
         if (argtypesig) {
             expr_iter sub_expr_cur = cur_expr + 1;
             expr_iter sub_expr_end = end_expr;
             std::string format;
             if (argtypesig->isMessageBox()
                 && sub_expr_cur != end_expr
                 && (*sub_expr_cur)->coerceString(format) ) {
                 argtypesig->setArgs(formatMessageBox(format));
             }
             arg_iter sub_arg_cur = argtypesig->getArgs().begin();
             arg_iter sub_arg_end = argtypesig->getArgs().end();
             bool optional = false;

             boost::shared_ptr<AST::CallArgs> result = processArgs(sub_expr_cur, sub_expr_end, sub_arg_cur, sub_arg_end, optional);
             ExprTypeCheck ignoremethods(*this);
             if (result) {
                 new_expr = boost::shared_ptr<AST::Expression>(new AST::CallExpr((*cur_expr)->getLoc(), *cur_expr, result));
                 if ((sub_arg_cur == sub_arg_end || optional) && (!toplevel || sub_expr_cur == end_expr)) {
                     cur_expr = sub_expr_cur;
                     return new_expr;
                 } else {
                     /* we have a remainder at the top level */
                     cur_expr = sub_expr_cur;
                     remainder = true;
                 }
             } else {
                 ignoremethods.setIgnoreCalls();
                 /* failed to process, re-evaluate as non-method call */
                 ignoremethods.acceptThis(*cur_expr);

                 loc = (*cur_expr)->getLoc();
                 exprsig = (*cur_expr)->getSig();
             }
         }

         if (cur_expr == end_expr) {
             return new_expr;
         }
         boost::shared_ptr<AST::NegateExpr> isneg = boost::dynamic_pointer_cast<AST::NegateExpr>(*cur_expr);
         if ((remainder && isneg) || !remainder) {
             if (!remainder) {
                 new_expr = *cur_expr;
                 cur_expr++;
             }

             if (cur_expr != end_expr) {
                 boost::shared_ptr<AST::NegateExpr> neg = boost::dynamic_pointer_cast<AST::NegateExpr>(*cur_expr);
                 loc = (*cur_expr)->getLoc();
                 if (neg) {
                     *cur_expr = neg->getExpr();
                     boost::shared_ptr<AST::Expression> sub_expr = processFn(cur_expr, end_expr, cur_arg, end_arg, false, false);
                     if (sub_expr) {
                         boost::shared_ptr<AST::Expression> neg_exp(new AST::NegateExpr(sub_expr->getLoc(), sub_expr));
                         boost::shared_ptr<AST::Expression> big_e(new AST::MathExpr(loc, AST::MINUS, new_expr, sub_expr));
                         new_expr = big_e;
                     } else {
                         return new_expr;
                     }
                 } else {
                     /* TODO: Decide what to do with extra args that aren't part of a call */
                     return new_expr;
                 }
                 cur_expr++;
             }
         } else {
             mModule.getErrorHandler().error("Call does not begin with instruction/function.", loc);
         }

         return new_expr;
     }

     boost::shared_ptr<AST::CallArgs> ExprTypeCheck::processArgs(
        expr_iter & cur_expr, expr_iter & end_expr,
        arg_iter & cur_arg, arg_iter & end_arg, bool & optional)
     {
        std::vector<boost::shared_ptr<AST::Expression> > exprs;
        TokenLoc loc;
        if (cur_expr != end_expr) {
            loc = (*cur_expr)->getLoc();
        }
        if (cur_arg != end_arg
            && *cur_arg == '/') {
            optional = true; /* catch this early, in case of zero optional args */
        }
        while (cur_arg != end_arg
               && cur_expr != end_expr) {
            switch(*cur_arg) {
            case '/':
                optional = true;
                break;
            case 'c':
            case 'S':
                /* no function returns strings, we don't need to do any function analysis. */
                exprs.push_back(*cur_expr);
                cur_expr++;
                break;
            case 'f':
            case 'l':
            case 's':
                {
                    acceptThis(*cur_expr);
                    AST::shared_typesig exprsig = (*cur_expr)->getSig();
                    boost::shared_ptr<AST::TypeArgs> argtypesig = boost::dynamic_pointer_cast<AST::TypeArgs>(exprsig);
                    boost::shared_ptr<AST::Expression> e = *cur_expr;
                    if (argtypesig) {
                        boost::shared_ptr<AST::Expression> fn_e = processFn(cur_expr, end_expr, cur_arg, end_arg, false, optional);
                        if (fn_e) {
                            e = fn_e;
                            acceptThis(fn_e);
                        }
                    } else {
                        cur_expr++;
                    }

                    argCoerce(mModule, *cur_arg, e);
                    exprs.push_back(e);
                }
                break;
            case 'x':
            case 'X':
            case 'z':
                /* ignored args */
                exprs.push_back(*cur_expr);
                cur_expr++;
                break;
            case 'j':
                break;
            }
            cur_arg++;
        }

        bool junk = true;
        while (cur_arg != end_arg && junk) {
            switch(*cur_arg) {
            case 'x':
            case 'X':
            case 'z':
                /* ignored args (unsupplied) */
                cur_arg++;
                break;
            case '/':
                optional = true;
                cur_arg++;
                break;
            default:
                junk = false;
                break;
            }
        }

        return boost::shared_ptr<AST::CallArgs>(new AST::CallArgs(loc, exprs));
    }


    void ExprTypeCheck::visit(AST::ExprItems & e) {
        expr_iter cur_expr = e.getItems().begin();
        expr_iter end_expr = e.getItems().end();
        arg_iter ignore;
        boost::shared_ptr<AST::Expression> newe = processFn(cur_expr, end_expr, ignore, ignore, true, false);
        if (newe) {
            acceptThis(newe);
            doReplace(e, newe);
        } else {
            mModule.getErrorHandler().error("Unable to parse expression.", (*cur_expr)->getLoc());
            e.setSig(mModule.getSig(AST::UNDEFINED));
        }
    }

    void ExprTypeCheck::visit(AST::CallExpr & e) {
        acceptThis(e.getFn());
        std::vector<boost::shared_ptr<AST::Expression> > & items = e.getArgs()->getItems();
        AST::shared_typesig fn_generic = e.getFn()->getSig();
        boost::shared_ptr<AST::TypeFunction> fnsig = boost::dynamic_pointer_cast<AST::TypeFunction>(fn_generic);
        boost::shared_ptr<AST::TypeArgs> fnargs = boost::dynamic_pointer_cast<AST::TypeArgs>(fn_generic);

        if (!fnargs && items.size() == 0) {
            /* this is not a CallExpr */
            doReplace(e, e.getFn());
            return;
        } else if (!fnargs) {
            if (mIgnoreInstructions) {
                mModule.getErrorHandler().error("Invalid context for instruction call.", e.getLoc());
            } else {
                mModule.getErrorHandler().error("Unknown instruction/function call", e.getLoc());
            }
            e.setSig(mModule.getSig(AST::UNDEFINED));
            return;
        }
        std::vector<boost::shared_ptr<AST::Expression> > final_items;
        arg_iter arg_it = fnargs->getArgs().begin();
        int optionals = 0;
        bool entered_optionals = false;
        expr_iter it = items.begin();
        for(;
            it != items.end() && arg_it != fnargs->getArgs().end();
            it++, arg_it++) {
            if (*arg_it == '/') {
                entered_optionals = true;
                arg_it++;
            }
            if (entered_optionals) {
                optionals++;
            }
            switch(*arg_it) {
            case 'c':
            case 'S':
                {
                    std::string val;
                    if (!(*it)->coerceString(val)) {
                        mModule.getErrorHandler().error("Argument is not string", (*it)->getLoc());
                    } else {
                        if (*arg_it == 'c') {
                            val = ::Misc::StringUtils::lowerCase(val);
                        }
                        boost::shared_ptr<AST::Expression> newlit(new AST::StringLit((*it)->getLoc(), val));
                        newlit->setSig(mModule.getSig(AST::STRING));
                        /* do NOT run accept on this newlit, we don't want to reinterpret it */
                        *it = newlit;
                    }
                }
                final_items.push_back(*it);
                break;
            case 'x':
            case 'X':
            case 'j':
            case 'z':
                entered_optionals = true;
                mModule.getErrorHandler().warning("Extra Argument is ignored.", (*it)->getLoc());
                (*it)->setSig(mModule.getSig(AST::UNDEFINED));
                break;
            default:
                acceptThis(*it);
                argCoerce(mModule, *arg_it, (*it));
                final_items.push_back(*it);
                break;
            }
        }
        while(it != items.end()) {
            if (!entered_optionals) {
                mModule.getErrorHandler().error("Extra Argument is ignored.", e.getLoc());
            } else {
                mModule.getErrorHandler().warning("Extra Argument is ignored.", e.getLoc());
                optionals++;
            }
            (*it)->setSig(mModule.getSig(AST::UNDEFINED));
            it++;
        }
        while (arg_it != fnargs->getArgs().end()) {
            switch(*arg_it) {
            case '/':
                entered_optionals = true;
                break;
            case 'z':
            case 'x':
            case 'X':
            case 'j':
                break;
            default:
                if (!entered_optionals) {
                    mModule.getErrorHandler().error("Missing required argument.", e.getLoc());
                }
                break;
            }
            arg_it++;
        }
        e.getArgs()->getItems() = final_items;
        fnargs->setOptionals(optionals);
        if (fnsig) {
            e.setSig(convertOldType(mModule, (*fnsig).getRet()));
        } else {
            e.setSig(mModule.getSig(AST::UNDEFINED));
        }
    }

    void ExprTypeCheck::visit(AST::CallArgs & e) {
        assert(0); /* should never be directly invoked */
    }

    void ExprTypeCheck::acceptThis(boost::shared_ptr<AST::Expression> & e)
    {
        if (e) {
            e->accept(*this);
            if (e->replaceMe()) {
                assert(mMutable);
                e = e->replaceMe();
            }
            assert(e->getSig());
        } else {
            throw SourceException();
            //throw "Undefined Expression";
        }
    }
}
