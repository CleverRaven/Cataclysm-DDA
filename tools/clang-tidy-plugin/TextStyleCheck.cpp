#include "TextStyleCheck.h"

#include "StringLiteralIterator.h"

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
            )
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

    // ignore macro expansions
    for( size_t i = 0; i < text.getNumConcatenated(); ++i ) {
        const SourceLocation &loc = text.getStrTokenLoc( i );
        if( loc.isInvalid() || loc.isMacroID() ) {
            return;
        }
    }

    // ignore wide/u16/u32 strings
    if( ( !text.isAscii() && !text.isUTF8() ) || text.getCharByteWidth() != 1 ) {
        return;
    }

    const SourceManager &SrcMgr = *Result.SourceManager;
    // disable fix-its for utf8 strings to avoid removing the u8 prefix
    bool fixit = text.isAscii();
    for( size_t i = 0; fixit && i < text.getNumConcatenated(); ++i ) {
        const SourceLocation &loc = text.getStrTokenLoc( i );
        if( SrcMgr.getCharacterData( loc )[0] == 'R' ) {
            // disable fix-its for raw strings to avoid removing the R prefix
            fixit = false;
        }
    }

    const LangOptions &LangOpts = Result.Context->getLangOpts();
    const TargetInfo &Info = Result.Context->getTargetInfo();
    const auto location = [&SrcMgr, &LangOpts, &Info]
    ( const StringLiteralIterator & it ) -> SourceLocation {
        return it.toSourceLocation( SrcMgr, LangOpts, Info );
    };

    struct punctuation {
        std::u32string symbol;
        std::u32string follow;
        struct {
            bool check;
            size_t min_string_length = 0;
            size_t min_word_length = 0;
            size_t fix_spaces_min = 0;
            size_t fix_spaces_max = 0;
            size_t fix_spaces_to = 0;
            // remove unnecessary spaces at string end
            size_t fix_end_max = 0;
            // remove unnecessary spaces at line end (before '\n')
            size_t fix_line_end_max = 0;
            // remove unnecessary spaces after a symbol that starts a string
            size_t fix_start_max = 0;
            // remove unnecessary spaces after a symbol that starts a line (after '\n')
            size_t fix_line_start_max = 0;
            // remove unnecessary spaces before the symbol
            size_t fix_before_max = 0;
        } spaces;
        struct {
            bool yes;
            std::string str {};
            std::string escaped {};
            std::string sym_desc {};
            std::string replace_desc {};
        } replace;
    };
    // always put the longest (in u32) symbols at the front, since we'll iterate
    // and search for them in this order.
    // *INDENT-OFF*
    static const std::array<punctuation, 13> punctuations = {{
        // symbol,follow,    spaces,                                 replace,
        //                    check,  len, num spc,  end,start,before    yes,   string,     escaped,  symbol desc,      replc desc
        { U"...",    U"",   {  true, 0, 1, 0, 0, 0, 2, 2, 2, 2, 1 }, {  true, "\u2026", R"(\u2026)", "three dots",      "ellipsis" } },
        { U"::",     U"",   { false,                              }, { false,                                                      } },
        { U"\r\n",   U"",   { false,                              }, {  true, R"(\n)",  R"(\n)",     "carriage return", "new line" } },
        { U"\u2026", U"",   {  true, 0, 1, 1, 3, 2, 2, 2, 2, 2, 1 }, { false,                                                      } },
        { U".",      U"",   {  true, 0, 3, 1, 3, 2, 0, 2, 0, 0, 1 }, { false,                                                      } },
        { U";",      U"",   {  true, 0, 1, 1, 2, 1, 1, 1, 0, 0, 1 }, { false,                                                      } },
        { U"!",      U"!?", {  true, 0, 1, 1, 3, 2, 2, 2, 0, 0, 1 }, { false,                                                      } },
        { U"?",      U"!?", {  true, 0, 1, 1, 3, 2, 2, 2, 0, 0, 1 }, { false,                                                      } },
        { U":",      U"",   {  true, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1 }, { false,                                                      } },
        { U",",      U"",   {  true, 0, 1, 1, 2, 1, 0, 1, 0, 0, 1 }, { false,                                                      } },
        { U"\r",     U"",   { false,                              }, {  true, R"(\n)",  R"(\n)",     "carriage return", "new line" } },
        { U"\n",     U"",   {  true, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1 }, { false,                                                      } },
        { U"\t",     U"",   { false,                              }, {  true, "    ",   "    ",      "tab",             "spaces"   } },
    }};
    // *INDENT-ON*

    const auto beg = StringLiteralIterator::begin( text );
    const auto end = StringLiteralIterator::end( text );
    const size_t text_length = StringLiteralIterator::distance( beg, end );
    const bool in_concat_expr = Result.Nodes.getNodeAs<CXXOperatorCallExpr>( "concat" ) != nullptr;
    for( auto it = beg; it < end; ) {
        auto itpunc = it;
        auto punc = punctuations.begin();
        for( ; punc < punctuations.end(); ++punc ) {
            auto itpuncend = it;
            bool matches = true;
            for( auto isym = punc->symbol.begin(); isym < punc->symbol.end(); ++isym, ++itpuncend ) {
                if( itpuncend >= end || *isym != *itpuncend ) {
                    matches = false;
                    break;
                }
            }
            if( !matches ) {
                continue;
            }
            for( ; itpuncend < end; ++itpuncend ) {
                if( punc->follow.find( *itpuncend ) == std::u32string::npos ) {
                    break;
                }
            }
            it = itpuncend;
            break;
        }
        if( punc >= punctuations.end() ) {
            ++it;
            continue;
        }
        if( punc->replace.yes ) {
            const CharSourceRange range = CharSourceRange::getCharRange(
                                              location( itpunc ), location( it ) );
            const std::string &replace = EscapeUnicode ? punc->replace.escaped : punc->replace.str;
            auto diags = diag( location( itpunc ), "%0 preferred over %1." )
                         << punc->replace.replace_desc << punc->replace.sym_desc;
            if( fixit ) {
                diags << FixItHint::CreateReplacement( range, replace );
            }
        }
        if( punc->spaces.check && text_length >= punc->spaces.min_string_length ) {
            size_t spacesbefore = 0;
            auto itspacebefore = itpunc;
            for( ; itspacebefore > beg; --itspacebefore, ++spacesbefore ) {
                const uint32_t ch = *( itspacebefore - 1 );
                if( ch != U' ' ) {
                    break;
                }
            }
            if( spacesbefore > 0 && spacesbefore <= punc->spaces.fix_before_max ) {
                const CharSourceRange range = CharSourceRange::getCharRange(
                                                  location( itspacebefore ), location( itpunc ) );
                auto diags = diag( location( itpunc ), "unnecessary spaces before this location." );
                if( fixit ) {
                    diags << FixItHint::CreateRemoval( range );
                }
            }
            size_t wordlen = 0;
            for( auto itword = itpunc; itword > beg; --itword, ++wordlen ) {
                const uint32_t ch = *( itword - 1 );
                if( ( ch < U'a' || ch > U'z' ) && ( ch < U'A' || ch > U'Z' ) &&
                    ( ch < U'0' || ch > U'9' ) && ch != U'-' ) {
                    break;
                }
            }
            bool after_word = wordlen >= punc->spaces.min_word_length;
            auto itspaceend = it;
            size_t spacelen = 0;
            for( ; itspaceend < end && *itspaceend == U' '; ++itspaceend, ++spacelen ) {
            }
            if( after_word && itspaceend >= end ) {
                // treat spaces at the end of strings in concat expressions (+, += or <<) as deliberate
                if( !in_concat_expr && spacelen > 0 && spacelen <= punc->spaces.fix_end_max ) {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( it ), location( itspaceend ) );
                    auto diags = diag( location( it ), "unnecessary spaces at end of string." );
                    if( fixit ) {
                        diags << FixItHint::CreateRemoval( range );
                    }
                }
            } else if( after_word && *itspaceend == U'\n' ) {
                if( spacelen > 0 && spacelen <= punc->spaces.fix_line_end_max ) {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( it ), location( itspaceend ) );
                    auto diags = diag( location( it ), "unnecessary spaces at end of line." );
                    if( fixit ) {
                        diags << FixItHint::CreateRemoval( range );
                    }
                }
            } else if( itpunc <= beg ) {
                if( spacelen > 0 && spacelen <= punc->spaces.fix_line_start_max ) {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( it ), location( itspaceend ) );
                    auto diags = diag( location( it ), "undesired spaces after a punctuation that starts a string." );
                    if( fixit ) {
                        diags << FixItHint::CreateRemoval( range );
                    }
                }
            } else if( itpunc > beg && *( itpunc - 1 ) == U'\n' ) {
                if( spacelen > 0 && spacelen <= punc->spaces.fix_start_max ) {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( it ), location( itspaceend ) );
                    auto diags = diag( location( it ), "undesired spaces after a punctuation that starts a line." );
                    if( fixit ) {
                        diags << FixItHint::CreateRemoval( range );
                    }
                }
            } else if( after_word ) {
                if( spacelen >= punc->spaces.fix_spaces_min &&
                    spacelen < punc->spaces.fix_spaces_to ) {
                    auto diags = diag( location( it ),
                                       "insufficient spaces at this location.  %0 required, but only %1 found." )
                                 << static_cast<unsigned int>( punc->spaces.fix_spaces_to )
                                 << static_cast<unsigned int>( spacelen );
                    if( fixit ) {
                        diags << FixItHint::CreateInsertion( location( it ),
                                                             std::string( punc->spaces.fix_spaces_to - spacelen, ' ' ) );
                    }
                } else if( spacelen > punc->spaces.fix_spaces_to &&
                           spacelen <= punc->spaces.fix_spaces_max ) {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( itspaceend - ( spacelen - punc->spaces.fix_spaces_to ) ),
                                                      location( itspaceend ) );
                    auto diags = diag( location( it ),
                                       "excessive spaces at this location.  %0 required, but %1 found." )
                                 << static_cast<unsigned int>( punc->spaces.fix_spaces_to )
                                 << static_cast<unsigned int>( spacelen );
                    if( fixit ) {
                        diags << FixItHint::CreateRemoval( range );
                    }
                }
            }
            it = itspaceend;
        }
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
