#ifndef CATA_TOOLS_CLANG_TIDY_TEXTSTYLECHECK_H
#define CATA_TOOLS_CLANG_TIDY_TEXTSTYLECHECK_H

#include "ClangTidy.h"

namespace clang
{
namespace tidy
{
namespace cata
{

class TextStyleCheck : public ClangTidyCheck
{
    public:
        TextStyleCheck( StringRef Name, ClangTidyContext *Context );

        void registerMatchers( ast_matchers::MatchFinder *Finder ) override;
        void check( const ast_matchers::MatchFinder::MatchResult &Result ) override;
        void storeOptions( ClangTidyOptions::OptionMap &Opts ) override;

    private:
        bool EscapeUnicode;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_TEXTSTYLECHECK_H
