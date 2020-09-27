#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_COMBINELOCALSINTOPOINTCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_COMBINELOCALSINTOPOINTCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>
#include <unordered_map>
#include <unordered_set>

#include "ClangTidy.h"

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class CombineLocalsIntoPointCheck : public ClangTidyCheck
{
    public:
        CombineLocalsIntoPointCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        using ClangTidyCheck::getLangOpts;

        std::unordered_map<const VarDecl *, std::string> usageReplacements;
        std::unordered_set<const FunctionDecl *> alteredFunctions;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_COMBINELOCALSINTOPOINTCHECK_H
