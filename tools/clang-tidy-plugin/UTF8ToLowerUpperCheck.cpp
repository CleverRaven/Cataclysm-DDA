#include <algorithm>
#include <cstring>
#include <unordered_set>

#include "UTF8ToLowerUpperCheck.h"

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

void UTF8ToLowerUpperCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher( binaryOperator( isAssignmentOperator(),
                                        hasLHS( hasDescendant( declRefExpr( hasType( asString( "std::string" ) ) ) ) ),
                                        hasRHS( hasDescendant( callExpr( callee( functionDecl( hasAnyName( "toupper", "tolower" ) ) ),
                                                hasArgument( 0, hasDescendant( declRefExpr( hasType(
                                                        asString( "std::string" ) ) ) ) ) ) ) ) ).bind( "assignment" ),
                        this
                      );
}

void UTF8ToLowerUpperCheck::check( const MatchFinder::MatchResult &Result )
{
    const BinaryOperator *op = Result.Nodes.getNodeAs<BinaryOperator>( "assignment" );
    if( op ) {
        diag( op->getBeginLoc(),
              "modifying individual bytes in a UTF-8 string has the risk of corrupting its content."
            );
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
