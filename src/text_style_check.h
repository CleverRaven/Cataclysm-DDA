#pragma once
#ifndef CATA_SRC_TEXT_STYLE_CHECK_H
#define CATA_SRC_TEXT_STYLE_CHECK_H

// This is used in both the game itself and the clang-tidy check,
// so only system headers should be included here.
#include <functional>
#include <string>

enum class text_style_fix : int {
    removal,
    insertion,
    replacement
};

enum class fix_end_of_string_spaces : int {
    no,
    yes
};

enum class escape_unicode : int {
    no,
    yes
};

// Check text style in the c++ code and json files.
template <typename Iter>
void text_style_check( Iter beg, Iter end,
                       const fix_end_of_string_spaces eos_spaces,
                       const escape_unicode esc_unicode,
                       const std::function<void( text_style_fix, const std::string & /*msg*/,
                               const Iter & /*beg*/, const Iter & /*end*/,
                               const Iter & /*at*/,
                               const Iter & /*from*/, const Iter & /*to*/,
                               const std::string & /*fix*/ )> &suggest_fix )
{
    struct punctuation {
        std::u32string symbol;
        std::u32string follow;
        struct {
            bool check = false;
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
            bool yes = false;
            std::string str {};
            std::string escaped {};
            std::string sym_desc {};
            std::string replace_desc {};
        } replace;
    };
    // always put the longest (in u32) symbols at the front, since we'll iterate
    // and search for them in this order.
    // *INDENT-OFF*
    static const std::array<punctuation, 14> punctuations = {{
        // symbol,follow,    spaces,                                 replace,
        //                    check,  len, num spc,  end,start,before    yes,   string,     escaped,  symbol desc,      replc desc
        { U"...",    U"",   {  true, 0, 1, 0, 0, 0, 2, 2, 2, 2, 0 }, {  true, "\u2026", R"(\u2026)", "three dots",      "ellipsis" } },
        { U"::",     U"",   { false,                              }, { false,                                                      } },
        { U"!=",     U"",   { false,                              }, { false,                                                      } },
        { U"\r\n",   U"",   { false,                              }, {  true, R"(\n)",  R"(\n)",     "carriage return", "new line" } },
        { U"\u2026", U"",   {  true, 0, 1, 0, 0, 0, 2, 2, 2, 2, 0 }, { false,                                                      } },
        { U".",      U"",   {  true, 0, 3, 1, 3, 2, 0, 2, 0, 0, 0 }, { false,                                                      } },
        { U";",      U"",   {  true, 0, 1, 1, 2, 1, 1, 1, 0, 0, 1 }, { false,                                                      } },
        { U"!",      U"!?", {  true, 0, 1, 1, 3, 2, 2, 2, 0, 0, 1 }, { false,                                                      } },
        { U"?",      U"!?", {  true, 0, 1, 1, 3, 2, 2, 2, 0, 0, 1 }, { false,                                                      } },
        { U":",      U"",   {  true, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0 }, { false,                                                      } },
        { U",",      U"",   {  true, 0, 1, 1, 2, 1, 0, 1, 0, 0, 1 }, { false,                                                      } },
        { U"\r",     U"",   { false,                              }, {  true, R"(\n)",  R"(\n)",     "carriage return", "new line" } },
        { U"\n",     U"",   {  true, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1 }, { false,                                                      } },
        { U"\t",     U"",   { false,                              }, {  true, "    ",   "    ",      "tab",             "spaces"   } },
    }};
    // *INDENT-ON*

    const size_t text_length = std::distance( beg, end );
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
            const std::string &replace = esc_unicode == escape_unicode::yes ?
                                         punc->replace.escaped : punc->replace.str;
            suggest_fix( text_style_fix::replacement,
                         punc->replace.replace_desc + " preferred over " + punc->replace.sym_desc + ".",
                         beg, end, itpunc, itpunc, it, replace );
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
                suggest_fix( text_style_fix::removal,
                             "unnecessary spaces before this location.",
                             beg, end, itpunc, itspacebefore, itpunc, {} );
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
                if( eos_spaces == fix_end_of_string_spaces::yes &&
                    spacelen > 0 && spacelen <= punc->spaces.fix_end_max ) {
                    suggest_fix( text_style_fix::removal,
                                 "unnecessary spaces at end of string.",
                                 beg, end, it, it, itspaceend, {} );
                }
            } else if( after_word && *itspaceend == U'\n' ) {
                if( spacelen > 0 && spacelen <= punc->spaces.fix_line_end_max ) {
                    suggest_fix( text_style_fix::removal,
                                 "unnecessary spaces at end of line.",
                                 beg, end, it, it, itspaceend, {} );
                }
            } else if( itpunc <= beg ) {
                if( spacelen > 0 && spacelen <= punc->spaces.fix_line_start_max ) {
                    suggest_fix( text_style_fix::removal,
                                 "undesired spaces after a punctuation that starts a string.",
                                 beg, end, it, it, itspaceend, {} );
                }
            } else if( itpunc > beg && *( itpunc - 1 ) == U'\n' ) {
                if( spacelen > 0 && spacelen <= punc->spaces.fix_start_max ) {
                    suggest_fix( text_style_fix::removal,
                                 "undesired spaces after a punctuation that starts a line.",
                                 beg, end, it, it, itspaceend, {} );
                }
            } else if( after_word ) {
                if( spacelen >= punc->spaces.fix_spaces_min &&
                    spacelen < punc->spaces.fix_spaces_to ) {
                    suggest_fix( text_style_fix::insertion,
                                 "insufficient spaces at this location.  " +
                                 std::to_string( punc->spaces.fix_spaces_to ) +
                                 " required, but only " + std::to_string( spacelen ) +
                                 " found.",
                                 beg, end, it, it, it,
                                 std::string( punc->spaces.fix_spaces_to - spacelen, ' ' ) );
                } else if( spacelen > punc->spaces.fix_spaces_to &&
                           spacelen <= punc->spaces.fix_spaces_max ) {
                    suggest_fix( text_style_fix::removal,
                                 "excessive spaces at this location.  " +
                                 std::to_string( punc->spaces.fix_spaces_to ) +
                                 " required, but " + std::to_string( spacelen ) +
                                 " found.",
                                 beg, end, it, itspaceend - ( spacelen - punc->spaces.fix_spaces_to ),
                                 itspaceend, {} );
                }
            }
            it = itspaceend;
        }
    }
}

#endif // CATA_SRC_TEXT_STYLE_CHECK_H
