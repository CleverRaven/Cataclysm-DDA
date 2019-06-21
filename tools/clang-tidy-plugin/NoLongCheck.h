#ifndef CATA_TOOLS_CLANG_TIDY_NOLONGCHECK_H
#define CATA_TOOLS_CLANG_TIDY_NOLONGCHECK_H

#include "ClangTidy.h"

namespace clang
{
namespace tidy
{
namespace cata
{

class NoLongCheck : public ClangTidyCheck
{
    public:
        NoLongCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerPPCallbacks( CompilerInstance &Compiler ) override;
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_NOLONGCHECK_H
