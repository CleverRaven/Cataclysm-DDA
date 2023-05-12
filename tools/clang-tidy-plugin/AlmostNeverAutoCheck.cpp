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

namespace clang::tidy::cata
{

void AlmostNeverAutoCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl(
            // Exclude lambda captures with initializers
            unless( hasParent( lambdaExpr() ) ),
            anyOf(
                varDecl( hasType( autoType() ) ),
                varDecl( hasType( references( autoType() ) ) )
            )
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

    const FunctionDecl *ContainingFunc = getContainingFunction( Result, AutoVarDecl );
    if( ContainingFunc &&
        ContainingFunc->getTemplateSpecializationKind() == TSK_ImplicitInstantiation ) {
        return;
    }

    QualType VarDeclType = AutoVarDecl->getType();
    const Expr *Initializer = AutoVarDecl->getAnyInitializer();

    if( !Initializer ) {
        // This happens for some range for loops in CDDA (e.g. test_statistics.h) and I'm not sure
        // why they are different from the others.
        if( VarDeclType.getTypePtr()->isDependentType() ) {
            // Probably shouldn't replace this one.
            return;
        }
        Check.diag(
            RangeToReplace.getBegin(),
            "Avoid auto in declaration of %0."
        ) << AutoVarDecl;
        return;
    }

    QualType AutoTp = Initializer->getType();
    bool WasRRef = VarDeclType.getTypePtr()->isRValueReferenceType();
    bool WasLRef = VarDeclType.getTypePtr()->isLValueReferenceType();
    VarDeclType = VarDeclType.getNonReferenceType();
    bool WasConst = VarDeclType.isLocalConstQualified();
    bool IsConstexpr = AutoVarDecl->isConstexpr();
    if( WasConst && !IsConstexpr ) {
        AutoTp.addConst();
    }

    if( VarDeclType.getTypePtr()->isArrayType() ) {
        return;
    }

    PrintingPolicy Policy( LangOptions{} );
    Policy.adjustForCPlusPlus();
    std::string TypeStr = AutoTp.getAsString( Policy );

    std::string DesugaredTypeStr;
    // Test stripped type is not null to avoid a crash in
    // QualType::getSplitDesugaredType in clang/lib/AST/Type.cpp
    if( QualifierCollector().strip( AutoTp ) ) {
        QualType DesugaredAutoTp = AutoTp.getDesugaredType( *Result.Context );
        DesugaredTypeStr = DesugaredAutoTp.getAsString( Policy );
    }

    // In the case of 'const auto' we need to bring the beginning forwards
    // to the start of the 'const'.
    SourceLocation MaybeNewBegin = RangeToReplace.getBegin().getLocWithOffset( -6 );
    std::string TextBeforeBegin( SM->getCharacterData( MaybeNewBegin ), 10 );
    if( TextBeforeBegin == "const auto" ) {
        RangeToReplace.setBegin( MaybeNewBegin );
    }
    // In the case of 'auto const' we need to push the end back to the end
    // of the 'const'.  Couldn't find a nice way to do that, so using this
    // super hacky way.
    std::string TextFromEnd( SM->getCharacterData( RangeToReplace.getEnd() ), 10 );
    if( TextFromEnd == "auto const" ) {
        RangeToReplace.setEnd( RangeToReplace.getEnd().getLocWithOffset( 10 ) );
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
             "terator", "element_type", "mapped_type", "value_type", "nternal", "__"
         } ) {
        if( std::search( TypeStr.begin(), TypeStr.end(), Fragment.begin(), Fragment.end() ) !=
            TypeStr.end() ) {
            return;
        }
        if( std::search( DesugaredTypeStr.begin(), DesugaredTypeStr.end(), Fragment.begin(),
                         Fragment.end() ) != DesugaredTypeStr.end() ) {
            return;
        }
    }

    if( WasLRef ) {
        TypeStr += " &";
    } else if( WasRRef ) {
        TypeStr += " &&";
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

} // namespace clang::tidy::cata
