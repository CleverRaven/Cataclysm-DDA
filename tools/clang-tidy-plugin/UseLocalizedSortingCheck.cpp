#include "UseLocalizedSortingCheck.h"

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
#include <iostream>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Casting.h>
#include <string>

#include "Utils.h"
#include "clang/Basic/OperatorKinds.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static bool IsStringish( QualType T );

static bool IsStringish( const TemplateArgument &Arg )
{
    switch( Arg.getKind() ) {
        case TemplateArgument::Type:
            if( IsStringish( Arg.getAsType() ) ) {
                return true;
            }
            break;
        case TemplateArgument::Pack:
            for( const TemplateArgument &PackArg : Arg.getPackAsArray() ) {
                if( IsStringish( PackArg ) ) {
                    return true;
                }
            }
            break;
        default:
            break;
    }
    return false;
}

static bool IsStringish( QualType T )
{
    const TagDecl *TTag = T.getTypePtr()->getAsTagDecl();
    if( !TTag ) {
        return false;
    }
    StringRef Name = TTag->getName();
    if( Name == "basic_string" ) {
        return true;
    }
    if( Name == "pair" || Name == "tuple" ) {
        const ClassTemplateSpecializationDecl *SpecDecl =
            dyn_cast<ClassTemplateSpecializationDecl>( TTag );
        if( !SpecDecl ) {
            std::cerr << "Not a spec: " << TTag->getKindName().str() << "\n";
            return false;
        }
        const TemplateArgumentList &Args = SpecDecl->getTemplateArgs();
        for( const TemplateArgument &Arg : Args.asArray() ) {
            if( IsStringish( Arg ) ) {
                return true;
            }
        }
    }
    return false;
}

void UseLocalizedSortingCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxOperatorCallExpr(
            hasArgument(
                0,
                expr( hasType( qualType().bind( "arg0Type" ) ) ).bind( "arg0Expr" )
            ),
            anyOf(
                hasOverloadedOperatorName( "<" ),
                hasOverloadedOperatorName( ">" ),
                hasOverloadedOperatorName( "<=" ),
                hasOverloadedOperatorName( ">=" )
            )
        ).bind( "opCall" ),
        this
    );

    Finder->addMatcher(
        callExpr(
            callee( namedDecl( hasAnyName( "std::sort", "std::stable_sort" ) ) ),
            argumentCountIs( 2 ),
            hasArgument(
                0,
                expr( hasType( qualType( anyOf(
                        pointerType( pointee( qualType().bind( "valueType" ) ) ),
                        hasDeclaration( decl().bind( "iteratorDecl" ) )
                                         ) ) ) )
            )
        ).bind( "sortCall" ),
        this
    );
}

static void CheckOpCall( UseLocalizedSortingCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CXXOperatorCallExpr *Call = Result.Nodes.getNodeAs<CXXOperatorCallExpr>( "opCall" );
    const QualType *Arg0Type = Result.Nodes.getNodeAs<QualType>( "arg0Type" );
    const Expr *Arg0Expr = Result.Nodes.getNodeAs<Expr>( "arg0Expr" );
    if( !Call || !Arg0Type || !Arg0Expr ) {
        return;
    }

    if( !IsStringish( *Arg0Type ) ) {
        return;
    }

    StringRef Arg0Text = getText( Result, Arg0Expr );
    if( Arg0Text.ends_with( "id" ) ) {
        return;
    }

    Check.diag( Call->getBeginLoc(),
                "Raw comparison of %0.  For UI purposes please use localized_compare from "
                "translations.h." ) << *Arg0Type;
}

static void CheckSortCall( UseLocalizedSortingCheck &Check, const MatchFinder::MatchResult &Result )
{
    const CallExpr *Call = Result.Nodes.getNodeAs<CallExpr>( "sortCall" );
    const QualType *BoundValueType = Result.Nodes.getNodeAs<QualType>( "valueType" );
    const TagDecl *IteratorDecl = Result.Nodes.getNodeAs<TagDecl>( "iteratorDecl" );
    if( !Call || ( !BoundValueType && !IteratorDecl ) ) {
        return;
    }

    QualType ValueType;

    if( IteratorDecl ) {
        //Check.diag( Call->getBeginLoc(), "Iterator type %0" ) << IteratorDecl;

        for( const Decl *D : IteratorDecl->decls() ) {
            if( const TypedefNameDecl *ND = dyn_cast<TypedefNameDecl>( D ) ) {
                if( ND->getName() == "value_type" ) {
                    ValueType = ND->getUnderlyingType();
                    break;
                }
            }
        }
    }

    if( BoundValueType ) {
        ValueType = *BoundValueType;
    }

    if( !IsStringish( ValueType ) ) {
        return;
    }

    Check.diag( Call->getBeginLoc(),
                "Raw sort of %0.  For UI purposes please use localized_compare from "
                "translations.h." ) << ValueType;
}

void UseLocalizedSortingCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckOpCall( *this, Result );
    CheckSortCall( *this, Result );
}

} // namespace clang::tidy::cata
