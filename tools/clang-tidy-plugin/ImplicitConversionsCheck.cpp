#include "ImplicitConversionsCheck.h"

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
#include "../../src/cata_assert.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void ImplicitConversionsCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxConstructorDecl( unless( isImplicit() ) ).bind( "constructor" ),
        this
    );
}

static void CheckDecl( ImplicitConversionsCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const CXXConstructorDecl *ConstructorDecl =
        Result.Nodes.getNodeAs<CXXConstructorDecl>( "constructor" );

    if( !ConstructorDecl ) {
        return;
    }

    if( ConstructorDecl->isExplicit() || ConstructorDecl->isDeleted() ) {
        return;
    }

    if( ConstructorDecl->getTemplateSpecializationKind() == TSK_ImplicitInstantiation ) {
        return;
    }

    // Ignore cases that are not the first declaration
    if( ConstructorDecl->getPreviousDecl() ) {
        return;
    }

    unsigned NumParams = ConstructorDecl->getNumParams();

    if( NumParams == 0 ) {
        return;
    }

    for( unsigned i = 1; i < NumParams; ++i ) {
        const ParmVarDecl *Param = ConstructorDecl->getParamDecl( i );
        if( !Param->hasDefaultArg() ) {
            return;
        }
    }

    const CXXRecordDecl *Record = ConstructorDecl->getParent();

    if( Record->getTemplateSpecializationKind() == TSK_ImplicitInstantiation ) {
        return;
    }

    const QualType ConstructedType = ConstructorDecl->getThisType().getTypePtr()->getPointeeType();

    QualType FirstArgType = ConstructorDecl->getParamDecl( 0 )->getOriginalType();
    FirstArgType = FirstArgType.getNonReferenceType();
    FirstArgType.removeLocalConst();

    if( ConstructedType.getCanonicalType() == FirstArgType.getCanonicalType() ) {
        return;
    }

    Check.diag(
        ConstructorDecl->getBeginLoc(),
        "Constructor for %0 makes it implicitly convertible from %1.  "
        "Consider marking it explicit."
    ) << ConstructedType << FirstArgType <<
      FixItHint::CreateInsertion( ConstructorDecl->getBeginLoc(), "explicit " );
}

void ImplicitConversionsCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
