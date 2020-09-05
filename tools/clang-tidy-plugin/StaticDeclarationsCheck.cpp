#include "StaticDeclarationsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void StaticDeclarationsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        declaratorDecl( hasParent( translationUnitDecl() ) ).bind( "decl" ),
        this
    );
}

static void CheckDecl( StaticDeclarationsCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const DeclaratorDecl *ThisDecl = Result.Nodes.getNodeAs<DeclaratorDecl>( "decl" );
    if( !ThisDecl ) {
        return;
    }

    const SourceManager &SM = *Result.SourceManager;

    // Ignore cases in header files for now
    if( !SM.isInMainFile( ThisDecl->getBeginLoc() ) ) {
        return;
    }

    // Ignore cases that are not the first declaration
    if( ThisDecl->getPreviousDecl() ) {
        return;
    }

    bool IsStatic = false;
    if( const VarDecl *V = dyn_cast<VarDecl>( ThisDecl ) ) {
        IsStatic = V->getStorageClass() == SC_Static;
    } else if( const FunctionDecl *F = dyn_cast<FunctionDecl>( ThisDecl ) ) {
        IsStatic = F->getStorageClass() == SC_Static;
    }

    // Ignore if already static.
    if( IsStatic ) {
        return;
    }

    // Ignore main
    if( ThisDecl->getNameAsString() == "main" ) {
        return;
    }

    const char *DeclSource = SM.getCharacterData( ThisDecl->getSourceRange().getBegin() );
    bool IsExtern = std::string( DeclSource, 7 ) == "extern ";

    if( IsExtern ) {
        Check.diag(
            ThisDecl->getBeginLoc(),
            "Prefer including a header to making a local extern declaration of %0."
        ) << ThisDecl;
    } else {
        Check.diag(
            ThisDecl->getBeginLoc(),
            "Global declaration of %0 in a cpp file should be static or have a previous "
            "declaration in a header."
        ) << ThisDecl <<
          FixItHint::CreateInsertion( ThisDecl->getSourceRange().getBegin(), "static " );
    }
}

void StaticDeclarationsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
