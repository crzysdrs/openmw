#include "codegen.hpp"
#include "context.hpp"
#include "extensions.hpp"
#include "extensions0.hpp"
#include "generator.hpp"

#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <components/misc/stringops.hpp>
#include <components/interpreter/interpreter.hpp>

namespace Compiler {

    void replaceInstr(ModuleCodegen & mod, int index, std::vector<Interpreter::Type_Code> & newcode) {
        std::copy(newcode.begin(), newcode.end(), mod.getOutput().getCode().begin() + index);
    }
    Output & ModuleCodegen::getOutput() {
        return mOutput;
    }
    bool ModuleCodegen::isConsole() {
        return mConsole;
    }

    Context & ModuleCodegen::getContext() {
        return mContext;
    }
    ModuleCodegen::ModuleCodegen(Context & c, Output & o)
        : mContext(c), mOutput(o), mStmt(*this), mConsole(true) {}

    void ModuleCodegen::visit(AST::Module & m) {
        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = m.getStmts().begin();
            it != m.getStmts().end();
            ++it) {
            (*it)->accept(mStmt);
        }
    }

    StmtCodegen::StmtCodegen(ModuleCodegen & m)
        : mModule(m), mExprLHS(m, true), mExprRHS(m, false) {
    }

    void StmtCodegen::visit(AST::TypeDecl & s) {
        /*
        Locals & l = mModule.getOutput().getLocals();
        l.declare(convertASTType(s.getType()), s.getName());
        */
    }
    void StmtCodegen::visit(AST::SetStatement & s) {
        char target_type = convertType(*s.getTarget()->getSig());
        char from_type = convertType(*s.getExpr()->getSig());
        if (s.isValid()) {
            mExprLHS.acceptThis(s.getTarget());
        }
        mExprRHS.acceptThis(s.getExpr());
        if (s.isValid()) {
            boost::shared_ptr<AST::GlobalVar> global_target = boost::dynamic_pointer_cast<AST::GlobalVar>(s.getTarget());
            boost::shared_ptr<AST::MemberVar> member_target = boost::dynamic_pointer_cast<AST::MemberVar>(s.getTarget());
            boost::shared_ptr<AST::LocalVar> local_target = boost::dynamic_pointer_cast<AST::LocalVar>(s.getTarget());
            if (global_target) {
                Generator::storeGlobal(
                    mModule.getOutput().getCode(),
                    target_type,
                    from_type
                                       );
            } else if (member_target) {
                boost::shared_ptr<AST::TypePrimitive> primsig = boost::dynamic_pointer_cast<AST::TypePrimitive>(s.getTarget()->getSig());
                Generator::storeMember(
                    mModule.getOutput().getCode(),
                    target_type,
                    primsig->getGlobal()
                                       );
            } else if (local_target) {
                Generator::storeLocal(
                    mModule.getOutput().getCode(),
                    target_type
                                      );
            } else {
                assert(0);
            }
        }
    }
    void StmtCodegen::visit(AST::NoOp & n) {
        /* nothing to do */
    }
    void StmtCodegen::visit(AST::WhileStatement & w) {
        int start = mModule.getOutput().getCode().size();
        mExprRHS.acceptThis(w.getCond());

        std::pair<int, int> cond_jump;
        cond_jump.first = mModule.getOutput().getCode().size();
        Generator::jumpOnZero(mModule.getOutput().getCode(), -1);
        cond_jump.second = mModule.getOutput().getCode().size();

        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = w.getStmts().begin();
            it != w.getStmts().end();
            ++it) {
            (*it)->accept(*this);
        }

        std::pair<int, int> back_jump;
        back_jump.first = mModule.getOutput().getCode().size();
        int back_jump_len = start - back_jump.first;
        assert(back_jump_len < 0);
        Generator::jump(mModule.getOutput().getCode(), back_jump_len);
        back_jump.second = mModule.getOutput().getCode().size();

        std::vector<Interpreter::Type_Code> replace;
        int cond_jump_len = back_jump.second - cond_jump.second + 1;
        assert(cond_jump_len > 0);
        Generator::jumpOnZero(replace, cond_jump_len);
        replaceInstr(mModule, cond_jump.first, replace);
    }

    void StmtCodegen::visit(AST::IfStatement & i) {
        mExprRHS.acceptThis(i.getCond());
        std::pair<int, int> jmp_to_false;
        jmp_to_false.first = mModule.getOutput().getCode().size();
        Generator::jumpOnZero(mModule.getOutput().getCode(), 1);
        jmp_to_false.second = mModule.getOutput().getCode().size();

        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = i.getTrueStmts().begin();
            it != i.getTrueStmts().end();
            ++it) {
            (*it)->accept(*this);
        }
        std::pair<int, int> jmp_to_end;
        jmp_to_end.first = mModule.getOutput().getCode().size();
        Generator::jump(mModule.getOutput().getCode(), 1);
        jmp_to_end.second = mModule.getOutput().getCode().size();

        std::vector<Interpreter::Type_Code> replace;
        int first_jmp_len = jmp_to_end.second - jmp_to_false.second + 1;
        assert(first_jmp_len > 0);
        Generator::jumpOnZero(replace, first_jmp_len);
        replaceInstr(mModule, jmp_to_false.first, replace);

        for(std::vector<boost::shared_ptr<AST::Statement> >::iterator it = i.getFalseStmts().begin();
            it != i.getFalseStmts().end();
            ++it) {
            (*it)->accept(*this);
        }

        int end_of_false = mModule.getOutput().getCode().size();
        std::vector<Interpreter::Type_Code> replace2;
        int jmp_len = end_of_false - jmp_to_end.second + 1;
        assert(jmp_len > 0);
        Generator::jump(replace2, jmp_len);
        replaceInstr(mModule, jmp_to_end.first, replace2);

    }
    void StmtCodegen::visit(AST::ReturnStatement & f)
    {
        Generator::exit(mModule.getOutput().getCode());
    }

    void StmtCodegen::visit(AST::StatementExpr & e)
    {
        char stmt_type = convertType(*e.getExpr()->getSig());
        mExprRHS.acceptThis(e.getExpr());
        if (mModule.isConsole()) {
            switch(stmt_type) {
            case 's':
            case 'l':
                Generator::report(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), "%g");
                break;
            case 'f':
                Generator::report(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), "%f");
                break;
            }
        }
    }
    char coerceShort(char s)
    {
        if (s == 's') {
            return 'l';
        }
        return s;
    }

    ExprCodegen::ExprCodegen(ModuleCodegen & m, bool lhs)
        : mModule(m), mLHS(lhs) {}

    void ExprCodegen::visit(AST::CastExpr & e)
    {
        char totype = convertType(*e.getSig());
        char fromtype = convertType(*e.getExpr()->getSig());

        acceptThis(e.getExpr());

        if (coerceShort(totype) == coerceShort(fromtype)) {
            /* don't bother, there doesn't seem to be a difference between s and l internally */
        } else {
            Generator::convert(mModule.getOutput().getCode(), coerceShort(fromtype), coerceShort(totype));
        }
    }

    void ExprCodegen::visit(AST::GlobalVar & e) {
        char type = convertType(*e.getSig());

        if (mLHS) {
            Generator::pushLiteral
                (
                    mModule.getOutput().getCode(),
                    mModule.getOutput().getLiterals(),
                    e.getGlobal()
                 );
        } else {
            Generator::fetchGlobal(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), type, e.getGlobal());
        }
    }

    void ExprCodegen::visit(AST::LocalVar & e) {
        std::string lower = ::Misc::StringUtils::lowerCase(e.getLocal());
        int index = mModule.getOutput().getLocals().getIndex(lower);
        char type = mModule.getOutput().getLocals().getType(lower);

        if (mLHS) {
            Generator::pushInt(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), index);
        } else {
            Generator::fetchLocal
                (
                    mModule.getOutput().getCode(),
                    type,
                    index
                 );
        }
    }

    void ExprCodegen::visit(AST::Journal & e) {
        Generator::pushString (mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), e.getJournal());
        const Compiler::Extensions * ext = mModule.getContext().getExtensions();
        int keyword = ext->searchKeyword ("getjournalindex");
        std::string empty;
        ext->generateFunctionCode (keyword, mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), empty, 0);
    }

    void ExprCodegen::visit(AST::MemberVar & e) {
        std::pair<char, bool> type = mModule.getContext().getMemberType(e.getName(), e.getModule());
        if (mLHS) {
            Generator::pushString(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), e.getName());
            Generator::pushString(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), e.getModule());
        } else {
            Generator::fetchMember
                (
                    mModule.getOutput().getCode(),
                    mModule.getOutput().getLiterals(),
                    type.first,
                    e.getName(),
                    e.getModule(),
                    !type.second
                 );
        }
    }
    void ExprCodegen::visit(AST::FloatLit & e)
    {
        Generator::pushFloat(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), e.getValue());
    }
    void ExprCodegen::visit(AST::LongLit & e)
    {
        Generator::pushInt(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), e.getValue());
    }

    void ExprCodegen::visit(AST::StringLit & e)
    {
        Generator::pushString(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), e.getValue());
    }

    void ExprCodegen::visit(AST::RefExpr & e)
    {
        AST::shared_typesig callsig = e.getSig();
        boost::shared_ptr<AST::TypeArgs> argsig = boost::dynamic_pointer_cast<AST::TypeArgs>(callsig);
        boost::shared_ptr<AST::TypeFunction> fnsig = boost::dynamic_pointer_cast<AST::TypeFunction>(callsig);
        boost::shared_ptr<AST::TypeInstruction> instrsig = boost::dynamic_pointer_cast<AST::TypeInstruction>(callsig);
        bool isexplicit = e.isExplicit();

        if (!fnsig && !instrsig) {
            assert(0);
            /* how did we get here */
            return;
        }
        int optionals = argsig->getOptionals();

        std::string lstr, rstr;
        e.getOffset()->coerceString(rstr);
        if (isexplicit) {
            e.getBase()->coerceString(lstr);
        }
        const Compiler::Extensions * ext = mModule.getContext().getExtensions();
        std::string lstr_lower = ::Misc::StringUtils::lowerCase(lstr);
        std::string rstr_lower = ::Misc::StringUtils::lowerCase(rstr);
        int keyword = ext->searchKeyword(rstr_lower);
        if (rstr_lower == "menumode") {
            Generator::menuMode(mModule.getOutput().getCode());
        } else if (rstr_lower == "random") {
            Generator::random(mModule.getOutput().getCode());
        } else if (rstr_lower == "startscript") {
            Generator::startScript(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower);
        } else if (rstr_lower == "stopscript") {
            Generator::stopScript(mModule.getOutput().getCode());
        } else if (rstr_lower == "scriptrunning") {
            Generator::scriptRunning(mModule.getOutput().getCode());
        } else if (rstr_lower == "getdistance") {
            Generator::getDistance(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower);
        } else if (rstr_lower == "getsecondspassed") {
            Generator::getSecondsPassed(mModule.getOutput().getCode());
        } else if (rstr_lower == "getdisabled") {
            Generator::getDisabled(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower);
        } else if (rstr_lower == "enable") {
            Generator::enable(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower);
        } else if (rstr_lower == "disable") {
            Generator::disable(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower);
        } else if (rstr_lower == "messagebox") {
            std::string message = "";
            Generator::message(mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), message, optionals, false);
        } else if (rstr_lower == "getsquareroot") {
            Generator::squareRoot(mModule.getOutput().getCode());
        } else if (fnsig) {
            ext->generateFunctionCode(keyword, mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower, optionals);
        } else if (instrsig) {
            ext->generateInstructionCode(keyword, mModule.getOutput().getCode(), mModule.getOutput().getLiterals(), lstr_lower,  optionals);
        } else {
            assert(0); /* how did we get here */
        }
    }

    void ExprCodegen::visit(AST::LogicExpr & e)
    {
        char op = convertLogicOp(e.getOp());
        char ltype = convertType(*e.getLeft()->getSig());
        char rtype = convertType(*e.getRight()->getSig());

        acceptThis(e.getLeft());
        acceptThis(e.getRight());

        Generator::compare(mModule.getOutput().getCode(), op, coerceShort(ltype), coerceShort(rtype));
    }

    void ExprCodegen::visit(AST::MathExpr & e) {
        char ltype = convertType(*e.getLeft()->getSig());
        char rtype = convertType(*e.getRight()->getSig());

        ltype = coerceShort(ltype);
        rtype = coerceShort(rtype);

        acceptThis(e.getLeft());
        acceptThis(e.getRight());

        switch(e.getOp()) {
        case AST::PLUS:
            Generator::add(mModule.getOutput().getCode(), ltype, rtype);
            break;
        case AST::MINUS:
            Generator::sub(mModule.getOutput().getCode(), ltype, rtype);
            break;
        case AST::DIVIDE:
            Generator::div(mModule.getOutput().getCode(), ltype, rtype);
            break;
        case AST::MULT:
            Generator::mul(mModule.getOutput().getCode(), ltype, rtype);
            break;
        default:
            assert(0);
        }
    }
    void ExprCodegen::visit(AST::NegateExpr & e) {
        char etype = convertType(*e.getSig());
        acceptThis(e.getExpr());

        Generator::negate(mModule.getOutput().getCode(), coerceShort(etype));
    }
    void ExprCodegen::visit(AST::ExprItems & e) {
        assert(0); /* ExprItems should have been resolved */
    }

    void ExprCodegen::visit(AST::CallExpr & e) {
        std::vector<boost::shared_ptr<AST::Expression> > items = e.getArgs()->getItems();
        for(std::vector<boost::shared_ptr<AST::Expression> >::reverse_iterator it = items.rbegin();
            it != items.rend();
            ++it) {
            (*it)->accept(*this);
        }
        acceptThis(e.getFn());
    }

    void ExprCodegen::visit(AST::CallArgs & e) {
        assert(0); /* should never be directly invoked */
    }

    void ExprCodegen::acceptThis(boost::shared_ptr<AST::Expression> & e)
    {
        if (e) {
            e->accept(*this);
        } else {
            throw "Undefined Expression";
        }
    }
}
