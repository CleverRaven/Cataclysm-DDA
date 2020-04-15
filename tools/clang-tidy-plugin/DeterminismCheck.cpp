#include "DeterminismCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{
void DeterminismCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            callee(
                functionDecl(
                    anyOf( hasName( "::rand" ), hasName( "::random" ), hasName( "::drand48" ),
                           hasName( "::erand48" ), hasName( "::lrand48" ), hasName( "::mrand48" ),
                           hasName( "::nrand48" ), hasName( "::jrand48" ) )
                )
            )
        ).bind( "call" ), this );

    // As a heuristic we guess that any record type with a "seed" member
    // function is a random engine.  Later in the code we also check for
    // "result_type".
    using TypeMatcher = clang::ast_matchers::internal::Matcher<Decl>;
    const TypeMatcher IsRngCoreDecl = cxxRecordDecl( hasMethod( hasName( "seed" ) ) );
    const TypeMatcher IsRngTypedefDecl =
        typedefNameDecl( hasType( qualType( hasDeclaration( IsRngCoreDecl ) ) ) );
    const TypeMatcher IsRngTypedef2Decl =
        typedefNameDecl( hasType( qualType( hasDeclaration( IsRngTypedefDecl ) ) ) );
    const TypeMatcher IsRngDecl = anyOf( IsRngCoreDecl, IsRngTypedefDecl, IsRngTypedef2Decl );
    Finder->addMatcher( cxxConstructExpr( hasType( IsRngDecl ) ).bind( "init" ), this );
}

static void CheckCall( DeterminismCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CallExpr *Call = Result.Nodes.getNodeAs<clang::CallExpr>( "call" );
    if( !Call ) {
        return;
    }
    const NamedDecl *Function = dyn_cast<NamedDecl>( Call->getCalleeDecl() );
    if( !Function ) {
        return;
    }
    Check.diag(
        Call->getBeginLoc(),
        "Call to library random function %0.  To ensure determinism for a fixed seed, "
        "use the common tools from rng.h rather than calling library random functions." )
            << Function;
}

static void CheckInit( DeterminismCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CXXConstructExpr *Construction = Result.Nodes.getNodeAs<CXXConstructExpr>( "init" );
    if( !Construction ) {
        return;
    }
    const CXXRecordDecl *ConstructedTypeDecl = Construction->getConstructor()->getParent();
    bool FoundResultType = false;
    for( const Decl *D : ConstructedTypeDecl->decls() ) {
        if( const TypedefNameDecl *ND = dyn_cast<TypedefNameDecl>( D ) ) {
            if( ND->getName() == "result_type" ) {
                FoundResultType = true;
                break;
            }
        }
    }
    if( !FoundResultType ) {
        return;
    }
    const QualType ConstructedType = Construction->getType();
    Check.diag(
        Construction->getBeginLoc(),
        "Construction of library random engine %0.  To ensure determinism for a fixed seed, "
        "use the common tools from rng.h rather than your own random number engines." )
            << ConstructedType;
}

void DeterminismCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckCall( *this, Result );
    CheckInit( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
