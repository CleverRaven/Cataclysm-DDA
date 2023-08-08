#include "NoStaticTranslationCheck.h"

#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void NoStaticTranslationCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            hasAncestor( varDecl( hasStaticStorageDuration() ) ),
            callee(
                decl(
                    anyOf(
                        functionDecl(
                            hasAnyName( "_", "translation_argument_identity", "pgettext",
                                        "n_gettext", "npgettext" )
                        ),
                        cxxMethodDecl(
                            ofClass( hasName( "translation" ) ),
                            hasAnyName( "translated", "translated_eq", "translated_ne",
                                        "translated_lt", "translated_gt", "translated_le", "translated_ge" )
                        ),
                        cxxMethodDecl(
                            ofClass( hasAnyName( "translated_less", "translated_greater",
                                     "translated_less_equal", "translated_greater_equal",
                                     "translated_equal_to", "translated_not_equal_to" ) ),
                            hasOverloadedOperatorName( "()" )
                        )
                    )
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
        "Translation functions should not be called when initializing a static variable.  "
        "See the `### Static string variables` section in `doc/TRANSLATING.md` for details."
    );
}

} // namespace clang::tidy::cata
