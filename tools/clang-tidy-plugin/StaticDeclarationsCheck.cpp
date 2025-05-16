#include "StaticDeclarationsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void StaticDeclarationsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        declaratorDecl(
            hasParent(
                decl( anyOf( translationUnitDecl(), namespaceDecl().bind( "namespace" ) ) )
            )
        ).bind( "decl" ),
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
    const NamespaceDecl *IsInNamespace = Result.Nodes.getNodeAs<NamespaceDecl>( "namespace" );

    const SourceManager &SM = *Result.SourceManager;

    // Ignore cases that are not the first declaration
    if( ThisDecl->getPreviousDecl() ) {
        return;
    }

    // Ignore cases that are in anonymous namespaces
    if( ThisDecl->isInAnonymousNamespace() ) {
        return;
    }

    bool IsStatic = false;
    bool IsConst = false;
    if( const VarDecl *V = dyn_cast<VarDecl>( ThisDecl ) ) {
        IsStatic = V->getStorageClass() == SC_Static;
        IsConst = V->getType().isConstQualified();
    } else if( const FunctionDecl *F = dyn_cast<FunctionDecl>( ThisDecl ) ) {
        IsStatic = F->getStorageClass() == SC_Static;
    }

    // In header files static is forbidden entirely
    if( isInHeader( ThisDecl->getBeginLoc(), SM ) ) {
        if( IsStatic ) {
            if( IsConst ) {
                Check.diag(
                    ThisDecl->getBeginLoc(),
                    "Declaration %0 should not be declared static in a header.  It is const, so "
                    "implicitly has internal linkage and the static keyword can be removed."
                ) << ThisDecl;
            } else {
                Check.diag(
                    ThisDecl->getBeginLoc(),
                    "Declaration %0 should not be declared static in a header.  Either move the "
                    "definition to a cpp file or declare it inline instead."
                ) << ThisDecl;
            }
        }
        return;
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
        std::string Context = IsInNamespace ? "Namespace-level" : "Global";
        Check.diag(
            ThisDecl->getBeginLoc(),
            "%0 declaration of %1 in a cpp file should be static or have a previous "
            "declaration in a header."
        ) << Context << ThisDecl <<
          FixItHint::CreateInsertion( ThisDecl->getSourceRange().getBegin(), "static " );
    }
}

void StaticDeclarationsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace clang::tidy::cata
