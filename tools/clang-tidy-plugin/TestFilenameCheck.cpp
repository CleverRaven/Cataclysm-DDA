#include "TestFilenameCheck.h"

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

class TestFilenameCallbacks : public PPCallbacks
{
    public:
        TestFilenameCallbacks( TestFilenameCheck *Check, CompilerInstance *Compiler ) :
            Check( Check ), Compiler( Compiler ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs * ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();

            if( MacroName == "TEST_CASE" ) {
                SourceManager &SM = Compiler->getSourceManager();
                StringRef Filename = SM.getBufferName( Range.getBegin() );
                bool IsTestFilename = Filename.endswith( "_test.cpp" );

                if( !IsTestFilename ) {
                    Check->diag( Range.getBegin(),
                                 "Files containing a test definition should have a filename "
                                 "ending in '_test.cpp'." );
                }
            }
        }
    private:
        TestFilenameCheck *Check;
        CompilerInstance *Compiler;
};

void TestFilenameCheck::registerPPCallbacks( CompilerInstance &Compiler )
{
    Compiler.getPreprocessor().addPPCallbacks(
        llvm::make_unique<TestFilenameCallbacks>( this, &Compiler ) );
}

} // namespace cata
} // namespace tidy
} // namespace clang
