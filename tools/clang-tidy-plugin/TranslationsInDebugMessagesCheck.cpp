#include "TranslationsInDebugMessagesCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/DiagnosticIDs.h>

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void TranslationsInDebugMessagesCheck::registerMatchers( MatchFinder *Finder )
{
    // <translation function>( ... <json input object>.<method>(...) ... )
    Finder->addMatcher(
        callExpr(
            callee(
                functionDecl(
                    anyOf(
                        functionDecl(
                            hasAnyName( "_", "translation_argument_identity", "gettext", "pgettext",
                                        "ngettext", "npgettext", "to_translation", "pl_translation",
                                        "no_translation" )
                        ),
                        functionDecl(
                            hasName( "to_string" )
                        ).bind( "to_string" )
                    )
                )
            ),
            hasAncestor( callExpr( callee( functionDecl( matchesName( "add_msg_debug.*" ) ) ) ) )
        ).bind( "translationCall" ),
        this
    );
}

void TranslationsInDebugMessagesCheck::check( const MatchFinder::MatchResult &Result )
{
    const CallExpr *translationCall = Result.Nodes.getNodeAs<CallExpr>( "translationCall" );
    const FunctionDecl *toStringDecl = Result.Nodes.getNodeAs<FunctionDecl>( "to_string" );
    if( !translationCall ) {
        return;
    }

    if( toStringDecl ) {
        diag(
            translationCall->getBeginLoc(),
            "string arguments to debug message functions should not be translated, because this "
            "is an unnecessary performance cost.  This call to to_string might involve a "
            "translation; consider using to_string_writable instead."
        );
    } else {
        diag(
            translationCall->getBeginLoc(),
            "string arguments to debug message functions should not be translated, because this "
            "is an unnecessary performance cost."
        );
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
