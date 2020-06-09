#include "XYCheck.h"

#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/Specifiers.h>

#include "Utils.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

void XYCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        fieldDecl(
            hasType( asString( "int" ) ),
            isXParam(),
            hasParent(
                cxxRecordDecl(
                    forEachDescendant( fieldDecl( isYParam() ).bind( "yfield" ) )
                ).bind( "record" )
            )
        ).bind( "xfield" ),
        this
    );
    Finder->addMatcher(
        parmVarDecl(
            hasType( asString( "int" ) ),
            isXParam(),
            hasAncestor( functionDecl().bind( "function" ) )
        ).bind( "xparam" ),
        this
    );
}

static void CheckField( XYCheck &Check, const MatchFinder::MatchResult &Result )
{
    const FieldDecl *XVar = Result.Nodes.getNodeAs<FieldDecl>( "xfield" );
    const FieldDecl *YVar = Result.Nodes.getNodeAs<FieldDecl>( "yfield" );
    const CXXRecordDecl *Record = Result.Nodes.getNodeAs<CXXRecordDecl>( "record" );
    if( !XVar || !YVar || !Record ) {
        return;
    }
    const NameConvention NameMatcher( XVar->getName() );
    if( !NameMatcher ) {
        return;
    }
    if( NameMatcher.Match( YVar->getName() ) != NameConvention::YName ) {
        return;
    }

    const FieldDecl *ZVar = nullptr;
    for( FieldDecl *Field : Record->fields() ) {
        if( NameMatcher.Match( Field->getName() ) == NameConvention::ZName ) {
            ZVar = Field;
            break;
        }
    }
    TemplateSpecializationKind tsk = Record->getTemplateSpecializationKind();
    if( tsk != TSK_Undeclared ) {
        // Avoid duplicate warnings for specializations
        return;
    }
    if( ZVar ) {
        Check.diag(
            Record->getLocation(),
            "%0 defines fields %1, %2, and %3.  Consider combining into a single tripoint "
            "field." ) << Record << XVar << YVar << ZVar;
    } else {
        Check.diag(
            Record->getLocation(),
            "%0 defines fields %1 and %2.  Consider combining into a single point "
            "field." ) << Record << XVar << YVar;
    }
    Check.diag( XVar->getLocation(), "declaration of %0", DiagnosticIDs::Note ) << XVar;
    Check.diag( YVar->getLocation(), "declaration of %0", DiagnosticIDs::Note ) << YVar;
    if( ZVar ) {
        Check.diag( ZVar->getLocation(), "declaration of %0", DiagnosticIDs::Note ) << ZVar;
    }
}

static void CheckParam( XYCheck &Check, const MatchFinder::MatchResult &Result )
{
    const ParmVarDecl *XParam = Result.Nodes.getNodeAs<ParmVarDecl>( "xparam" );
    const FunctionDecl *Function = Result.Nodes.getNodeAs<FunctionDecl>( "function" );
    if( !XParam || !Function ) {
        return;
    }
    // Don't mess with the methods of point and tripoint themselves
    if( isPointMethod( Function ) ) {
        return;
    }
    const NameConvention NameMatcher( XParam->getName() );

    const ParmVarDecl *YParam = nullptr;
    const ParmVarDecl *ZParam = nullptr;
    for( ParmVarDecl *Parameter : Function->parameters() ) {
        switch( NameMatcher.Match( Parameter->getName() ) ) {
            case NameConvention::ZName:
                ZParam = Parameter;
                break;
            case NameConvention::YName:
                YParam = Parameter;
                break;
            default:
                break;
        }
    }

    if( !YParam ) {
        return;
    }

    TemplateSpecializationKind tsk = Function->getTemplateSpecializationKind();
    if( tsk != TSK_Undeclared ) {
        // Avoid duplicate warnings for specializations
        return;
    }

    if( ZParam ) {
        Check.diag(
            Function->getLocation(),
            "%0 has parameters %1, %2, and %3.  Consider combining into a single tripoint "
            "parameter." ) << Function << XParam << YParam << ZParam;
    } else {
        Check.diag(
            Function->getLocation(),
            "%0 has parameters %1 and %2.  Consider combining into a single point "
            "parameter." ) << Function << XParam << YParam;
    }
    Check.diag( XParam->getLocation(), "declaration of %0", DiagnosticIDs::Note ) << XParam;
    Check.diag( YParam->getLocation(), "declaration of %0", DiagnosticIDs::Note ) << YParam;
    if( ZParam ) {
        Check.diag( ZParam->getLocation(), "declaration of %0", DiagnosticIDs::Note ) << ZParam;
    }
}

void XYCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckField( *this, Result );
    CheckParam( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
