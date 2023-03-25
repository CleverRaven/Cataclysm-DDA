#include "AssertCheck.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

class AssertMacroCallbacks : public PPCallbacks
{
    public:
        AssertMacroCallbacks( Preprocessor *PP, AssertCheck *Check ) :
            PP( PP ), Check( Check ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs *Args ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();
            SourceLocation Begin = Range.getBegin();
            SourceManager &SM = PP->getSourceManager();
            // When using assert inside cata_assert, this fetches the location
            // of the cata_assert from that of the assert.
            SourceLocation ExpansionBegin = SM.getFileLoc( Begin );
            if( MacroName == "cata_assert" ) {
                CataAssertLocations.insert( ExpansionBegin );

                if( Args->getNumMacroArguments() == 1 ) {
                    const Token *Arg = Args->getUnexpArgument( 0 );
                    if( Arg[0].is( tok::kw_false ) ) {
                        Check->diag( Begin, "Prefer cata_fatal to cata_assert( false )." );
                    } else if( Arg[0].is( tok::exclaim ) && Arg[1].is( tok::string_literal ) ) {
                        std::string Replacement = "cata_fatal( ";
                        Replacement += std::string( Arg[1].getLiteralData(), Arg[1].getLength() );
                        Replacement += " )";
                        // NOLINTNEXTLINE(cata-text-style)
                        Check->diag( Begin, "Prefer cata_fatal to cata_assert( !\"…\" )." ) <<
                                FixItHint::CreateReplacement( Range, Replacement );
                    }
                }
            } else if( MacroName == "assert" ) {
                if( !CataAssertLocations.count( ExpansionBegin ) ) {
                    SourceRange RangeToReplace(
                        Begin, Begin.getLocWithOffset( MacroName.size() - 1 ) );
                    Check->diag( Begin, "Prefer cata_assert to assert." ) <<
                            FixItHint::CreateReplacement( RangeToReplace, "cata_assert" );
                }
            }
        }
    private:
        Preprocessor *PP;
        AssertCheck *Check;
        llvm::SmallSet<SourceLocation, 10> CataAssertLocations;
};

void AssertCheck::registerPPCallbacks( const SourceManager &, Preprocessor *PP, Preprocessor * )
{
    PP->addPPCallbacks( std::make_unique<AssertMacroCallbacks>( PP, this ) );
}

void AssertCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        callExpr(
            callee( namedDecl( hasName( "abort" ) ) )
        ).bind( "call" ),
        this
    );
}

void AssertCheck::check( const MatchFinder::MatchResult &Result )
{
    const CallExpr *call = Result.Nodes.getNodeAs<CallExpr>( "call" );

    if( !call ) {
        return;
    }

    SourceLocation loc = call->getBeginLoc();

    const SourceManager *SM = Result.SourceManager;
    SourceLocation ExpansionBegin = SM->getFileLoc( loc );
    if( ExpansionBegin != loc ) {
        // This means we're inside a macro expansion
        return;
    }

    diag( loc, "Prefer cata_fatal to abort()." );
}

} // namespace clang::tidy::cata
