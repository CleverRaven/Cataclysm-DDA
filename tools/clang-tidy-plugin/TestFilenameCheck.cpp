#include "TestFilenameCheck.h"

#include <clang-tidy/ClangTidyDiagnosticConsumer.h>
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

namespace clang::tidy::cata
{

class TestFilenameCallbacks : public PPCallbacks
{
    public:
        TestFilenameCallbacks( TestFilenameCheck *Check, const SourceManager *SrcM ) :
            Check( Check ), SM( SrcM ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs * ) override {
            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();

            if( MacroName == "TEST_CASE" ) {
                StringRef Filename = SM->getBufferName( Range.getBegin() );
                bool IsTestFilename = Filename.ends_with( "_test.cpp" );

                if( !IsTestFilename ) {
                    Check->diag( Range.getBegin(),
                                 "Files containing a test definition should have a filename "
                                 "ending in '_test.cpp'." );
                }
            }
        }
    private:
        TestFilenameCheck *Check;
        const SourceManager *SM;
};

void TestFilenameCheck::registerPPCallbacks( const SourceManager &SM, Preprocessor *PP,
        Preprocessor * )
{
    PP->addPPCallbacks( std::make_unique<TestFilenameCallbacks>( this, &SM ) );
}

} // namespace clang::tidy::cata
