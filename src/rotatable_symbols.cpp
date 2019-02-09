#include "rotatable_symbols.h"

#include <array>
#include <vector>

#include "generic_factory.h"
#include "json.h"
#include "string_formatter.h"

namespace
{

struct rotatable_symbol {
    long sym;
    std::array<long, 3> rotated_sym;

    bool operator<( const long &rhs ) const {
        return sym < rhs;
    }

    bool operator<( const rotatable_symbol &rhs ) const {
        return sym < rhs.sym;
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

    std::vector<long> tuple;

    mandatory( jo, false, tuple_key, tuple );

    if( tuple.size() != 2 && tuple.size() != 4 ) {
        jo.throw_error( "Invalid size.  Must be either 2 or 4.", tuple_key );
    }

    rotatable_symbol temp_entry;

    for( auto iter = tuple.cbegin(); iter != tuple.cend(); ++iter ) {
        const auto entry_iter = std::lower_bound( symbols.begin(), symbols.end(), *iter );
        const bool found = entry_iter != symbols.end() && entry_iter->sym == *iter;

        if( strict && found ) {
            jo.throw_error( string_format( "Symbol %ld was already defined.", *iter ), tuple_key );
        }

        rotatable_symbol &entry = found ? *entry_iter : temp_entry;

        entry.sym = *iter;

        auto rotation_iter = iter;
        for( auto &element : entry.rotated_sym ) {
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

long get( long sym, int n )
{
    n = std::abs( n ) % 4;

    if( n == 0 ) {
        return sym;
    }

    const auto iter = std::lower_bound( symbols.begin(), symbols.end(), sym );
    const bool found = iter != symbols.end() && iter->sym == sym;

    return found ? iter->rotated_sym[n - 1] : sym;
}

} // namespace rotatable_symbols
