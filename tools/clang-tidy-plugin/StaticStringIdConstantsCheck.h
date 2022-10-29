#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_STATICSTRINGIDCONSTANTSCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_STATICSTRINGIDCONSTANTSCHECK_H

#include <unordered_set>

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

struct FoundDecl {
    const VarDecl *decl;
    CharSourceRange range;
    StringRef text;
};

struct PromotableCall {
    const VarDecl *decl;
    const CXXConstructExpr *construction;
    std::string canonical_name;
    StringRef string_literal_arg;
};

class StaticStringIdConstantsCheck : public ClangTidyCheck
{
    public:
        StaticStringIdConstantsCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;

    private:
        void CheckConstructor( const ast_matchers::MatchFinder::MatchResult &Result );

        std::unordered_set<const VarDecl *> found_decls_set_;
        std::vector<FoundDecl> found_decls_;
        std::unordered_set<unsigned> promotable_set_;
        std::vector<PromotableCall> promotable_;
        bool any_wrong_names_ = false;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_STATICSTRINGIDCONSTANTSCHECK_H
