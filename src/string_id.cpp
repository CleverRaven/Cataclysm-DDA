#include <unordered_map>
#include <vector>
#include "string_id.h"

using MapType = std::unordered_map<std::string, int>;
using VecType = std::vector<const std::string *>;

inline static MapType &get_map()
{
    static MapType map{};
    return map;
}

inline static VecType &get_vec()
{
    static VecType vec{};
    return vec;
}

template<typename S>
inline static int universal_string_id_intern( S &&s )
{
    int next_id = get_vec().size();
    const auto &pair = get_map().insert( std::make_pair( std::forward<S>( s ), next_id ) );
    if( pair.second ) { // inserted
        get_vec().push_back( &pair.first->first );
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
    return *get_vec()[id];
}

int string_identity_static::empty_interned_string()
{
    static int empty_string_id = string_id_intern( "" );
    return empty_string_id;
}