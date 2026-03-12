#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_UNITOVERFLOWCHECK_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_UNITOVERFLOWCHECK_H

#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyCheck.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang
{

namespace tidy
{
class ClangTidyContext;

namespace cata
{

class UnitOverflowCheck : public ClangTidyCheck
{
    public:
        UnitOverflowCheck( StringRef Name, ClangTidyContext *Context ) : ClangTidyCheck( Name, Context ) {}
        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;

    protected:
        void emitDiag( const SourceLocation &loc, std::string_view QuantityType,
                       std::string_view ValueType, std::string_view FunctionName, const FixItHint &fix );
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_UNITOVERFLOWCHECK_H
