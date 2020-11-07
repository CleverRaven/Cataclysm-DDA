#include <unordered_map>
#include <vector>
#include "string_id.h"

namespace
{
using InternMapType = std::unordered_map<std::string, int>;
using ReverseLookupVecType = std::vector<const std::string *>;
} // namespace

inline static InternMapType &get_intern_map()
{
    static InternMapType map{};
    return map;
}

inline static ReverseLookupVecType &get_reverse_lookup_vec()
{
    static ReverseLookupVecType vec{};
    return vec;
}

template<typename S>
inline static int universal_string_id_intern( S &&s )
{
    int next_id = get_reverse_lookup_vec().size();
    const auto &pair = get_intern_map().emplace( std::forward<S>( s ), next_id );
    if( pair.second ) { // inserted
        get_reverse_lookup_vec().push_back( &pair.first->first );
    }
    return pair.first->second;
}

int string_identity_static::string_id_intern( const std::string &s )
{
    return universal_string_id_intern( s );
}

int string_identity_static::string_id_intern( std::string &s )
{
    return universal_string_id_intern( s );
}

int string_identity_static::string_id_intern( std::string &&s )
{
    return universal_string_id_intern( std::move( s ) );
}

const std::string &string_identity_static::get_interned_string( int id )
{
    return *get_reverse_lookup_vec()[id];
}

int string_identity_static::empty_interned_string()
{
    static int empty_string_id = string_id_intern( "" );
    return empty_string_id;
}
