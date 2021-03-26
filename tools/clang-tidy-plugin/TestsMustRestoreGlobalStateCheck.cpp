#include "TestsMustRestoreGlobalStateCheck.h"

#include <ClangTidyDiagnosticConsumer.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/STLExtras.h>

#include "clang/Frontend/CompilerInstance.h"

namespace clang
{
class MacroArgs;
class MacroDefinition;
}  // namespace clang

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

class TestsMustRestoreGlobalStateCallbacks : public PPCallbacks
{
    public:
        explicit TestsMustRestoreGlobalStateCallbacks( TestsMustRestoreGlobalStateCheck *Check ) :
            Check( Check ) {}

        void MacroDefined( const Token &MacroNameTok,
                           const MacroDirective * ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();

            if( MacroName == "TEST_CASE" ) {
                Check->setIsTestFile();
            }
        }
    private:
        TestsMustRestoreGlobalStateCheck *Check;
};

void TestsMustRestoreGlobalStateCheck::registerPPCallbacks( CompilerInstance &Compiler )
{
    Compiler.getPreprocessor().addPPCallbacks(
        llvm::make_unique<TestsMustRestoreGlobalStateCallbacks>( this ) );
}

void TestsMustRestoreGlobalStateCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        cxxConstructExpr(
            hasArgument( 0, declRefExpr( hasDeclaration( namedDecl().bind( "restoredDecl" ) ) ) ),
            hasDeclaration( namedDecl( hasName( "restore_on_out_of_scope" ) ) )
        ).bind( "construction" ),
        this
    );

    Finder->addMatcher(
        binaryOperator(
            hasLHS(
                declRefExpr(
                    hasDeclaration(
                        namedDecl(
                            hasDeclContext( anyOf( namespaceDecl(), translationUnitDecl() ) )
                        ).bind( "lhsDecl" )
                    )
                )
            ),
            isAssignmentOperator()
        ).bind( "assign" ),
        this
    );
}

void TestsMustRestoreGlobalStateCheck::check( const MatchFinder::MatchResult &Result )
{
    const BinaryOperator *Assignment = Result.Nodes.getNodeAs<BinaryOperator>( "assign" );
    const NamedDecl *LHSDecl = Result.Nodes.getNodeAs<NamedDecl>( "lhsDecl" );

    if( Assignment && LHSDecl ) {
        suspicious_assignments_.push_back( {Assignment, LHSDecl} );
        return;
    }

    const CXXConstructExpr *ConstructExpr =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "construction" );
    const NamedDecl *RestoredDecl = Result.Nodes.getNodeAs<NamedDecl>( "restoredDecl" );

    if( ConstructExpr && RestoredDecl ) {
        restored_decls_.insert( RestoredDecl );
    }
}

void TestsMustRestoreGlobalStateCheck::onEndOfTranslationUnit()
{
    if( !is_test_file_ ) {
        return;
    }

    for( const AssignmentToGlobal &a : suspicious_assignments_ ) {
        if( restored_decls_.count( a.lhsDecl ) ) {
            continue;
        }
        diag( a.assignment->getBeginLoc(),
              "Test alters global variable %0. You must ensure it is restored using "
              "'restore_on_out_of_scope'." ) << a.lhsDecl;
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
