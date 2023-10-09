#include "StaticInitializationOrderCheck.h"
#include "Utils.h"
#include <unordered_map>

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void StaticInitializationOrderCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        declRefExpr(
            to( varDecl( unless( isConstexpr() ) ).bind( "referencedDecl" ) ),
            hasAncestor(
                varDecl(
                    hasParent( decl( anyOf( translationUnitDecl(), namespaceDecl() ) ) )
                ).bind( "decl" )
            )
        ).bind( "declref" ),
        this
    );
}

static void CheckDecl( StaticInitializationOrderCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const VarDecl *referencing_decl = Result.Nodes.getNodeAs<VarDecl>( "decl" );
    const VarDecl *referenced_decl = Result.Nodes.getNodeAs<VarDecl>( "referencedDecl" );
    const DeclRefExpr *referencing_expr = Result.Nodes.getNodeAs<DeclRefExpr>( "declref" );
    if( !referencing_decl || !referenced_decl || !referencing_expr ) {
        return;
    }

    if( getContainingFunction( Result, referencing_expr ) ) {
        // This can happen e.g. if the reference is used in a lambda function
        // in the initializer expression.
        return;
    }

    const SourceManager &SM = *Result.SourceManager;

    SourceLocation referencing_loc = referencing_expr->getBeginLoc();

    if( referenced_decl->hasDefinition() == VarDecl::DefinitionKind::Definition ) {
        const VarDecl *referenced_definition = referenced_decl->getDefinition();
        SourceLocation referenced_loc = referenced_definition->getBeginLoc();
        bool in_same_file = SM.isWrittenInSameFile( referenced_loc, referencing_loc );
        if( in_same_file &&
            SM.getFileOffset( referenced_loc ) < SM.getFileOffset( referencing_loc ) ) {
            // We can be sure they will be initialized in the same order, so
            // it's all good
            return;
        }

        if( !in_same_file && SM.isInMainFile( referencing_loc ) ) {
            // This case is similarly safe; we know header initializations
            // always happen before cpp file ones
            return;
        }

        if( in_same_file ) {
            // We know for certain that this is wrong
            Check.diag( referencing_loc, "Static initialization of %0 depends on extern %1, which "
                        "is initialized afterwards." ) << referencing_decl << referenced_decl;
            return;
        }
    }

    Check.diag( referencing_loc, "Static initialization of %0 depends on extern %1, which may be "
                "initialized afterwards." ) << referencing_decl << referenced_decl;
}

void StaticInitializationOrderCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace clang::tidy::cata
