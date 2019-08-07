#ifndef CATA_TOOLS_CLANG_TIDY_UTILS_H
#define CATA_TOOLS_CLANG_TIDY_UTILS_H

#include "clang/ASTMatchers/ASTMatchFinder.h"

namespace clang
{
namespace tidy
{
namespace cata
{

inline StringRef getText(
    const ast_matchers::MatchFinder::MatchResult &Result, SourceRange Range )
{
    return Lexer::getSourceText( CharSourceRange::getTokenRange( Range ),
                                 *Result.SourceManager,
                                 Result.Context->getLangOpts() );
}

template <typename T>
inline StringRef getText( const ast_matchers::MatchFinder::MatchResult &Result, T *Node )
{
    if( const CXXDefaultArgExpr *Default = dyn_cast<clang::CXXDefaultArgExpr>( Node ) ) {
        return getText( Result, Default->getExpr() );
    }
    return getText( Result, Node->getSourceRange() );
}

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_UTILS_H
