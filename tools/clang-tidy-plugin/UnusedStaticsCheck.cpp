#include "UnusedStaticsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void UnusedStaticsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl(
            anyOf( hasParent( namespaceDecl() ), hasParent( translationUnitDecl() ) ),
            hasStaticStorageDuration()
        ).bind( "decl" ),
        this
    );
    Finder->addMatcher(
        declRefExpr( to( varDecl().bind( "decl" ) ) ).bind( "ref" ),
        this
    );
}

void UnusedStaticsCheck::check( const MatchFinder::MatchResult &Result )
{
    const VarDecl *ThisDecl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    if( !ThisDecl ) {
        return;
    }

    const DeclRefExpr *Ref = Result.Nodes.getNodeAs<DeclRefExpr>( "ref" );
    if( Ref ) {
        used_decls_.insert( ThisDecl );
    }

    const SourceManager &SM = *Result.SourceManager;

    // Ignore cases in header files
    if( !SM.isInMainFile( ThisDecl->getBeginLoc() ) ) {
        return;
    }

    // Ignore cases that are not the first declaration
    if( ThisDecl->getPreviousDecl() ) {
        return;
    }

    // Ignore cases that are not static linkage
    Linkage Lnk = ThisDecl->getFormalLinkage();
    if( Lnk != Linkage::Internal && Lnk != Linkage::UniqueExternal ) {
        return;
    }

    SourceLocation DeclLoc = ThisDecl->getBeginLoc();
    SourceLocation ExpansionLoc = SM.getFileLoc( DeclLoc );
    if( DeclLoc != ExpansionLoc ) {
        // We are inside a macro expansion
        return;
    }

    decls_.push_back( ThisDecl );
}

void UnusedStaticsCheck::onEndOfTranslationUnit()
{
    for( const VarDecl *V : decls_ ) {
        if( used_decls_.count( V ) ) {
            continue;
        }

        diag( V->getBeginLoc(), "Variable %0 declared but not used." ) << V;
    }
}

} // namespace clang::tidy::cata
