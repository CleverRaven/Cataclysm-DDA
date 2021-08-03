#include "AlmostNeverAutoCheck.h"

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

void AlmostNeverAutoCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl(
            hasType( autoType() )
        ).bind( "decl" ),
        this
    );
}

static void CheckDecl( AlmostNeverAutoCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const VarDecl *AutoVarDecl = Result.Nodes.getNodeAs<VarDecl>( "decl" );

    if( !AutoVarDecl ) {
        return;
    }

    if( AutoVarDecl->isImplicit() ) {
        // This is some compiler-generated VarDecl, such as in a range for
        // loop; skip it
        return;
    }

    SourceRange RangeToReplace = AutoVarDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();

    const SourceManager *SM = Result.SourceManager;
    SourceLocation ExpansionBegin = SM->getFileLoc( RangeToReplace.getBegin() );
    if( ExpansionBegin != RangeToReplace.getBegin() ) {
        // This means we're inside a macro expansion
        return;
    }

    PrintingPolicy Policy( LangOptions{} );
    Policy.adjustForCPlusPlus();
    const Expr *Initializer = AutoVarDecl->getAnyInitializer();
    if( !Initializer ) {
        // This happens for one range for loop in CDDA (in test_statistics.h) and I'm not sure why
        // it's different from the others.
        Check.diag(
            RangeToReplace.getBegin(),
            "Avoid auto in declaration of %0."
        ) << AutoVarDecl;
        return;
    }
    QualType AutoTp = Initializer->getType();
    bool WasConst = AutoVarDecl->getType().isLocalConstQualified();
    AutoTp.removeLocalConst();
    std::string TypeStr = AutoTp.getAsString( Policy );

    if( WasConst && AutoTp.getTypePtr()->isPointerType() ) {
        // In the case of 'const auto' we need to bring the beginning forwards
        // to the start of the 'const'.
        RangeToReplace.setBegin( AutoVarDecl->getBeginLoc() );
        AutoTp.addConst();
        TypeStr = AutoTp.getAsString( Policy );
        // In the case of 'auto const' we need to push the end back to the end
        // of the 'const'.  Couldn't find a nice way to do that, so using this
        // super hacky way.
        std::string TextFromEnd( SM->getCharacterData( RangeToReplace.getEnd() ), 10 );
        if( TextFromEnd == "auto const" ) {
            RangeToReplace.setEnd( RangeToReplace.getEnd().getLocWithOffset( 10 ) );
        }
    }

    if( TypeStr == "auto" ) {
        // Not sure what causes this; usually something that returns "auto"
        // like this is a thing we're relatively happy to be auto.  Can revisit
        // if necessary.
        return;
    }

    // Ignore lambdas and "<dependent type>"
    if( TypeStr.find_first_of( "(<" ) != std::string::npos ) {
        return;
    }

    // Heuristic to ignore certain types (e.g. iterators, implementation
    // details) based on their names.  Skipping the first character of each
    // word to avoid worrying about capitalization.
    for( std::string Fragment : {
             "terator", "nternal"
         } ) {
        if( std::search( TypeStr.begin(), TypeStr.end(), Fragment.begin(), Fragment.end() ) !=
            TypeStr.end() ) {
            return;
        }
    }

    const FunctionDecl *ContainingFunc = getContainingFunction( Result, AutoVarDecl );
    if( ContainingFunc &&
        ContainingFunc->getTemplateSpecializationKind() == TSK_ImplicitInstantiation ) {
        return;
    }

    Check.diag(
        RangeToReplace.getBegin(),
        "Avoid auto in declaration of %0."
    ) << AutoVarDecl <<
      FixItHint::CreateReplacement( RangeToReplace, TypeStr );
}

void AlmostNeverAutoCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
