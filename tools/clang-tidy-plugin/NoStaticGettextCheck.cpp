#include "NoStaticGettextCheck.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void NoStaticGettextCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            hasAncestor( varDecl( hasStaticStorageDuration() ) ),
            callee( functionDecl( hasAnyName( "_", "gettext", "pgettext", "ngettext", "npgettext" ) ) )
        ).bind( "gettextCall" ),
        this
    );
}

void NoStaticGettextCheck::check( const MatchFinder::MatchResult &Result )
{
    const CallExpr *gettextCall =
        Result.Nodes.getNodeAs<CallExpr>( "gettextCall" );
    if( !gettextCall ) {
        return;
    }
    diag(
        gettextCall->getBeginLoc(),
        "Gettext calls in static variable initialization will cause text to be "
        "untranslated (global static) or not updated when switching language "
        "(local static). Consider using translation objects (to_translation()) "
        "or translate_marker(), and translate the text on demand "
        "(with translation::translated() or gettext calls outside static vars)"
    );
}

} // namespace cata
} // namespace tidy
} // namespace clang
