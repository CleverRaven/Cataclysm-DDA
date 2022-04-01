#include "NoStaticTranslationCheck.h"

#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void NoStaticTranslationCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            hasAncestor( varDecl( hasStaticStorageDuration() ) ),
            callee(
                functionDecl(
                    hasAnyName( "_", "translation_argument_identity", "gettext", "pgettext",
                                "n_gettext", "npgettext" )
                )
            )
        ).bind( "translationCall" ),
        this
    );
}

void NoStaticTranslationCheck::check( const MatchFinder::MatchResult &Result )
{
    const CallExpr *translationCall =
        Result.Nodes.getNodeAs<CallExpr>( "translationCall" );
    if( !translationCall ) {
        return;
    }
    diag(
        translationCall->getBeginLoc(),
        "Gettext calls in static variable initialization will cause text to be "
        "untranslated (global static) or not updated when switching language "
        "(local static). Consider using translation objects (to_translation() or pl_translation()) "
        "or translate_marker(), and translate the text on demand "
        "(with translation::translated() or gettext calls outside static vars)"
    );
}

} // namespace cata
} // namespace tidy
} // namespace clang
