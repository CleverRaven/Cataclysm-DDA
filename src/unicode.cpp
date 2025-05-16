#include "unicode.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

// https://en.wikipedia.org/wiki/Unicode_block
static const std::vector<std::pair<uint32_t, uint32_t>> cjk_or_emoji_ranges = {
    { 0x1100, 0x11FF }, // Hangul Jamo
    { 0x2600, 0x26FF }, // Miscellaneous Symbols
    { 0x2700, 0x27BF }, // Dingbats
    { 0x2E80, 0x2EFF }, // CJK Radicals Supplement
    { 0x2F00, 0x2FDF }, // Kangxi Radicals
    { 0x2FF0, 0x2FFF }, // Ideographic Description Characters
    { 0x3000, 0x303F }, // CJK Symbols and Punctuation
    { 0x3040, 0x309F }, // Hiragana
    { 0x30A0, 0x30FF }, // Katakana
    { 0x3100, 0x312F }, // Bopomofo
    { 0x3130, 0x318F }, // Hangul Compatibility Jamo
    { 0x3190, 0x319F }, // Kanbun
    { 0x31A0, 0x31BF }, // Bopomofo Extended
    { 0x31C0, 0x31EF }, // CJK Strokes
    { 0x31F0, 0x31FF }, // Katakana Phonetic Extensions
    { 0x3200, 0x32FF }, // Enclosed CJK Letters and Months
    { 0x3300, 0x33FF }, // CJK Compatibility
    { 0x3400, 0x4DBF }, // CJK Unified Ideographs Extension A
    { 0x4DC0, 0x4DFF }, // Yijing Hexagram Symbols
    { 0x4E00, 0x9FFF }, // CJK Unified Ideographs
    { 0xA960, 0xA97F }, // Hangul Jamo Extended-A
    { 0xAC00, 0xD7AF }, // Hangul Syllables
    { 0xD7B0, 0xD7FF }, // Hangul Jamo Extended-B
    { 0xF900, 0xFAFF }, // CJK Compatibility Ideographs
    { 0xFE30, 0xFE4F }, // CJK Compatibility Forms
    { 0x1AFF0, 0x1AFFF }, // Kana Extended-B
    { 0x1B000, 0x1B0FF }, // Kana Supplement
    { 0x1B100, 0x1B12F }, // Kana Extended-A
    { 0x1B130, 0x1B16F }, // Small Kana Extension
    { 0x1F000, 0x1F02F }, // Mahjong Tiles
    { 0x1F030, 0x1F09F }, // Domino Tiles
    { 0x1F0A0, 0x1F0FF }, // Playing Cards
    { 0x1F100, 0x1F1FF }, // Enclosed Alphanumeric Supplement
    { 0x1F200, 0x1F2FF }, // Enclosed Ideographic Supplement
    { 0x1F300, 0x1F5FF }, // Miscellaneous Symbols and Pictographs
    { 0x1F600, 0x1F64F }, // Emoticons
    { 0x1F650, 0x1F67F }, // Ornamental Dingbats
    { 0x1F680, 0x1F6FF }, // Transport and Map Symbols
    { 0x1F700, 0x1F77F }, // Alchemical Symbols
    { 0x1F900, 0x1F9FF }, // Supplemental Symbols and Pictographs
    { 0x1FA70, 0x1FAFF }, // Symbols and Pictographs Extended-A
    { 0x20000, 0x2A6DF }, // CJK Unified Ideographs Extension B
    { 0x2A700, 0x2B73F }, // CJK Unified Ideographs Extension C
    { 0x2B740, 0x2B81F }, // CJK Unified Ideographs Extension D
    { 0x2B820, 0x2CEAF }, // CJK Unified Ideographs Extension E
    { 0x2CEB0, 0x2EBEF }, // CJK Unified Ideographs Extension F
    { 0x2F800, 0x2FA1F }, // CJK Compatibility Ideographs Supplement
    { 0x30000, 0x3134F }, // CJK Unified Ideographs Extension G
};

static bool compare_range_end( const std::pair<uint32_t, uint32_t> &range,
                               const uint32_t value )
{
    return range.second < value;
}

bool is_cjk_or_emoji( const uint32_t ch )
{
    const auto it = std::lower_bound( cjk_or_emoji_ranges.begin(),
                                      cjk_or_emoji_ranges.end(),
                                      ch, compare_range_end );
    return it != cjk_or_emoji_ranges.end() && ch >= it->first;
}

void u32_to_lowercase( char32_t &ch )
{
    // Reference: https://en.wikibooks.org/wiki/Unicode/Character_reference/0000-0FFF
    if( ( 0x41 <= ch && ch <= 0x5a ) // A - Z
        || ( 0xc0 <= ch && ch <= 0x00de ) // accented latin
        || ( 0x410 <= ch && ch <= 0x42f ) ) { // cyrillic А - cyrillic Я
        ch += 0x20;
    } else if( 0x401 <= ch && ch <= 0x40f ) { // cyrillic Ё - cyrillic Џ
        ch += 0x50;
    } else if( ch == 0x178 ) { // latin Ÿ
        ch = 0xff;
    } else if( ( 0x100 <= ch && ch <= 0x137 ) // latin Ā - latin Ķ
               || ( 0x14a <= ch && ch <= 0x177 ) // latin Ŋ - latin Ŷ
               || ( 0x460 <= ch && ch <= 0x481 ) // cyrillic Ѡ - cyrillic Ҁ
               || ( 0x490 <= ch && ch <= 0x4bf )  // cyrillic Ґ - cyrillic Ҿ
               || ( 0x4d0 <= ch && ch <= 0x52f ) ) { // cyrillic Ӑ - cyrillic Ԯ
        if( ( ch & 1 ) == 0 ) {
            ch += 1;
        }
    } else if( ( 0x139 <= ch && ch <= 0x148 ) // latin Ĺ - latin Ň
               || ( 0x179 <= ch && ch <= 0x17e ) // latin Ź - latin Ž
               || ( 0x4c1 <= ch && ch <= 0x4cc ) ) { // cyrillic Ӂ - cyrillic Ӌ
        if( ( ch & 1 ) == 1 ) {
            ch += 1;
        }
    }
}

void remove_accent( char32_t &ch )
{
    static const std::vector<std::pair<char32_t, std::set<char32_t>>> accented_characters {
        {U'a', {U'á', U'â', U'ä', U'à', U'ã', U'ā', U'å'}},
        {U'e', {U'é', U'ê', U'ë', U'è', U'ē', U'ę'}},
        {U'i', {U'í', U'î', U'ï', U'ì', U'ī'}},
        {U'o', {U'ó', U'ô', U'ö', U'ò', U'õ', U'ō'}},
        {U'u', {U'ú', U'û', U'ü', U'ù', U'ū'}},
        {U'n', {U'ñ', U'ń'}},
        {U'c', {U'ć', U'ç', U'č'}},
        {U'е', {U'ё'}}
    };

    static std::map<char32_t, char32_t> lookup_table;
    if( lookup_table.empty() ) {
        for( const std::pair<char32_t, std::set<char32_t>> &p : accented_characters ) {
            for( char32_t accented : p.second ) {
                lookup_table[accented] = p.first;
            }
        }
    }

    auto it = lookup_table.find( ch );
    if( it != lookup_table.end() ) {
        ch = it->second;
    }
}
