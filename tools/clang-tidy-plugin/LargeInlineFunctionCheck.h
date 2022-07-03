#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_LARGEINLINEFUNCTIONCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_LARGEINLINEFUNCTIONCHECK_H

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

class LargeInlineFunctionCheck : public ClangTidyCheck
{
    public:
        LargeInlineFunctionCheck( StringRef Name, ClangTidyContext *Context );
        void storeOptions( ClangTidyOptions::OptionMap &Opts ) override;
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;

        unsigned getMaxStatements() const {
            return MaxStatements;
        }
    private:
        const unsigned MaxStatements;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_LARGEINLINEFUNCTIONCHECK_H
