#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_SIMPLIFYPOINTCONSTRUCTORSCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_SIMPLIFYPOINTCONSTRUCTORSCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class SimplifyPointConstructorsCheck : public ClangTidyCheck
{
    public:
        SimplifyPointConstructorsCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        using ClangTidyCheck::getLangOpts;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_SIMPLIFYPOINTCONSTRUCTORSCHECK_H
