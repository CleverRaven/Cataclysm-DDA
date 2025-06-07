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
    const ast_matchers::MatchFinder::MatchResult &Result, CharSourceRange Range )
{
    return Lexer::getSourceText( Range, *Result.SourceManager, Result.Context->getLangOpts() );
}

inline StringRef getText(
    const ast_matchers::MatchFinder::MatchResult &Result, SourceRange Range )
{
    return getText( Result, CharSourceRange::getTokenRange( Range ) );
}

template <typename T>
inline StringRef getText( const ast_matchers::MatchFinder::MatchResult &Result, T *Node )
{
    if( const CXXDefaultArgExpr *Default = dyn_cast<clang::CXXDefaultArgExpr>( Node ) ) {
        return getText( Result, Default->getExpr() );
    }
    return getText( Result, Node->getSourceRange() );
}

template<typename T, typename U>
static const T *getParent( const ast_matchers::MatchFinder::MatchResult &Result, const U *Node )
{
    for( const DynTypedNode &parent : Result.Context->getParents( *Node ) ) {
        if( const T *Candidate = parent.get<T>() ) {
            return Candidate;
        }
    }

    return nullptr;
}

template<typename T>
static const FunctionDecl *getContainingFunction(
    const ast_matchers::MatchFinder::MatchResult &Result, const T *Node )
{
    for( const DynTypedNode &parent : Result.Context->getParents( *Node ) ) {
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

inline bool isInHeader( const SourceLocation &loc, const SourceManager &SM )
{
    StringRef Filename = SM.getFilename( loc );
    // The .h.tmp.cpp catches the test case; that's the style of filename used
    // by lit.
    return !SM.isInMainFile( loc ) || Filename.ends_with( ".h.tmp.cpp" );
}

inline bool isPointType( const CXXRecordDecl *R )
{
    if( !R ) {
        return false;
    }
    StringRef name = R->getName();
    return name == "point" || name == "tripoint";
}

inline auto isPointType()
{
    using namespace clang::ast_matchers;
    return cxxRecordDecl( anyOf( hasName( "point" ), hasName( "tripoint" ) ) );
}

inline auto isPointOrCoordPointType()
{
    using namespace clang::ast_matchers;
    return cxxRecordDecl(
               anyOf( hasName( "point" ), hasName( "tripoint" ), hasName( "coord_point" ) )
           );
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

inline auto testWhetherParentIsVarDecl()
{
    using namespace clang::ast_matchers;
    return expr(
               anyOf(
                   hasParent( varDecl().bind( "parentVarDecl" ) ),
                   anything()
               )
           );
}

inline auto testWhetherGrandparentIsTranslationUnitDecl()
{
    using namespace clang::ast_matchers;
    return expr(
               anyOf(
                   hasParent(
                       varDecl(
                           hasParent( translationUnitDecl().bind( "grandparentTranslationUnit" ) )
                       )
                   ),
                   anything()
               )
           );
}

inline auto isXParam()
{
    using namespace clang::ast_matchers;
    return matchesName( "[xX]" );
}

inline auto isYParam()
{
    using namespace clang::ast_matchers;
    return matchesName( "[yY]" );
}

bool isPointMethod( const FunctionDecl *d );

// Struct to help identify and construct names of associated points and
// coordinates
class NameConvention
{
    public:
        explicit NameConvention( StringRef xName );

        enum MatchResult {
            XName,
            YName,
            ZName,
            None
        };

        MatchResult Match( StringRef name ) const;

        bool operator!() const {
            return !valid;
        }

        const std::string &getRoot() const {
            return root;
        }
    private:
        std::string root;
        bool capital;
        bool atEnd;
        bool valid = true;
};

template<typename T, typename U>
inline size_t HashCombine( const T &t, const U &u )
{
    std::hash<T> t_hash;
    std::hash<U> u_hash;
    size_t result = t_hash( t );
    result ^= 0x9e3779b9 + ( result << 6 ) + ( result >> 2 );
    result ^= u_hash( u );
    return result;
}

template<typename T0, typename... T>
std::string StrCat( T0 &&a0, T &&...a )
{
    std::string result( std::forward<T0>( a0 ) );
    // Using initializer list as a poor man's fold expression until C++17.
    static_cast<void>(
        std::array<bool, sizeof...( T )> { ( result.append( std::forward<T>( a ) ), false )... } );
    return result;
}

} // namespace cata
} // namespace tidy
} // namespace clang

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_UTILS_H
