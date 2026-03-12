#include "UseMdarrayCheck.h"

#include <cassert>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <string>
#include <tuple>
#include <utility>

#include "Utils.h"
#include "clang/AST/OperationKinds.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void UseMdarrayCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        declaratorDecl(
            hasType( arrayType( hasElementType( arrayType() ) ) )
        ).bind( "decl" ),
        this
    );
}

static void CheckDecl( UseMdarrayCheck &Check, const MatchFinder::MatchResult &Result )
{
    const DeclaratorDecl *ArrayDecl = Result.Nodes.getNodeAs<DeclaratorDecl>( "decl" );
    if( !ArrayDecl ) {
        return;
    }

    Check.diag(
        ArrayDecl->getBeginLoc(),
        "Consider using cata::mdarray rather than a raw 2D C array in declaration of %0."
    ) << ArrayDecl;
}

void UseMdarrayCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace clang::tidy::cata
