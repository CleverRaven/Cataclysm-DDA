#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTFILENAMECHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTFILENAMECHECK_H

#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"

namespace clang
{
class CompilerInstance;

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class TestFilenameCheck : public ClangTidyCheck
{
    public:
        TestFilenameCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}

        void registerPPCallbacks( CompilerInstance &Compiler ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_TESTFILENAMECHECK_H
