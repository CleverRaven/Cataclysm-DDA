#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_UTILS_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_UTILS_H

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <string>

namespace clang
{
class Decl;
class Stmt;

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

inline auto isXParam()
{
    using namespace clang::ast_matchers;
    return matchesName( "[xX]" );
}

inline bool isPointType( const CXXRecordDecl *R )
{
    if( !R ) {
        return false;
    }
    StringRef name = R->getName();
    return name == "point" || name == "tripoint";
}

// Struct to help identify and construct names of associated points and
// coordinates
class NameConvention
{
    public:
        NameConvention( StringRef xName );

        enum MatchResult {
            XName,
            YName,
            ZName,
            None
        };

        MatchResult Match( StringRef name );

        bool operator!() const {
            return !valid;
        }
    private:
        std::string root;
        bool capital;
        bool atEnd;
        bool valid = true;
};

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_UTILS_H
