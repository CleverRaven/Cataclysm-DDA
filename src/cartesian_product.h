#pragma once
#ifndef CATA_SRC_CARTESIAN_PRODUCT_H
#define CATA_SRC_CARTESIAN_PRODUCT_H

namespace cata
{

template<typename RangeOfRanges>
auto cartesian_product( const RangeOfRanges &input )
-> std::vector<std::vector<typename RangeOfRanges::value_type::value_type>>
{
    std::vector<std::vector<typename RangeOfRanges::value_type::value_type>> result;
    using Iterators = std::vector<typename RangeOfRanges::value_type::const_iterator>;
    Iterators iterators;
    iterators.reserve( input.size() );
    for( const auto &range : input ) {
        if( range.empty() ) {
            return result;
        }
        iterators.push_back( range.begin() );
    }

    while( true ) {
        // Append the new result
        result.emplace_back();
        auto &new_list = result.back();
        new_list.reserve( input.size() );
        for( typename Iterators::value_type it : iterators ) {
            new_list.push_back( *it );
        }

        // Increment the iterators
        size_t i = 0;
        for( ; i != input.size(); ++i ) {
            auto &which_it = iterators[i];
            const auto &range = input[i];
            ++which_it;
            if( which_it == range.end() ) {
                which_it = range.begin();
            } else {
                break;
            }
        }
        if( i == input.size() ) {
            return result;
        }
    }
}

} // namespace cata

#endif // CATA_SRC_CARTESIAN_PRODUCT_H
