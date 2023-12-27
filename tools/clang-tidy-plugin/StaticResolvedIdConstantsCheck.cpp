#include "StaticResolvedIdConstantsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void StaticResolvedIdConstantsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl(
            hasType(
                namedDecl(
                    anyOf(
                        hasName( "resolved_id" ),
                        typedefNameDecl(
                            hasType(
                                hasCanonicalType(
                                    hasDeclaration( namedDecl( hasName( "resolved_id" ) ) )
                                )
                            )
                        )
                    )
                ).bind( "typeDecl" )
            )
        ).bind( "varDecl" ),
        this
    );
}

static void CheckConstructor( StaticResolvedIdConstantsCheck &Check,
                              const MatchFinder::MatchResult &Result )
{
    const VarDecl *ResolvedIdVarDecl = Result.Nodes.getNodeAs<VarDecl>( "varDecl" );
    const TypeDecl *ResolvedIdTypeDecl = Result.Nodes.getNodeAs<TypeDecl>( "typeDecl" );
    if( !ResolvedIdVarDecl || !ResolvedIdTypeDecl ) {
        return;
    }

    const VarDecl *PreviousDecl = dyn_cast_or_null<VarDecl>( ResolvedIdVarDecl->getPreviousDecl() );

    if( PreviousDecl ) {
        // Only complain about each variable once
        return;
    }

    std::string Adjective;

    if( ResolvedIdVarDecl->hasGlobalStorage() ) {
        Adjective = "Global";
    }

    if( ResolvedIdVarDecl->isStaticDataMember() || ResolvedIdVarDecl->isStaticLocal() ) {
        Adjective = "Static";
    }

    if( Adjective.empty() ) {
        return;
    }

    Check.diag(
        ResolvedIdVarDecl->getBeginLoc(),
        "%2 declaration of %0 is dangerous because %1 is a specialization of resolved_id and it "
        "will not update automatically when game data changes.  Consider switching to a string_id."
    ) << ResolvedIdVarDecl << ResolvedIdTypeDecl << Adjective;
}

void StaticResolvedIdConstantsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( *this, Result );
}

} // namespace clang::tidy::cata
