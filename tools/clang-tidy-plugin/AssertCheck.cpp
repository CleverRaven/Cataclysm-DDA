#include "AssertCheck.h"

#include "clang/Frontend/CompilerInstance.h"

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
        AssertMacroCallbacks( AssertCheck *Check ) :
            Check( Check ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs * ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();
            if( MacroName == "assert" ) {
                SourceRange RangeToReplace(
                    Range.getBegin(), Range.getBegin().getLocWithOffset( MacroName.size() - 1 ) );
                Check->diag( Range.getBegin(), "Prefer cata_assert to assert." ) <<
                        FixItHint::CreateReplacement( RangeToReplace, "cata_assert" );
            }
        }
    private:
        AssertCheck *Check;
};

void AssertCheck::registerPPCallbacks( CompilerInstance &Compiler )
{
    Compiler.getPreprocessor().addPPCallbacks(
        llvm::make_unique<AssertMacroCallbacks>( this ) );
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
