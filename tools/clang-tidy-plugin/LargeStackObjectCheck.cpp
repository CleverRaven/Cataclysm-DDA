#include "LargeStackObjectCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/AST/TypeLoc.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/Specifiers.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/STLExtras.h>
#include <string>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

namespace clang
{
class MacroArgs;
class MacroDefinition;
}  // namespace clang

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void LargeStackObjectCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher( varDecl().bind( "decl" ), this );
}

static void CheckDecl( LargeStackObjectCheck &Check, const MatchFinder::MatchResult &Result )
{
    const VarDecl *MatchedDecl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    if( !MatchedDecl || !MatchedDecl->getLocation().isValid() ) {
        return;
    }

    if( !MatchedDecl->isLocalVarDeclOrParm() ) {
        return;
    }

    if( MatchedDecl->getName().empty() ) {
        return;
    }

    const Type *T = MatchedDecl->getType().getTypePtr();

    if( T->isReferenceType() || T->isUndeducedAutoType() ) {
        return;
    }

#if defined(LLVM_VERSION_MAJOR) && LLVM_VERSION_MAJOR >= 17
    if( std::optional<CharUnits> VarSize = Result.Context->getTypeSizeInCharsIfKnown( T ) ) {
#else
    if( Optional<CharUnits> VarSize = Result.Context->getTypeSizeInCharsIfKnown( T ) ) {
#endif
        int VarSize_KiB = *VarSize / CharUnits::fromQuantity( 1024 );

        if( VarSize_KiB >= 100 ) {
            Check.diag(
                MatchedDecl->getLocation(),
                "Variable %0 consumes %1KiB of stack space.  Putting such large objects on the "
                "stack risks stack overflow.  Please allocate it on the heap instead." ) <<
                        MatchedDecl << VarSize_KiB;
        }
    }
}

void LargeStackObjectCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace clang::tidy::cata
