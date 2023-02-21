#include "pinyin.h"
#include "third-party/pinyin/pinyin_data.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace pinyin
{
bool pinyin_match( const std::u32string &str, const std::u32string &qry )
{
    // we convert the data to an unordered map to lower the cost of looking up entries.
    // O(1) instead of O(n)
    static std::unordered_map<char32_t, std::vector<std::u32string>> indexed_pinyin_map;

    int combination_index = 0;                  //how many combinations have we tried
    bool all_combinations_tested = false;
    while( !all_combinations_tested ) {
        std::u32string current_combination;
        //longest pinyin is 6 letter for a character, so we pre-allocate here
        current_combination.reserve( str.length() * 6 );

        int combination = combination_index;    //a copy so the record will not be destoryed
        int total_combination = 1;              //the total possible amount of combinations

        for( const char32_t ch : str ) {
            //try to find the pinyins for the current character
            if( indexed_pinyin_map.find( ch ) == indexed_pinyin_map.end() ) {
                //not a known character
                current_combination += ch;
                continue;
            }
            const std::vector<std::u32string> &cur_char_pinyin_list = indexed_pinyin_map.at( ch );

            /*
            * This two lines iterate through all possible combinations.
            * combination % list.size() will give one of the index for this list
            * and then divide it by the list size ensure that for each of the next index,
            * we have tried all possible combinations for this index
            */
            current_combination += cur_char_pinyin_list.at( combination % cur_char_pinyin_list.size() );
            combination /= cur_char_pinyin_list.size();

            //we count the amount of total combinations possible to determine when to stop
            total_combination *= cur_char_pinyin_list.size();
        }

        if( current_combination.find( qry ) != std::u32string::npos ) {
            return true;
        }
        //increase combination index by 1, if it had tried all total combinations we return
        combination_index++;
        all_combinations_tested = ( combination_index >= total_combination );
    }


    return false;
}
} // namespace pinyin
