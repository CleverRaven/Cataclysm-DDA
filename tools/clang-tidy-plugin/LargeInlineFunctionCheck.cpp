#include "LargeInlineFunctionCheck.h"
#include "Utils.h"

#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static constexpr unsigned DefaultMaxStatements = 2;

LargeInlineFunctionCheck::LargeInlineFunctionCheck( StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context )
    , MaxStatements( Options.get( "MaxStatements", DefaultMaxStatements ) )
{}

void LargeInlineFunctionCheck::storeOptions( ClangTidyOptions::OptionMap &Opts )
{
    Options.store( Opts, "MaxStatements", MaxStatements );
}

void LargeInlineFunctionCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        functionDecl(
            anyOf( isInline(), cxxMethodDecl() ),
            hasAnyBody( compoundStmt().bind( "body" ) )
        ).bind( "decl" ),
        this
    );
}

struct CountStmtsASTVisitor final
    : public RecursiveASTVisitor<CountStmtsASTVisitor> {
    using Base = RecursiveASTVisitor<CountStmtsASTVisitor>;

    int Count = 0;

    bool TraverseStmt( Stmt *Node ) {
        if( Node && !isa<Expr>( Node ) ) {
            ++Count;
        }

        return Base::TraverseStmt( Node );
    }
};

static void CheckDecl( LargeInlineFunctionCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const FunctionDecl *ThisDecl = Result.Nodes.getNodeAs<FunctionDecl>( "decl" );
    if( !ThisDecl ) {
        return;
    }

    const SourceManager &SM = *Result.SourceManager;

    // Ignore cases that are not the first declaration
    if( ThisDecl->getPreviousDecl() ) {
        return;
    }

    // Ignore compiler-generated stuff
    if( ThisDecl->isImplicit() ) {
        return;
    }

    // Ignore things (possibly nested) inside class/struct templates
    // Also ignore lambda function call operators
    const DeclContext *Parent = ThisDecl->getParent();
    while( const CXXRecordDecl *ParentAsRecord = dyn_cast_or_null<CXXRecordDecl>( Parent ) ) {
        if( ParentAsRecord->isTemplated() || ParentAsRecord->isLambda() ) {
            return;
        }
        Parent = ParentAsRecord->getParent();
    }

    // Ignore templates
    if( ThisDecl->getTemplatedKind() != FunctionDecl::TK_NonTemplate ) {
        return;
    }

    // Ignore functions inside other functions
    if( getContainingFunction( Result, ThisDecl ) ) {
        return;
    }

    SourceLocation ExpansionBegin = SM.getFileLoc( ThisDecl->getBeginLoc() );
    if( ExpansionBegin != ThisDecl->getBeginLoc() ) {
        // This means we're inside a macro expansion
        return;
    }

    // Source file uses should be banned entirely
    if( !isInHeader( ThisDecl->getBeginLoc(), SM ) ) {
        // ... but only when they are *really* inline
        if( ThisDecl->isInlineSpecified() ) {
            Check.diag(
                ThisDecl->getBeginLoc(),
                "Function %0 declared inline in a cpp file.  'inline' should only be used in "
                "header files."
            ) << ThisDecl;
        }
        return;
    }

    const CompoundStmt *Body = Result.Nodes.getNodeAs<CompoundStmt>( "body" );
    if( !Body ) {
        return;
    }

    CountStmtsASTVisitor Visitor;
    Visitor.TraverseDecl( const_cast<FunctionDecl *>( ThisDecl ) );
    unsigned NumLines = Visitor.Count;
    if( NumLines <= Check.getMaxStatements() + 1 ) {
        return;
    }

    unsigned MaxStatements = Check.getMaxStatements();

    Check.diag(
        ThisDecl->getBeginLoc(),
        "Function %0 declared inline but contains more than %1 statements.  Consider moving the "
        "definition to a cpp file."
    ) << ThisDecl << MaxStatements;
}

void LargeInlineFunctionCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

} // namespace clang::tidy::cata
