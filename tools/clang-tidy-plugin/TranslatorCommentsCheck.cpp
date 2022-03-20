#include "TranslatorCommentsCheck.h"

#include <ClangTidyDiagnosticConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/ASTMatchers/ASTMatchersMacros.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/Token.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Support/Regex.h>
#include <iterator>
#include <map>
#include <utility>

#include "clang/Basic/TokenKinds.h"

namespace clang
{
class CXXConstructExpr;
class MacroDefinition;
}  // namespace clang

using namespace clang::ast_matchers;

namespace clang
{
namespace ast_matchers
{
AST_POLYMORPHIC_MATCHER_P2( hasImmediateArgument,
                            AST_POLYMORPHIC_SUPPORTED_TYPES( CallExpr, CXXConstructExpr ),
                            unsigned int, N, internal::Matcher<Expr>, InnerMatcher )
{
    return N < Node.getNumArgs() &&
           InnerMatcher.matches( *Node.getArg( N )->IgnoreImplicit(), Finder, Builder );
}

AST_MATCHER_P( StringLiteral, isMarkedString, tidy::cata::TranslatorCommentsCheck *, Check )
{
    Check->MatchingStarted = true;
    SourceManager &SM = Finder->getASTContext().getSourceManager();
    SourceLocation Loc = SM.getFileLoc( Node.getBeginLoc() );
    return Check->MarkedStrings.find( Loc ) != Check->MarkedStrings.end();
    static_cast<void>( Builder );
}
} // namespace ast_matchers
namespace tidy
{
namespace cata
{

class TranslatorCommentsCheck::TranslatorCommentsHandler : public CommentHandler
{
    public:
        explicit TranslatorCommentsHandler( TranslatorCommentsCheck &Check ) : Check( Check ),
            // xgettext will treat all comments containing the marker as
            // translator comments, but we only match those starting with
            // the marker to allow using the marker inside normal comments
            Match( "^/[/*]~.*$" ) {}

        bool HandleComment( Preprocessor &PP, SourceRange Range ) override {
            if( Check.MatchingStarted ) {
                // according to the standard, all comments are processed before analyzing the syntax
                Check.diag( Range.getBegin(), "AST Matching started before the end of comment preprocessing",
                            DiagnosticIDs::Error );
            }

            const SourceManager &SM = PP.getSourceManager();
            StringRef Text = Lexer::getSourceText( CharSourceRange::getCharRange( Range ),
                                                   SM, PP.getLangOpts() );

            if( !Match.match( Text ) ) {
                return false;
            }

            SourceLocation BegLoc = SM.getFileLoc( Range.getBegin() );
            SourceLocation EndLoc = SM.getFileLoc( Range.getEnd() );
            FileID File = SM.getFileID( EndLoc );
            unsigned int EndLine = SM.getSpellingLineNumber( EndLoc );
            unsigned int EndCol = SM.getSpellingColumnNumber( EndLoc );

            if( File != SM.getFileID( BegLoc ) ) {
                Check.diag( BegLoc, "Mysterious multi-file comment", DiagnosticIDs::Error );
                return false;
            }

            unsigned int BegLine = SM.getSpellingLineNumber( BegLoc );

            TranslatorComments.emplace( TranslatorCommentLocation { File, EndLine, EndCol },
                                        TranslatorComment { BegLoc, BegLine, false } );
            return false;
        }

        struct TranslatorCommentLocation {
            FileID File;
            unsigned int EndLine;
            unsigned int EndCol;

            bool operator==( const TranslatorCommentLocation &Other ) const {
                return File == Other.File && EndLine == Other.EndLine && EndCol == Other.EndCol;
            }

            bool operator<( const TranslatorCommentLocation &Other ) const {
                if( File != Other.File ) {
                    return File < Other.File;
                }
                if( EndLine != Other.EndLine ) {
                    return EndLine < Other.EndLine;
                }
                return EndCol < Other.EndCol;
            }
        };

        struct TranslatorComment {
            SourceLocation Beg;
            unsigned int BegLine;
            bool Checked;
        };

        std::map<TranslatorCommentLocation, TranslatorComment> TranslatorComments;

    private:
        TranslatorCommentsCheck &Check;
        llvm::Regex Match;
};

class TranslatorCommentsCheck::TranslationMacroCallback : public PPCallbacks
{
    public:
        TranslationMacroCallback( TranslatorCommentsCheck &Check, const SourceManager &SM )
            : Check( Check ), SM( SM ) {}

        void MacroExpands( const Token &MacroNameTok,
                           const MacroDefinition &,
                           SourceRange Range,
                           const MacroArgs *Args ) override {
            if( Check.MatchingStarted ) {
                // according to the standard, all macros are expanded before analyzing the syntax
                Check.diag( Range.getBegin(), "AST Matching started before the end of macro expansion",
                            DiagnosticIDs::Error );
            }

            StringRef MacroName = MacroNameTok.getIdentifierInfo()->getName();

            bool is_marker;
            unsigned int RawStringInd;
            if( MacroName == "_" ) {
                is_marker = false;
                RawStringInd = 0;
            } else if( MacroName == "translate_marker" ) {
                is_marker = true;
                RawStringInd = 0;
            } else if( MacroName == "translate_marker_context" ) {
                is_marker = true;
                RawStringInd = 1;
            } else {
                return;
            }

            if( RawStringInd >= Args->getNumMacroArguments() ) {
                Check.diag( Range.getBegin(), "Translation marker doesn't have expected number of arguments",
                            DiagnosticIDs::Error );
            }

            // First ensure that translation markers have only string literal arguments
            for( unsigned int i = 0; i < Args->getNumMacroArguments(); i++ ) {
                const Token *Tok = Args->getUnexpArgument( i );
                if( Tok->is( tok::eof ) ) {
                    Check.diag( Tok->getLocation(), "Empty argument to a translation marker macro" );
                    return;
                }
                for( ; Tok->isNot( tok::eof ); ++Tok ) {
                    if( !tok::isStringLiteral( Tok->getKind() ) ) {
                        if( is_marker ) {
                            Check.diag( Tok->getLocation(), "Translation marker macros only accepts string literal arguments" );
                        }
                        return;
                    }
                }
            }

            const Token *Tok = Args->getUnexpArgument( RawStringInd );
            Check.MarkedStrings.emplace( SM.getFileLoc( Tok->getLocation() ) );
        }

    private:
        TranslatorCommentsCheck &Check;
        const SourceManager &SM;
};

TranslatorCommentsCheck::TranslatorCommentsCheck( StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context ),
      MatchingStarted( false ),
      Handler( std::make_unique<TranslatorCommentsHandler>( *this ) ) {}

TranslatorCommentsCheck::~TranslatorCommentsCheck() = default;

void TranslatorCommentsCheck::registerPPCallbacks(
    const SourceManager &SM, Preprocessor *PP, Preprocessor * )
{
    PP->addCommentHandler( Handler.get() );
    PP->addPPCallbacks( std::make_unique<TranslationMacroCallback>( *this, SM ) );
}

void TranslatorCommentsCheck::registerMatchers( MatchFinder *Finder )
{
    const auto stringLiteralArgumentBound =
        anyOf(
            stringLiteral().bind( "RawText" ),
            cxxConstructExpr(
                unless( isListInitialization() ),
                hasImmediateArgument( 0, stringLiteral().bind( "RawText" ) )
            )
        );
    const auto stringLiteralArgumentUnbound =
        anyOf(
            stringLiteral(),
            cxxConstructExpr(
                unless( isListInitialization() ),
                hasImmediateArgument( 0, stringLiteral() )
            )
        );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasAnyName( "_", "translation_argument_identity", "gettext" ) ) ),
            hasImmediateArgument( 0, stringLiteralArgumentBound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasName( "n_gettext" ) ) ),
            hasImmediateArgument( 0, stringLiteralArgumentBound ),
            hasImmediateArgument( 1, stringLiteralArgumentUnbound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasName( "to_translation" ) ) ),
            argumentCountIs( 1 ),
            hasImmediateArgument( 0, stringLiteralArgumentBound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasName( "pl_translation" ) ) ),
            argumentCountIs( 2 ),
            hasImmediateArgument( 0, stringLiteralArgumentBound ),
            hasImmediateArgument( 1, stringLiteralArgumentUnbound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasAnyName( "pgettext" ) ) ),
            hasImmediateArgument( 0, stringLiteralArgumentUnbound ),
            hasImmediateArgument( 1, stringLiteralArgumentBound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasAnyName( "npgettext" ) ) ),
            hasImmediateArgument( 0, stringLiteralArgumentUnbound ),
            hasImmediateArgument( 1, stringLiteralArgumentBound ),
            hasImmediateArgument( 2, stringLiteralArgumentUnbound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasName( "to_translation" ) ) ),
            argumentCountIs( 2 ),
            hasImmediateArgument( 0, stringLiteralArgumentUnbound ),
            hasImmediateArgument( 1, stringLiteralArgumentBound )
        ),
        this
    );
    Finder->addMatcher(
        callExpr(
            callee( functionDecl( hasName( "pl_translation" ) ) ),
            argumentCountIs( 3 ),
            hasImmediateArgument( 0, stringLiteralArgumentUnbound ),
            hasImmediateArgument( 1, stringLiteralArgumentBound ),
            hasImmediateArgument( 2, stringLiteralArgumentUnbound )
        ),
        this
    );
    Finder->addMatcher(
        stringLiteral( isMarkedString( this ) ).bind( "RawText" ),
        this
    );
}

void TranslatorCommentsCheck::check( const MatchFinder::MatchResult &Result )
{
    MatchingStarted = true;

    const StringLiteral *RawText = Result.Nodes.getNodeAs<StringLiteral>( "RawText" );
    if( !RawText ) {
        return;
    }

    const SourceManager &SM = *Result.SourceManager;
    SourceLocation BegLoc = SM.getFileLoc( RawText->getBeginLoc() );
    FileID File = SM.getFileID( BegLoc );
    unsigned int BegLine = SM.getSpellingLineNumber( BegLoc );
    unsigned int BegCol = SM.getSpellingColumnNumber( BegLoc );

    auto it = Handler->TranslatorComments.lower_bound( { File, BegLine, BegCol } );
    // Strictly speaking, a translator comment preceding a raw string with only
    // blank lines in between will also be extracted, but we report it as an
    // error here for simplicity.
    while( it != Handler->TranslatorComments.begin() && std::prev( it )->first.File == File &&
           std::prev( it )->first.EndLine + 1 >= BegLine ) {
        it = std::prev( it );
        // TODO: for the following code,
        //
        // /*<marker> foo*/ to_translation( "bar" );
        // _( "baz" );
        //
        // The current logic will mark the comment when matching _() in addition
        // to to_translation(), while xgettext will only match the comment with
        // to_translation(). However the logic currently does not concern the
        // content of the extracted string, so this doens't affect the results
        // for now.
        it->second.Checked = true;
        BegLine = it->second.BegLine;
    }
}

void TranslatorCommentsCheck::onEndOfTranslationUnit()
{
    // Report all translator comments without a matching string, after the end of AST iteration
    for( const auto &elem : Handler->TranslatorComments ) {
        if( !elem.second.Checked ) {
            diag( elem.second.Beg, "Translator comment without a matching raw string" );
        }
    }
    ClangTidyCheck::onEndOfTranslationUnit();
}

} // namespace cata
} // namespace tidy
} // namespace clang
