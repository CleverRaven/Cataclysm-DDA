#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_NOSTATICTRANSLATIONCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_NOSTATICTRANSLATIONCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class NoStaticTranslationCheck : public ClangTidyCheck
{
    public:
        NoStaticTranslationCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_NOSTATICTRANSLATIONCHECK_H
