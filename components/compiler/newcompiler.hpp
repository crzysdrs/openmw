#ifndef NEWCOMPILER_HPP
#define NEWCOMPILER_HPP

#include "driver.hpp"
#include "output.hpp"
#include "errorhandler.hpp"
#include "context.hpp"

namespace Compiler {
    class NewCompiler {
        Driver mDriver;
        ErrorHandler & mError;
        Context & mContext;
        bool mExtState;
        bool compile(AST::Module & mod, Output & output);
        bool doLocals(AST::Module & mod, Output & output);
    public:
        NewCompiler(ErrorHandler & errors, Context & context);
        bool compile_stream(std::istream& in, const std::string& sname, Output & output);
        bool compile_file(const std::string &filename, Output & output);
        bool compile_string(const std::string &input, const std::string& sname, Output &output);
        bool get_locals(std::istream & in, const std::string & sname, Output & output);
        ~NewCompiler();
    };
}
#endif
