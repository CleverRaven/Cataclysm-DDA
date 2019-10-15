#include "JsonTranslationInputCheck.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void JsonTranslationInputCheck::registerMatchers( MatchFinder *Finder )
{
    // <translation function>( ... <json input object>.<method>(...) ... )
    Finder->addMatcher(
        callExpr(
            // <json input object>.<method>
            callee( cxxMethodDecl(
                        // <json input object>
                        ofClass( hasAnyName( "JsonIn", "JsonArray", "JsonObject" ) )
                    ) ),
            // <translation function>( ... )
            hasAncestor(
                callExpr( callee( decl( anyOf(
                                            functionDecl(
                                                    hasAnyName( "_", "gettext", "pgettext", "ngettext", "npgettext" )
                                            ).bind( "translationFunc" ),
                                            functionDecl(
                                                    hasAnyName( "to_translation", "pl_translation" )
                                            ) ) ) )
                          // no_translation is ok, it's used to load generated names such as artifact names
                        ).bind( "translationCall" ) )
        ).bind( "jsonInputCall" ),
        this
    );
}

void JsonTranslationInputCheck::check( const MatchFinder::MatchResult &Result )
{
    const CallExpr *jsonInputCall = Result.Nodes.getNodeAs<CallExpr>( "jsonInputCall" );
    const CallExpr *translationCall = Result.Nodes.getNodeAs<CallExpr>( "translationCall" );
    if( jsonInputCall && translationCall ) {
        const FunctionDecl *translationFunc = Result.Nodes.getNodeAs<FunctionDecl>( "translationFunc" );
        if( translationFunc ) {
            diag(
                translationCall->getBeginLoc(),
                "immediately translating a value read from json causes translation "
                "updating issues.  Consider reading into a translation object instead."
            );
            diag(
                jsonInputCall->getBeginLoc(),
                "value read from json",
                DiagnosticIDs::Note
            );
        } else {
            diag(
                translationCall->getBeginLoc(),
                "read translation directly instead of constructing it from "
                "json strings to ensure consistent format in json."
            );
            diag(
                jsonInputCall->getBeginLoc(),
                "string read from json",
                DiagnosticIDs::Note
            );
        }
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
