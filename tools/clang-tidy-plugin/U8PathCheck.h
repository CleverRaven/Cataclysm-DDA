#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_U8PATHCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_U8PATHCHECK_H

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class U8PathCheck : public ClangTidyCheck
{
    public:
        U8PathCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_U8PATHCHECK_H
