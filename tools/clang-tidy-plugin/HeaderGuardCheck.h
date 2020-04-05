#ifndef CATA_CLANG_TIDY_PLUGIN_HEADER_GUEAD_CHECK_H
#define CATA_CLANG_TIDY_PLUGIN_HEADER_GUEAD_CHECK_H

#include <ClangTidy.h>

namespace clang
{
namespace tidy
{
namespace cata
{

/// Finds and fixes header guards.
/// Based loosely on the LLVM version.
class CataHeaderGuardCheck : public ClangTidyCheck
{
    public:
        CataHeaderGuardCheck( StringRef Name, ClangTidyContext *Context );
        void registerPPCallbacks( CompilerInstance &Compiler ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_LLVM_HEADER_GUARD_CHECK_H
