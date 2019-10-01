#include "XYCheck.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"

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
            matchesName( "x$" ),
            hasParent(
                cxxRecordDecl(
                    forEachDescendant( fieldDecl( matchesName( "y$" ) ).bind( "yfield" ) )
                ).bind( "record" )
            )
        ).bind( "xfield" ),
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
    llvm::StringRef XPrefix = XVar->getName().drop_back();
    llvm::StringRef YPrefix = YVar->getName().drop_back();
    if( XPrefix != YPrefix ) {
        return;
    }

    const FieldDecl *ZVar = nullptr;
    for( FieldDecl *Field : Record->fields() ) {
        StringRef Name = Field->getName();
        if( Name.endswith( "z" ) && Name.drop_back() == XPrefix ) {
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

void XYCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckField( *this, Result );
}

} // namespace cata
} // namespace tidy
} // namespace clang
