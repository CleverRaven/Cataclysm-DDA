#include "unicode.h"

#include <algorithm>
#include <vector>

// https://en.wikipedia.org/wiki/Unicode_block
static std::vector<std::pair<uint32_t, uint32_t>> cjk_or_emoji_ranges = {
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
