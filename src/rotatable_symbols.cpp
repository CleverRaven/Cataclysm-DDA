#include "rotatable_symbols.h"

#include <cstdlib>
#include <array>
#include <vector>
#include <algorithm>
#include <cstdint>

#include "generic_factory.h"
#include "json.h"
#include "string_formatter.h"
#include "catacharset.h"

namespace
{

struct rotatable_symbol {
    uint32_t symbol;
    std::array<uint32_t, 3> rotated_symbol;

    bool operator<( const uint32_t &rhs ) const {
        return symbol < rhs;
    }

    bool operator<( const rotatable_symbol &rhs ) const {
        return symbol < rhs.symbol;
    }
};

std::vector<rotatable_symbol> symbols;

} // anonymous namespace

namespace rotatable_symbols
{

void load( JsonObject &jo, const std::string &src )
{
    const std::string tuple_key = "tuple";
    const bool strict = src == "dda";

    std::vector<std::string> tuple_temp;

    mandatory( jo, false, tuple_key, tuple_temp );

    if( tuple_temp.size() != 2 && tuple_temp.size() != 4 ) {
        jo.throw_error( "Invalid size.  Must be either 2 or 4.", tuple_key );
    }
    std::vector<uint32_t> tuple;
    tuple.reserve( tuple_temp.size() );
    for( std::string &elem : tuple_temp ) {
        tuple.emplace_back( UTF8_getch( elem ) );
    }

    rotatable_symbol temp_entry;

    for( auto iter = tuple.cbegin(); iter != tuple.cend(); ++iter ) {
        const auto entry_iter = std::lower_bound( symbols.begin(), symbols.end(), *iter );
        const bool found = entry_iter != symbols.end() && entry_iter->symbol == *iter;

        if( strict && found ) {
            jo.throw_error( string_format( "Symbol %ld was already defined.", *iter ), tuple_key );
        }

        rotatable_symbol &entry = found ? *entry_iter : temp_entry;

        entry.symbol = *iter;

        auto rotation_iter = iter;
        for( auto &element : entry.rotated_symbol ) {
            if( ++rotation_iter == tuple.cend() ) {
                rotation_iter = tuple.cbegin();
            }

            element = *rotation_iter;
        }

        if( !found ) {
            symbols.insert( entry_iter, entry );
        }
    }
}

void reset()
{
    symbols.clear();
}

uint32_t get( const uint32_t &symbol, int n )
{
    n = std::abs( n ) % 4;

    if( n == 0 ) {
        return symbol;
    }

    const auto iter = std::lower_bound( symbols.begin(), symbols.end(), symbol );
    const bool found = iter != symbols.end() && iter->symbol == symbol;

    return found ? iter->rotated_symbol[n - 1] : symbol;
}

} // namespace rotatable_symbols
