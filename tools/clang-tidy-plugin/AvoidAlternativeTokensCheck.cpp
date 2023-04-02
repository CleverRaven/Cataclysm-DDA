#include "AvoidAlternativeTokensCheck.h"

#include <unordered_set>

#include "Utils.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void AvoidAlternativeTokensCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        binaryOperator(
            hasAnyOperatorName( "&&", "&", "||", "|", "^", "!=", "&=", "|=", "^=" )
        ).bind( "binaryOperator" ),
        this
    );
    Finder->addMatcher(
        unaryOperator(
            hasAnyOperatorName( "~", "!" )
        ).bind( "unaryOperator" ),
        this
    );
}

static void CheckOperator( AvoidAlternativeTokensCheck &Check,
                           const MatchFinder::MatchResult &Result )
{
    const BinaryOperator *binOpExpr = Result.Nodes.getNodeAs<BinaryOperator>( "binaryOperator" );
    const UnaryOperator *unOpExpr = Result.Nodes.getNodeAs<UnaryOperator>( "unaryOperator" );

    if( !binOpExpr && !unOpExpr ) {
        return;
    }

    SourceLocation loc;
    StringRef Replacement;

    if( binOpExpr ) {
        loc = binOpExpr->getOperatorLoc();
        Replacement = binOpExpr->getOpcodeStr();
    } else {
        loc = unOpExpr->getOperatorLoc();
        Replacement = UnaryOperator::getOpcodeStr( unOpExpr->getOpcode() );
    }

    StringRef operatorString = getText( Result, CharSourceRange::getTokenRange( loc, loc ) );

    if( Replacement == operatorString ) {
        return;
    }

    if( operatorString.empty() ) {
        // This happens when the operator is inside a macro expansion
        return;
    }

    if( Replacement == "!=" && operatorString != "not_eq" ) {
        // This happens in the compiler-generated code inside a forach loop and
        // in some other generated code
        return;
    }

    if( Replacement == "!" && operatorString != "not" ) {
        // This happens in some compiler-generated code
        return;
    }

    Check.diag( loc, "Avoid alternative token '%0'; prefer '%1'." )
            << operatorString << Replacement << loc
            << FixItHint::CreateReplacement( loc, Replacement );
}

void AvoidAlternativeTokensCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckOperator( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
