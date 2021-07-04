#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_STATICSTRINGIDCONSTANTSCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_STATICSTRINGIDCONSTANTSCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

#include "ClangTidy.h"
#include "ClangTidyCheck.h"

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class StaticStringIdConstantsCheck : public ClangTidyCheck
{
    public:
        StaticStringIdConstantsCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_STATICSTRINGIDCONSTANTSCHECK_H
