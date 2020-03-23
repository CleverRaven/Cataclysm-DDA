#ifndef CATA_TOOLS_CLANG_TIDY_TESTFILENAMECHECK_H
#define CATA_TOOLS_CLANG_TIDY_TESTFILENAMECHECK_H

#include "ClangTidy.h"

namespace clang
{
namespace tidy
{
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

#endif // CATA_TOOLS_CLANG_TIDY_TESTFILENAMECHECK_H
