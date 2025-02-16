#include "StaticIntIdConstantsCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void StaticIntIdConstantsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl(
            hasType(
                namedDecl(
                    anyOf(
                        hasName( "int_id" ),
                        typedefNameDecl(
                            hasType(
                                hasCanonicalType(
                                    hasDeclaration( namedDecl( hasName( "int_id" ) ) )
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

static void CheckConstructor( StaticIntIdConstantsCheck &Check,
                              const MatchFinder::MatchResult &Result )
{
    const VarDecl *IntIdVarDecl = Result.Nodes.getNodeAs<VarDecl>( "varDecl" );
    const TypeDecl *IntIdTypeDecl = Result.Nodes.getNodeAs<TypeDecl>( "typeDecl" );
    if( !IntIdVarDecl || !IntIdTypeDecl ) {
        return;
    }

    StringRef VarName = IntIdVarDecl->getName();
    if( VarName.ends_with( "null" ) || VarName.ends_with( "NULL" ) ) {
        // Null constants are OK because they probably don't vary
        return;
    }

    const VarDecl *PreviousDecl = dyn_cast_or_null<VarDecl>( IntIdVarDecl->getPreviousDecl() );

    if( PreviousDecl ) {
        // Only complain about each variable once
        return;
    }

    std::string Adjective;

    if( IntIdVarDecl->hasGlobalStorage() ) {
        Adjective = "Global";
    }

    if( IntIdVarDecl->isStaticDataMember() || IntIdVarDecl->isStaticLocal() ) {
        Adjective = "Static";
    }

    if( Adjective.empty() ) {
        return;
    }

    Check.diag(
        IntIdVarDecl->getBeginLoc(),
        "%2 declaration of %0 is dangerous because %1 is a specialization of int_id and it "
        "will not update automatically when game data changes.  Consider switching to a string_id."
    ) << IntIdVarDecl << IntIdTypeDecl << Adjective;
}

void StaticIntIdConstantsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckConstructor( *this, Result );
}

} // namespace clang::tidy::cata
