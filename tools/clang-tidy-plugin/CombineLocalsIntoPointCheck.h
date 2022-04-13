#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_COMBINELOCALSINTOPOINTCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_COMBINELOCALSINTOPOINTCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <llvm/ADT/StringRef.h>
#include <unordered_map>
#include <unordered_set>

#include "ClangTidy.h"
#include "ClangTidyCheck.h"

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

struct DeclRefReplacementData {
    const DeclRefExpr *DeclRef;
    const VarDecl *Decl;
    std::string NewName;
};

struct FunctionReplacementData {
    const VarDecl *XDecl;
    const VarDecl *YDecl;
    const VarDecl *ZDecl;
    std::vector<DeclRefReplacementData> RefReplacements;
    std::vector<FixItHint> Fixits;
};

class CombineLocalsIntoPointCheck : public ClangTidyCheck
{
    public:
        CombineLocalsIntoPointCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;
        using ClangTidyCheck::getLangOpts;

        std::unordered_map<const VarDecl *, std::string> usageReplacements;
        std::unordered_map<const FunctionDecl *, FunctionReplacementData> alteredFunctions;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_COMBINELOCALSINTOPOINTCHECK_H
