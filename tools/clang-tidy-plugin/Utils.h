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

template<typename T>
static const FunctionDecl *getContainingFunction(
    const ast_matchers::MatchFinder::MatchResult &Result, const T *Node )
{
    for( const ast_type_traits::DynTypedNode &parent : Result.Context->getParents( *Node ) ) {
        if( const Decl *Candidate = parent.get<Decl>() ) {
            if( const FunctionDecl *ContainingFunction = dyn_cast<FunctionDecl>( Candidate ) ) {
                return ContainingFunction;
            }
            if( const FunctionDecl *ContainingFunction =
                    getContainingFunction( Result, Candidate ) ) {
                return ContainingFunction;
            }
        }
        if( const Stmt *Candidate = parent.get<Stmt>() ) {
            if( const FunctionDecl *ContainingFunction =
                    getContainingFunction( Result, Candidate ) ) {
                return ContainingFunction;
            }
        }
    }

    return nullptr;
}

inline auto isPointType()
{
    using namespace clang::ast_matchers;
    return cxxRecordDecl( anyOf( hasName( "point" ), hasName( "tripoint" ) ) );
}

inline auto isPointConstructor()
{
    using namespace clang::ast_matchers;
    return cxxConstructorDecl( ofClass( isPointType() ) );
}

// This returns a matcher that always matches, but binds "temp" if the
// constructor call is constructing a temporary object.
inline auto testWhetherConstructingTemporary()
{
    using namespace clang::ast_matchers;
    return cxxConstructExpr(
               anyOf(
                   hasParent( materializeTemporaryExpr().bind( "temp" ) ),
                   hasParent(
                       implicitCastExpr( hasParent( materializeTemporaryExpr().bind( "temp" ) ) )
                   ),
                   hasParent( callExpr().bind( "temp" ) ),
                   hasParent( initListExpr().bind( "temp" ) ),
                   anything()
               )
           );
}

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_UTILS_H
