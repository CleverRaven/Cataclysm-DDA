#include "AssertCheck.h"

#include <unordered_set>

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Token.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

class AssertMacroCallbacks : public PPCallbacks
{
    public:
        AssertMacroCallbacks( Preprocessor *PP, AssertCheck *Check ) :
            PP( PP ), Check( Check ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs * ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();
            SourceLocation Begin = Range.getBegin();
            SourceManager &SM = PP->getSourceManager();
            // When using assert inside cata_assert, this fetches the location
            // of the cata_assert from that of the assert.
            SourceLocation ExpansionBegin = SM.getFileLoc( Begin );
            if( MacroName == "cata_assert" ) {
                CataAssertLocations.insert( ExpansionBegin );
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
        llvm::SmallPtrSet<SourceLocation, 10> CataAssertLocations;
};

void AssertCheck::registerPPCallbacks( const SourceManager &, Preprocessor *PP, Preprocessor * )
{
    PP->addPPCallbacks( std::make_unique<AssertMacroCallbacks>( PP, this ) );
}

void AssertCheck::registerMatchers( MatchFinder * /*Finder*/ )
{
}

void AssertCheck::check( const MatchFinder::MatchResult &/*Result*/ )
{
}

} // namespace cata
} // namespace tidy
} // namespace clang
