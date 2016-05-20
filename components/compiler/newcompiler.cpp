#include "newcompiler.hpp"
#include "codegen.hpp"
#include "printast.hpp"
#include "typecheck.hpp"

namespace Compiler {
    NewCompiler::NewCompiler(ErrorHandler & errors, Context & context)
        : mError(errors), mContext(context)
    {

    }

    bool NewCompiler::compile(AST::Module & mod, Output & output)
    {
        Compiler::ModulePrinter print;
        Compiler::ModuleTypeCheck typecheck(mError, mContext);
        Compiler::ModuleCodegen codegen(mContext, output);

        //print.visit(mod);
        typecheck.visit(mod);

        if (!mError.isGood()) {
            return false;
        }

        print.visit(mod);
        codegen.visit(mod);
        return true;
    }
    bool NewCompiler::compile_stream(std::istream& in, const std::string& sname, Output & output)
    {
        if (!mDriver.parse_stream(in, sname)) {
            return false;
        } else if (!compile(mDriver.getResult(), output)) {
            return false;
        }

        return true;
    }
    bool NewCompiler::compile_file(const std::string &filename, Output & output)
    {
        if (!mDriver.parse_file(filename)) {
            return false;
        } else if (!compile(mDriver.getResult(), output)) {
            return false;
        }
        return true;

    }
    bool NewCompiler::compile_string(const std::string &input, const std::string& sname, Output &output)
    {
        if (!mDriver.parse_string(input, sname)) {
            return false;
        } else if (!compile(mDriver.getResult(), output)) {
            return false;
        }
        return true;
    }
}
