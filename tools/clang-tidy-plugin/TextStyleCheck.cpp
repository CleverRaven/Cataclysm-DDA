#include "TextStyleCheck.h"

#include "StringLiteralIterator.h"
#include "../../src/text_style_check.h"

using namespace clang::ast_matchers;

namespace clang
{
namespace tidy
{
namespace cata
{

TextStyleCheck::TextStyleCheck( StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context ),
      EscapeUnicode( Options.get( "EscapeUnicode", 0 ) != 0 ) {}

void TextStyleCheck::storeOptions( ClangTidyOptions::OptionMap &Opts )
{
    Options.store( Opts, "EscapeUnicode", EscapeUnicode );
}

void TextStyleCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        stringLiteral(
            anyOf(
                // check if the literal is used in string concat.
                // todo: check the side of the literal
                hasAncestor(
                    cxxOperatorCallExpr(
                        anyOf(
                            hasOverloadedOperatorName( "+" ),
                            hasOverloadedOperatorName( "<<" ),
                            hasOverloadedOperatorName( "+=" )
                        )
                    ).bind( "concat" )
                ),
                anything()
            ),
            // specifically ignore mapgen strings
            unless(
                hasAncestor(
                    callExpr(
                        callee(
                            functionDecl(
                                hasAnyName( "formatted_set_simple", "ter_bind", "furn_bind" )
                            )
                        )
                    )
                )
            ),
            // __func__, etc
            unless( hasAncestor( predefinedExpr() ) )
        ).bind( "str" ),
        this
    );
}

void TextStyleCheck::check( const MatchFinder::MatchResult &Result )
{
    const StringLiteral *str = Result.Nodes.getNodeAs<StringLiteral>( "str" );
    if( !str ) {
        return;
    }
    const StringLiteral &text = *str;

    const SourceManager &SrcMgr = *Result.SourceManager;
    for( size_t i = 0; i < text.getNumConcatenated(); ++i ) {
        const SourceLocation &loc = text.getStrTokenLoc( i );
        if( loc.isInvalid() ) {
            return;
        } else if( StringRef( SrcMgr.getPresumedLoc( SrcMgr.getSpellingLoc(
                                  loc ) ).getFilename() ).equals( "<scratch space>" ) ) {
            return;
        }
    }

    // ignore wide/u16/u32 strings
    if( ( !text.isAscii() && !text.isUTF8() ) || text.getCharByteWidth() != 1 ) {
        return;
    }

    // disable fix-its for utf8 strings to avoid removing the u8 prefix
    bool fixit = text.isAscii();
    for( size_t i = 0; fixit && i < text.getNumConcatenated(); ++i ) {
        const SourceLocation &loc = text.getStrTokenLoc( i );
        if( !loc.isMacroID() && SrcMgr.getCharacterData( loc )[0] == 'R' ) {
            // disable fix-its for raw strings to avoid removing the R prefix
            fixit = false;
        }
    }

    const LangOptions &LangOpts = Result.Context->getLangOpts();
    const TargetInfo &Info = Result.Context->getTargetInfo();
    const auto location = [&SrcMgr, &LangOpts, &Info]
    ( const StringLiteralIterator & it ) -> SourceLocation {
        return SrcMgr.getSpellingLoc( it.toSourceLocation( SrcMgr, LangOpts, Info ) );
    };

    const auto text_style_check_callback =
        [this, fixit, location]
        ( const text_style_fix type, const std::string & msg,
          const StringLiteralIterator & /*beg*/, const StringLiteralIterator & /*end*/,
          const StringLiteralIterator & at,
          const StringLiteralIterator & from, const StringLiteralIterator & to,
          const std::string & fix
    ) {
        auto diags = diag( location( at ), msg );
        if( fixit ) {
            switch( type ) {
                case text_style_fix::removal: {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( from ), location( to ) );
                    diags << FixItHint::CreateRemoval( range );
                }
                break;
                case text_style_fix::insertion:
                    diags << FixItHint::CreateInsertion( location( from ), fix );
                    break;
                case text_style_fix::replacement: {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( from ), location( to ) );
                    diags << FixItHint::CreateReplacement( range, fix );
                }
                break;
            }
        }
    };

    const auto beg = StringLiteralIterator::begin( text );
    const auto end = StringLiteralIterator::end( text );
    const bool in_concat_expr = Result.Nodes.getNodeAs<CXXOperatorCallExpr>( "concat" ) != nullptr;
    text_style_check<StringLiteralIterator>( beg, end,
            // treat spaces at the end of strings in concat expressions (+, += or <<) as deliberate
            in_concat_expr ? fix_end_of_string_spaces::no : fix_end_of_string_spaces::yes,
            EscapeUnicode ? escape_unicode::yes : escape_unicode::no,
            text_style_check_callback );
}

} // namespace cata
} // namespace tidy
} // namespace clang
