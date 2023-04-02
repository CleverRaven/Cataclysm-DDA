#include "OtMatchCheck.h"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <climits>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Casting.h>
#include <string>

#include "Utils.h"
#include "clang/Basic/OperatorKinds.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void OtMatchCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            callee( namedDecl( hasName( "is_ot_match" ) ) ),
            hasArgument( 0, expr().bind( "string_arg" ) ),
            hasArgument( 1, expr().bind( "ter_arg" ) ),
            hasArgument( 2, declRefExpr( to( enumConstantDecl().bind( "enum_decl" ) ) ) )
        ).bind( "call" ),
        this
    );
}

static void CheckCall( OtMatchCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CallExpr *Call = Result.Nodes.getNodeAs<CallExpr>( "call" );
    const Expr *StringArg = Result.Nodes.getNodeAs<Expr>( "string_arg" );
    const Expr *TerArg = Result.Nodes.getNodeAs<Expr>( "ter_arg" );
    const EnumConstantDecl *EnumDecl = Result.Nodes.getNodeAs<EnumConstantDecl>( "enum_decl" );
    if( !Call || !StringArg || !TerArg ) {
        return;
    }

    SourceRange CallRange = Call->getSourceRange();

    if( EnumDecl->getName() == "type" ) {
        std::string Replacement =
            ( "( ( " + getText( Result, TerArg ) + " )->get_type_id() == oter_type_str_id( " +
              getText( Result, StringArg ) + " ) )" ).str();
        Check.diag( Call->getBeginLoc(),
                    "Call to is_ot_match with fixed ot_match_type 'type'.  For better efficientcy, "
                    "instead compare the type directly." ) <<
                            FixItHint::CreateReplacement( CallRange, Replacement );
    } else if( EnumDecl->getName() == "exact" ) {
        std::string Replacement =
            ( "( " + getText( Result, TerArg ) + " == oter_str_id( " +
              getText( Result, StringArg ) + " ).id() )" ).str();
        Check.diag( Call->getBeginLoc(),
                    "Call to is_ot_match with fixed ot_match_type 'exact'.  For better "
                    "efficientcy, instead compare the id directly." ) <<
                            FixItHint::CreateReplacement( CallRange, Replacement );
    }
}

void OtMatchCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckCall( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
