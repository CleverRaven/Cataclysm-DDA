#include "SerializeCheck.h"

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

SerializeCheck::SerializeCheck(
    StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context ) {}

void SerializeCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        memberExpr(
            member( fieldDecl().bind( "member" ) ),
            hasAncestor(
                cxxMethodDecl( hasAnyName( "serialize", "deserialize" ) ).bind( "function" )
            )
        ),
        this
    );
}

void SerializeCheck::check( const MatchFinder::MatchResult &Result )
{
    sm_ = Result.SourceManager;
    const FieldDecl *ReferencedMember = Result.Nodes.getNodeAs<FieldDecl>( "member" );
    const CXXMethodDecl *ContainingFunction = Result.Nodes.getNodeAs<CXXMethodDecl>( "function" );

    if( !ReferencedMember || !ContainingFunction ) {
        return;
    }

    mentioned_decls_[ContainingFunction].push_back( ReferencedMember );
}

void SerializeCheck::onEndOfTranslationUnit()
{
    for( std::pair<const CXXMethodDecl *const, std::vector<const FieldDecl *>> &p :
         mentioned_decls_ ) {
        const CXXMethodDecl *method = p.first;
        CharSourceRange function_range = CharSourceRange::getTokenRange( method->getSourceRange() );
        StringRef function_text = Lexer::getSourceText( function_range, *sm_, LangOptions() );
        if( function_text.contains( "CATA_DO_NOT_CHECK_SERIALIZE" ) ) {
            continue;
        }
        const CXXRecordDecl *record = method->getParent();
        std::vector<const FieldDecl *> &mentioned_fields = p.second;
        std::sort( mentioned_fields.begin(), mentioned_fields.end() );
        mentioned_fields.erase( std::unique( mentioned_fields.begin(), mentioned_fields.end() ),
                                mentioned_fields.end() );
        for( const FieldDecl *field : record->fields() ) {
            if( field->getName() == "was_loaded" ) {
                continue;
            }
            auto it = std::lower_bound( mentioned_fields.begin(), mentioned_fields.end(), field );
            if( it == mentioned_fields.end() || *it != field ) {
                diag( field->getEndLoc(),
                      "Function %0 appears to be a serialization function for class %1 but does "
                      "not mention field %2." ) << method << record << field;
                diag( method->getBeginLoc(), "see function %0", DiagnosticIDs::Note ) << method;
            }
        }
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
