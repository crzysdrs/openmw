#include "scriptmanagerimp.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>
#include <algorithm>

#include <components/esm/loadscpt.hpp>

#include <components/misc/stringops.hpp>

#include <components/compiler/scanner.hpp>
#include <components/compiler/context.hpp>
#include <components/compiler/exception.hpp>
#include <components/compiler/quickfileparser.hpp>
#include <components/compiler/output.hpp>
#include <components/compiler/newcompiler.hpp>

#include <components/compiler/nullerrorhandler.hpp>

#include "../mwworld/esmstore.hpp"

#include "extensions.hpp"

namespace MWScript
{
    ScriptManager::ScriptManager (const MWWorld::ESMStore& store, bool verbose,
        Compiler::Context& compilerContext, int warningsMode,
        const std::vector<std::string>& scriptBlacklist, bool newcompiler)
    : mErrorHandler (std::cerr), mStore (store), mVerbose (verbose),
      mCompilerContext (compilerContext), mParser (mErrorHandler, mCompilerContext),
      mOpcodesInstalled (false), mGlobalScripts (store), mNewCompiler(newcompiler)
    {
        mErrorHandler.setWarningsMode (warningsMode);

        mScriptBlacklist.resize (scriptBlacklist.size());

        std::transform (scriptBlacklist.begin(), scriptBlacklist.end(),
            mScriptBlacklist.begin(), Misc::StringUtils::lowerCase);
        std::sort (mScriptBlacklist.begin(), mScriptBlacklist.end());
    }

    bool ScriptManager::compile (const std::string& name)
    {
        mErrorHandler.reset();
        Compiler::NullErrorHandler noError;

        Compiler::ErrorHandler & errorhandler = noError;

        if (!mNewCompiler) {
            mParser.reset();


            if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
                {
                    if (mVerbose)
                        std::cout << "compiling script: " << name << std::endl;

                    bool Success = true;
                    try
                        {
                            std::istringstream input (script->mScriptText);

                            Compiler::Scanner scanner (errorhandler, input, mCompilerContext.getExtensions());

                            scanner.scan (mParser);

                            if (!errorhandler.isGood())
                                Success = false;
                        }
                    catch (const Compiler::SourceException&)
                        {
                            // error has already been reported via error handler
                            Success = false;
                        }
                    catch (const std::exception& error)
                        {
                            std::cerr << "An exception has been thrown: " << error.what() << std::endl;
                            Success = false;
                        }

                    if (!Success)
                        {
                            std::cerr
                                << "compiling failed: " << name << std::endl;
                            if (mVerbose)
                                std::cerr << script->mScriptText << std::endl << std::endl;
                        }

                    if (Success)
                        {
                            std::vector<Interpreter::Type_Code> code;
                            mParser.getCode (code);
                            mScripts.insert (std::make_pair (name, std::make_pair (code, mParser.getLocals())));

                            return true;
                        }
                }

        } else {

            if (const ESM::Script *script = mStore.get<ESM::Script>().find (name))
                {
                    if (mVerbose)
                        std::cout << "compiling script: " << name << std::endl;

#if 0
                    try
                        {
#endif
                            std::istringstream input (script->mScriptText);
                            Compiler::NewCompiler compiler(errorhandler, mCompilerContext);
                            Compiler::Locals locals;
                            Compiler::Output output(locals);
                            if (compiler.compile_stream(input, name, output)) {
                                std::vector<Interpreter::Type_Code> code;
                                output.getCode(code);
                                mScripts.insert (std::make_pair (name, std::make_pair (code, output.getLocals())));
                                return true;
                            } else {
                                std::cout << "Failed on " << name << std::endl;
                                return false;
                            }
#if 0
                        } catch (...) {
                        std::cout << "Horrific error" << std::endl;
                        return false;
                    }
#endif
                }
        }

        return false;
    }

    void ScriptManager::run (const std::string& name, Interpreter::Context& interpreterContext)
    {
        // compile script
        ScriptCollection::iterator iter = mScripts.find (name);

        if (iter==mScripts.end())
        {
            if (!compile (name))
            {
                // failed -> ignore script from now on.
                std::vector<Interpreter::Type_Code> empty;
                mScripts.insert (std::make_pair (name, std::make_pair (empty, Compiler::Locals())));
                return;
            }

            iter = mScripts.find (name);
            assert (iter!=mScripts.end());
        }

        // execute script
        if (!iter->second.first.empty())
            try
            {
                if (!mOpcodesInstalled)
                {
                    installOpcodes (mInterpreter);
                    mOpcodesInstalled = true;
                }

                mInterpreter.run (&iter->second.first[0], iter->second.first.size(), interpreterContext);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Execution of script " << name << " failed:" << std::endl;
                std::cerr << e.what() << std::endl;

                iter->second.first.clear(); // don't execute again.
            }
    }

    std::pair<int, int> ScriptManager::compileAll()
    {
        int count = 0;
        int success = 0;

        const MWWorld::Store<ESM::Script>& scripts = mStore.get<ESM::Script>();

        for (MWWorld::Store<ESM::Script>::iterator iter = scripts.begin();
            iter != scripts.end(); ++iter)
            if (!std::binary_search (mScriptBlacklist.begin(), mScriptBlacklist.end(),
                Misc::StringUtils::lowerCase (iter->mId)))
            {
                ++count;

                if (compile (iter->mId))
                    ++success;
            }

        return std::make_pair (count, success);
    }

    const Compiler::Locals& ScriptManager::getLocals (const std::string& name)
    {
        std::string name2 = Misc::StringUtils::lowerCase (name);

        {
            ScriptCollection::iterator iter = mScripts.find (name2);

            if (iter!=mScripts.end())
                return iter->second.second;
        }

        {
            std::map<std::string, Compiler::Locals>::iterator iter = mOtherLocals.find (name2);

            if (iter!=mOtherLocals.end())
                return iter->second;
        }

        if (const ESM::Script *script = mStore.get<ESM::Script>().search (name2))
        {
            if (mVerbose)
                std::cout
                    << "scanning script for local variable declarations: " << name2
                    << std::endl;

            Compiler::Locals locals;

            std::istringstream stream (script->mScriptText);
            Compiler::QuickFileParser parser (mErrorHandler, mCompilerContext, locals);
            Compiler::Scanner scanner (mErrorHandler, stream, mCompilerContext.getExtensions());
            scanner.scan (parser);

            std::map<std::string, Compiler::Locals>::iterator iter =
                mOtherLocals.insert (std::make_pair (name2, locals)).first;

            return iter->second;
        }

        throw std::logic_error ("script " + name + " does not exist");
    }

    GlobalScripts& ScriptManager::getGlobalScripts()
    {
        return mGlobalScripts;
    }
}
