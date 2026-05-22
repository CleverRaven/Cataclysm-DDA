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

char32_t u32_to_lowercase( const char32_t ch )
{
    // Reference: https://en.wikibooks.org/wiki/Unicode/Character_reference/0000-0FFF
    if( ( 0x41 <= ch && ch <= 0x5a ) // A - Z
        || ( 0xc0 <= ch && ch <= 0xd6 ) // accented latin
        || ( 0xd8 <= ch && ch <= 0xde ) // accented latin
        || ( 0x391 <= ch && ch <= 0x3a1 ) // greek Α - Ρ
        || ( 0x3a3 <= ch && ch <= 0x3a9 ) // greek Σ - Ω
        || ( 0x410 <= ch && ch <= 0x42f ) ) { // cyrillic А - cyrillic Я
        return ch + 0x20;
    } else if( 0x400 <= ch && ch <= 0x40f ) {
        // cyrillic Ѐ - cyrillic Џ
        return ch + 0x50;
    } else if( ch == 0x178 ) { // latin Ÿ
        return 0xff;
    } else if( ch == 0x1e9e ) { // latin capital letter sharp s
        return 0x00df;
    } else if( ( 0x100 <= ch && ch <= 0x136 ) // latin Ā - latin Ķ
               || ( 0x14a <= ch && ch <= 0x176 ) // latin Ŋ - latin Ŷ
               || ( 0x460 <= ch && ch <= 0x480 ) // cyrillic Ѡ - cyrillic Ҁ
               || ( 0x490 <= ch && ch <= 0x4be )  // cyrillic Ґ - cyrillic Ҿ
               || ( 0x4d0 <= ch && ch <= 0x52e ) ) { // cyrillic Ӑ - cyrillic Ԯ
        if( ( ch & 1 ) == 0 ) {
            return ch + 1;
        }
    } else if( ( 0x139 <= ch && ch <= 0x147 ) // latin Ĺ - latin Ň
               || ( 0x179 <= ch && ch <= 0x17d ) // latin Ź - latin Ž
               || ( 0x4c1 <= ch && ch <= 0x4cb ) ) { // cyrillic Ӂ - cyrillic Ӌ
        if( ( ch & 1 ) == 1 ) {
            return ch + 1;
        }
    }
    return ch;
}

char32_t u32_to_uppercase( const char32_t ch )
{
    // Reference: https://en.wikibooks.org/wiki/Unicode/Character_reference/0000-0FFF
    if( ( 0x61 <= ch && ch <= 0x7a ) // a - z
        || ( 0xe0 <= ch && ch <= 0xf6 ) // accented latin
        || ( 0xf8 <= ch && ch <= 0xfe ) // accented latin
        || ( 0x3b1 <= ch && ch <= 0x3c1 ) // greek α - ρ
        || ( 0x3c3 <= ch && ch <= 0x3c9 ) // greek σ - ω
        || ( 0x430 <= ch && ch <= 0x44f ) ) { // cyrillic а - cyrillic я
        return ch - 0x20;
    } else if( 0x450 <= ch && ch <= 0x45f ) { // cyrillic ѐ - cyrillic џ
        return ch - 0x50;
    } else if( ch == 0xff ) { // latin ÿ
        return 0x178;
    } else if( ch == 0x00df ) {
        // latin small letter sharp s
        return 0x1e9e;
    } else if( ch == 0x3c2 ) { // greek final sigma
        return 0x3a3;
    } else if( ( 0x101 <= ch && ch <= 0x137 ) // latin ā - latin ķ
               || ( 0x14b <= ch && ch <= 0x177 ) // latin ŋ - latin ŷ
               || ( 0x461 <= ch && ch <= 0x481 ) // cyrillic ѡ - cyrillic ҁ
               || ( 0x491 <= ch && ch <= 0x4bf )  // cyrillic ґ - cyrillic ҿ
               || ( 0x4d1 <= ch && ch <= 0x52f ) ) { // cyrillic ӑ - cyrillic ԯ
        if( ( ch & 1 ) == 1 ) {
            return ch - 1;
        }
    } else if( ( 0x13a <= ch && ch <= 0x148 ) // latin Ĺ - latin Ň
               || ( 0x17a <= ch && ch <= 0x17e ) // latin ź - latin ž
               || ( 0x4c2 <= ch && ch <= 0x4cc ) ) { // cyrillic ӂ - cyrillic ӌ
        if( ( ch & 1 ) == 0 ) {
            return ch - 1;
        }
    }
    return ch;
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

bool u32_isspace( const char32_t ch )
{
    // Reference: ISO/IEC 30112 WD10 page 30
    // https://www.open-std.org/JTC1/SC35/WG5/docs/30112d10.pdf
    return ( 0x09U <= ch && ch <= 0x0DU ) || ch == 0x20U || ch == 0x1680U || ch == 0x180EU ||
           ( 0x2000U <= ch && ch <= 0x2006U ) || ( 0x2008U <= ch && ch <= 0x200AU ) || ch == 0x2028U ||
           ch == 0x2029U || ch == 0x205FU || ch == 0x3000U;
}

bool u32_ispunct( const char32_t ch )
{
    // Reference: ISO/IEC 30112 WD10 page 30-31
    // https://www.open-std.org/JTC1/SC35/WG5/docs/30112d10.pdf
    return ( 0x0021 <= ch && ch <= 0x002F ) ||
           ( 0x003A <= ch && ch <= 0x0040 ) ||
           ( 0x005B <= ch && ch <= 0x0060 ) ||
           ( 0x007B <= ch && ch <= 0x007E ) ||
           ( 0x00A0 <= ch && ch <= 0x00A9 ) ||
           ( 0x00AB <= ch && ch <= 0x00B4 ) ||
           ( 0x00B6 <= ch && ch <= 0x00B9 ) ||
           ( 0x00BB <= ch && ch <= 0x00BF ) ||
           ( ch == 0x00D7 ) ||
           ( ch == 0x00F7 ) ||
           ( 0x02C2 <= ch && ch <= 0x02C5 ) ||
           ( 0x02D2 <= ch && ch <= 0x02DF ) ||
           ( 0x02E5 <= ch && ch <= 0x02ED ) ||
           ( 0x02EF <= ch && ch <= 0x0344 ) ||
           ( 0x0346 <= ch && ch <= 0x036F ) ||
           ( 0x0374 <= ch && ch <= 0x0375 ) ||
           ( ch == 0x037E ) ||
           ( 0x0384 <= ch && ch <= 0x0385 ) ||
           ( ch == 0x0387 ) ||
           ( ch == 0x03F6 ) ||
           ( 0x0482 <= ch && ch <= 0x0486 ) ||
           ( 0x0488 <= ch && ch <= 0x0489 ) ||
           ( 0x055A <= ch && ch <= 0x055F ) ||
           ( 0x0589 <= ch && ch <= 0x058A ) ||
           ( 0x0591 <= ch && ch <= 0x05C7 ) ||
           ( 0x05F3 <= ch && ch <= 0x05F4 ) ||
           ( 0x0600 <= ch && ch <= 0x0603 ) ||
           ( 0x060B <= ch && ch <= 0x061B ) ||
           ( 0x061E <= ch && ch <= 0x061F ) ||
           ( 0x064B <= ch && ch <= 0x065E ) ||
           ( 0x066A <= ch && ch <= 0x066D ) ||
           ( ch == 0x0670 ) ||
           ( ch == 0x06D4 ) ||
           ( 0x06D6 <= ch && ch <= 0x06E4 ) ||
           ( 0x06E7 <= ch && ch <= 0x06ED ) ||
           ( 0x06FD <= ch && ch <= 0x06FE ) ||
           ( 0x0700 <= ch && ch <= 0x070D ) ||
           ( ch == 0x070F ) ||
           ( ch == 0x0711 ) ||
           ( 0x0730 <= ch && ch <= 0x074A ) ||
           ( 0x07A6 <= ch && ch <= 0x07B0 ) ||
           ( 0x07EB <= ch && ch <= 0x07F3 ) ||
           ( 0x07F6 <= ch && ch <= 0x07F9 ) ||
           ( ch == 0x0964 ) ||
           ( ch == 0x0965 ) ||
           ( ch == 0x0E2F ) ||
           ( ch == 0x0E3F ) ||
           ( ch == 0x0E46 ) ||
           ( ch == 0x0E4F ) ||
           ( 0x0E5A <= ch && ch <= 0x0E5B ) ||
           ( ch == 0x0EB1 ) ||
           ( 0x0EB4 <= ch && ch <= 0x0EB9 ) ||
           ( 0x0EBB <= ch && ch <= 0x0EBC ) ||
           ( 0x0EC8 <= ch && ch <= 0x0ECD ) ||
           ( 0x0F01 <= ch && ch <= 0x0F1F ) ||
           ( 0x0F2A <= ch && ch <= 0x0F3F ) ||
           ( 0x0F71 <= ch && ch <= 0x0F87 ) ||
           ( 0x0F90 <= ch && ch <= 0x0F97 ) ||
           ( 0x0F99 <= ch && ch <= 0x0FBC ) ||
           ( 0x0FBE <= ch && ch <= 0x0FCC ) ||
           ( 0x0FCE <= ch && ch <= 0x0FD4 ) ||
           ( 0x102B <= ch && ch <= 0x103F ) ||
           ( 0x104A <= ch && ch <= 0x104F ) ||
           ( 0x1056 <= ch && ch <= 0x1059 ) ||
           ( 0x105E <= ch && ch <= 0x1060 ) ||
           ( 0x1062 <= ch && ch <= 0x1064 ) ||
           ( 0x1067 <= ch && ch <= 0x106D ) ||
           ( 0x1071 <= ch && ch <= 0x1074 ) ||
           ( 0x1082 <= ch && ch <= 0x108D ) ||
           ( 0x108F <= ch && ch <= 0x1099 ) ||
           ( ch == 0x109E ) ||
           ( ch == 0x109F ) ||
           ( ch == 0x10FB ) ||
           ( 0x135F <= ch && ch <= 0x137C ) ||
           ( 0x1390 <= ch && ch <= 0x1399 ) ||
           ( 0x166D <= ch && ch <= 0x166E ) ||
           ( 0x169B <= ch && ch <= 0x169C ) ||
           ( 0x16EB <= ch && ch <= 0x16ED ) ||
           ( 0x1712 <= ch && ch <= 0x1714 ) ||
           ( 0x1732 <= ch && ch <= 0x1736 ) ||
           ( 0x1752 <= ch && ch <= 0x1753 ) ||
           ( 0x1772 <= ch && ch <= 0x1773 ) ||
           ( 0x17B4 <= ch && ch <= 0x17D6 ) ||
           ( 0x17D8 <= ch && ch <= 0x17DB ) ||
           ( ch == 0x17DD ) ||
           ( 0x17F0 <= ch && ch <= 0x17F9 ) ||
           ( 0x1800 <= ch && ch <= 0x180D ) ||
           ( ch == 0x18A9 ) ||
           ( 0x1920 <= ch && ch <= 0x192B ) ||
           ( 0x1930 <= ch && ch <= 0x193B ) ||
           ( ch == 0x1940 ) ||
           ( 0x1944 <= ch && ch <= 0x1945 ) ||
           ( 0x19B0 <= ch && ch <= 0x19C0 ) ||
           ( 0x19C8 <= ch && ch <= 0x19C9 ) ||
           ( 0x19DE <= ch && ch <= 0x19FF ) ||
           ( 0x1A17 <= ch && ch <= 0x1A1B ) ||
           ( 0x1A1E <= ch && ch <= 0x1A1F ) ||
           ( 0x1B00 <= ch && ch <= 0x1B04 ) ||
           ( 0x1B34 <= ch && ch <= 0x1B44 ) ||
           ( 0x1B5A <= ch && ch <= 0x1B7C ) ||
           ( 0x1B80 <= ch && ch <= 0x1B82 ) ||
           ( 0x1BA1 <= ch && ch <= 0x1BAA ) ||
           ( 0x1C24 <= ch && ch <= 0x1C37 ) ||
           ( 0x1C3B <= ch && ch <= 0x1C3F ) ||
           ( 0x1C7E <= ch && ch <= 0x1C7F ) ||
           ( 0x1DC0 <= ch && ch <= 0x1DE6 ) ||
           ( 0x1DFE <= ch && ch <= 0x1DFF ) ||
           ( ch == 0x1FBD ) ||
           ( 0x1FBF <= ch && ch <= 0x1FC1 ) ||
           ( 0x1FCD <= ch && ch <= 0x1FCF ) ||
           ( 0x1FDD <= ch && ch <= 0x1FDF ) ||
           ( 0x1FED <= ch && ch <= 0x1FEF ) ||
           ( 0x1FFD <= ch && ch <= 0x1FFE ) ||
           ( ch == 0x2007 ) ||
           ( 0x200B <= ch && ch <= 0x2027 ) ||
           ( 0x202A <= ch && ch <= 0x205E ) ||
           ( 0x2060 <= ch && ch <= 0x2064 ) ||
           ( 0x206A <= ch && ch <= 0x2070 ) ||
           ( 0x2074 <= ch && ch <= 0x207E ) ||
           ( 0x2080 <= ch && ch <= 0x208E ) ||
           ( 0x20A0 <= ch && ch <= 0x20B5 ) ||
           ( 0x20D0 <= ch && ch <= 0x20F0 ) ||
           ( 0x2100 <= ch && ch <= 0x2101 ) ||
           ( 0x2103 <= ch && ch <= 0x2106 ) ||
           ( 0x2108 <= ch && ch <= 0x2109 ) ||
           ( ch == 0x2114 ) ||
           ( 0x2116 <= ch && ch <= 0x2118 ) ||
           ( 0x211E <= ch && ch <= 0x2123 ) ||
           ( ch == 0x2125 ) ||
           ( ch == 0x2127 ) ||
           ( ch == 0x212E ) ||
           ( 0x213A <= ch && ch <= 0x213B ) ||
           ( 0x2140 <= ch && ch <= 0x2144 ) ||
           ( 0x214A <= ch && ch <= 0x214D ) ||
           ( 0x2153 <= ch && ch <= 0x215F ) ||
           ( 0x2190 <= ch && ch <= 0x23E7 ) ||
           ( 0x2400 <= ch && ch <= 0x2426 ) ||
           ( 0x2440 <= ch && ch <= 0x244A ) ||
           ( 0x2460 <= ch && ch <= 0x249B ) ||
           ( 0x24EA <= ch && ch <= 0x269D ) ||
           ( 0x26A0 <= ch && ch <= 0x26C3 ) ||
           ( 0x2701 <= ch && ch <= 0x2704 ) ||
           ( 0x2706 <= ch && ch <= 0x2709 ) ||
           ( 0x270C <= ch && ch <= 0x2727 ) ||
           ( 0x2729 <= ch && ch <= 0x274B ) ||
           ( ch == 0x274D ) ||
           ( 0x274F <= ch && ch <= 0x2752 ) ||
           ( ch == 0x2756 ) ||
           ( 0x2758 <= ch && ch <= 0x275E ) ||
           ( 0x2761 <= ch && ch <= 0x2794 ) ||
           ( 0x2798 <= ch && ch <= 0x27AF ) ||
           ( 0x27B1 <= ch && ch <= 0x27BE ) ||
           ( 0x27C0 <= ch && ch <= 0x27CA ) ||
           ( ch == 0x27CC ) ||
           ( 0x27D0 <= ch && ch <= 0x27EF ) ||
           ( 0x27F0 <= ch && ch <= 0x2B4C ) ||
           ( 0x2B50 <= ch && ch <= 0x2B54 ) ||
           ( 0x2DE0 <= ch && ch <= 0x2DFF ) ||
           ( 0x2CE5 <= ch && ch <= 0x2CEA ) ||
           ( 0x2CF9 <= ch && ch <= 0x2CFF ) ||
           ( 0x2E00 <= ch && ch <= 0x2E30 ) ||
           ( 0x2E80 <= ch && ch <= 0x2E99 ) ||
           ( 0x2E9B <= ch && ch <= 0x2EF3 ) ||
           ( 0x2F00 <= ch && ch <= 0x2FD5 ) ||
           ( 0x2FF0 <= ch && ch <= 0x2FFB ) ||
           ( 0x3001 <= ch && ch <= 0x3004 ) ||
           ( 0x3008 <= ch && ch <= 0x3020 ) ||
           ( 0x302A <= ch && ch <= 0x3030 ) ||
           ( 0x3036 <= ch && ch <= 0x3037 ) ||
           ( 0x303D <= ch && ch <= 0x303F ) ||
           ( 0x3099 <= ch && ch <= 0x309C ) ||
           ( ch == 0x30A0 ) ||
           ( ch == 0x30FB ) ||
           ( 0x3190 <= ch && ch <= 0x319F ) ||
           ( 0x31C0 <= ch && ch <= 0x31CF ) ||
           ( 0x3200 <= ch && ch <= 0x321E ) ||
           ( 0x3220 <= ch && ch <= 0x3243 ) ||
           ( 0x3250 <= ch && ch <= 0x32FE ) ||
           ( 0x3300 <= ch && ch <= 0x33FF ) ||
           ( 0x4DC0 <= ch && ch <= 0x4DFF ) ||
           ( 0xA490 <= ch && ch <= 0xA4C6 ) ||
           ( 0xA60C <= ch && ch <= 0xA60F ) ||
           ( 0xA66F <= ch && ch <= 0xA673 ) ||
           ( 0xA67C <= ch && ch <= 0xA67F ) ||
           ( 0xA700 <= ch && ch <= 0xA716 ) ||
           ( 0xA720 <= ch && ch <= 0xA721 ) ||
           ( ch == 0xA802 ) ||
           ( ch == 0xA806 ) ||
           ( ch == 0xA80B ) ||
           ( 0xA823 <= ch && ch <= 0xA82B ) ||
           ( 0xA874 <= ch && ch <= 0xA877 ) ||
           ( ch == 0xA880 ) ||
           ( ch == 0xA881 ) ||
           ( 0xA8B4 <= ch && ch <= 0xA8C4 ) ||
           ( 0xA8CE <= ch && ch <= 0xA8CF ) ||
           ( 0xA92E <= ch && ch <= 0xA92F ) ||
           ( 0xA947 <= ch && ch <= 0xA953 ) ||
           ( ch == 0xA95F ) ||
           ( 0xAA29 <= ch && ch <= 0xAA36 ) ||
           ( ch == 0xAA43 ) ||
           ( 0xAA4C <= ch && ch <= 0xAA4D ) ||
           ( 0xAA5C <= ch && ch <= 0xAA5F ) ||
           ( 0xE000 <= ch && ch <= 0xF8FF ) ||
           ( ch == 0xFB1E ) ||
           ( ch == 0xFB29 ) ||
           ( 0xFD3E <= ch && ch <= 0xFD3F ) ||
           ( 0xFDFC <= ch && ch <= 0xFDFD ) ||
           ( 0xFE00 <= ch && ch <= 0xFE19 ) ||
           ( 0xFE20 <= ch && ch <= 0xFE26 ) ||
           ( 0xFE30 <= ch && ch <= 0xFE52 ) ||
           ( 0xFE54 <= ch && ch <= 0xFE66 ) ||
           ( 0xFE68 <= ch && ch <= 0xFE6B ) ||
           ( ch == 0xFEFF ) ||
           ( 0xFF01 <= ch && ch <= 0xFF0F ) ||
           ( 0xFF1A <= ch && ch <= 0xFF20 ) ||
           ( 0xFF3B <= ch && ch <= 0xFF40 ) ||
           ( 0xFF5B <= ch && ch <= 0xFF65 ) ||
           ( 0xFFE0 <= ch && ch <= 0xFFE6 ) ||
           ( 0xFFE8 <= ch && ch <= 0xFFEE ) ||
           ( 0xFFF9 <= ch && ch <= 0xFFFD ) ||
           ( 0x00010100 <= ch && ch <= 0x00010102 ) ||
           ( 0x00010107 <= ch && ch <= 0x00010133 ) ||
           ( 0x00010137 <= ch && ch <= 0x0001013F ) ||
           ( 0x00010175 <= ch && ch <= 0x0001018A ) ||
           ( 0x00010320 <= ch && ch <= 0x00010323 ) ||
           ( ch == 0x0001039F ) ||
           ( ch == 0x000103D0 ) ||
           ( 0x00010916 <= ch && ch <= 0x00010919 ) ||
           ( ch == 0x0001091F ) ||
           ( 0x00010A01 <= ch && ch <= 0x00010A03 ) ||
           ( 0x00010A05 <= ch && ch <= 0x00010A06 ) ||
           ( 0x00010A0C <= ch && ch <= 0x00010A0F ) ||
           ( 0x00010A38 <= ch && ch <= 0x00010A3A ) ||
           ( 0x00010A3F <= ch && ch <= 0x00010A47 ) ||
           ( 0x00010A50 <= ch && ch <= 0x00010A58 ) ||
           ( 0x00012470 <= ch && ch <= 0x00012473 ) ||
           ( 0x0001D000 <= ch && ch <= 0x0001D0F5 ) ||
           ( 0x0001D100 <= ch && ch <= 0x0001D126 ) ||
           ( 0x0001D129 <= ch && ch <= 0x0001D1DD ) ||
           ( 0x0001D200 <= ch && ch <= 0x0001D245 ) ||
           ( 0x0001D300 <= ch && ch <= 0x0001D356 ) ||
           ( 0x0001D360 <= ch && ch <= 0x0001D371 ) ||
           ( ch == 0x0001D6C1 ) ||
           ( ch == 0x0001D6DB ) ||
           ( ch == 0x0001D6FB ) ||
           ( ch == 0x0001D715 ) ||
           ( ch == 0x0001D735 ) ||
           ( ch == 0x0001D74F ) ||
           ( ch == 0x0001D76F ) ||
           ( ch == 0x0001D789 ) ||
           ( ch == 0x0001D7A9 ) ||
           ( ch == 0x0001D7C3 ) ||
           ( ch == 0x000E0001 ) ||
           ( 0x000E0020 <= ch && ch <= 0x000E007F ) ||
           ( 0x000E0100 <= ch && ch <= 0x000E01EF ) ||
           ( 0x000F0000 <= ch && ch <= 0x000FFFFD ) ||
           ( 0x00100000 <= ch && ch <= 0x0010FFFD );
}
