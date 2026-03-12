#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_USESTRINGVIEWCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_USESTRINGVIEWCHECK_H

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>
#include <llvm/ADT/StringRef.h>

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

struct FoundStringViewDecl {
    SourceRange range;
    std::string text;
};

struct FoundStringViewDefn {
    const ParmVarDecl *param;
    const FunctionDecl *func;
    std::string text;
};

class UseStringViewCheck : public ClangTidyCheck
{
    public:
        UseStringViewCheck( StringRef Name, ClangTidyContext *Context )
            : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void onEndOfTranslationUnit() override;

    private:
        std::unordered_multimap<const FunctionDecl *, FoundStringViewDecl> found_decls_;
        std::unordered_map<const FunctionDecl *, FoundStringViewDefn> found_defns_;
        std::unordered_map<const ParmVarDecl *, const FunctionDecl *> function_from_param_;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_USESTRINGVIEWCHECK_H
