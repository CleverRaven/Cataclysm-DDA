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
#include "Utils.h"

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

TestsMustRestoreGlobalStateCheck::TestsMustRestoreGlobalStateCheck(
    StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context ) {}

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

void TestsMustRestoreGlobalStateCheck::registerPPCallbacks(
    const SourceManager &, Preprocessor *PP, Preprocessor * )
{
    PP->addPPCallbacks( std::make_unique<TestsMustRestoreGlobalStateCallbacks>( this ) );
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

    Finder->addMatcher(
        cxxOperatorCallExpr(
            hasArgument(
                0,
                memberExpr( member( namedDecl( hasName( "weather_override" ) ) ) )
            ),
            isAssignmentOperator()
        ).bind( "assignToWeatherOverride" ),
        this
    );
}

void TestsMustRestoreGlobalStateCheck::check( const MatchFinder::MatchResult &Result )
{
    if( !is_test_file_ ) {
        return;
    }

    const BinaryOperator *Assignment = Result.Nodes.getNodeAs<BinaryOperator>( "assign" );
    const NamedDecl *LHSDecl = Result.Nodes.getNodeAs<NamedDecl>( "lhsDecl" );

    if( Assignment && LHSDecl ) {
        const FunctionDecl *FuncDecl = getContainingFunction( Result, Assignment );
        suspicious_assignments_.push_back( {Assignment, {FuncDecl, LHSDecl}} );
        return;
    }

    const CXXConstructExpr *ConstructExpr =
        Result.Nodes.getNodeAs<CXXConstructExpr>( "construction" );
    const NamedDecl *VarDecl = Result.Nodes.getNodeAs<NamedDecl>( "restoredDecl" );

    if( ConstructExpr && VarDecl ) {
        const FunctionDecl *FuncDecl = getContainingFunction( Result, ConstructExpr );
        restored_decls_.insert( {FuncDecl, VarDecl} );
        return;
    }

    const CXXOperatorCallExpr *AssignmentToWeather =
        Result.Nodes.getNodeAs<CXXOperatorCallExpr>( "assignToWeatherOverride" );
    if( AssignmentToWeather ) {
        diag( AssignmentToWeather->getBeginLoc(),
              "Test assigns to weather_override.  It should instead use scoped_weather_override." );
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
              "'restore_on_out_of_scope'." ) << a.lhsDecl.variable;
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
