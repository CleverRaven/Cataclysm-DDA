#pragma once
#ifndef CATA_SRC_LRU_CACHE_H
#define CATA_SRC_LRU_CACHE_H

#include <list>
#include <unordered_map>
#include <utility>

template<typename Key, typename Value>
class lru_cache
{
    public:
        using Pair = std::pair<Key, Value>;

        Value get( const Key &, const Value &default_ ) const;
        void insert( int limit, const Key &, const Value & );
        void remove( const Key & );

        void clear();
    protected:
        void trim( int limit );
        void touch( typename std::list<Pair>::iterator iter ) const;
        mutable std::list<Pair> ordered_list;
        std::unordered_map<Key, typename std::list<Pair>::iterator> map;
};

template<typename Key, typename Value>
inline Value lru_cache<Key, Value>::get( const Key &pos, const Value &default_ ) const
{
    if( const auto found = this->map.find( pos ); found != this->map.end() ) {
        const auto list_iter = found->second;
        this->touch( list_iter );
        return list_iter->second;
    }
    return default_;
}

template<typename Key, typename Value>
inline void lru_cache<Key, Value>::remove( const Key &pos )
{
    if( const auto found = map.find( pos ); found != map.end() ) {
        ordered_list.erase( found->second );
        map.erase( found );
    }
}

template<typename Key, typename Value>
inline void lru_cache<Key, Value>::insert( int limit, const Key &pos, const Value &t )
{
    auto found = map.find( pos );
    if( found == map.end() ) {
        // Need new entry in map.  Make the new list entry and point to it.
        ordered_list.emplace_back( pos, t );
        map.emplace( pos, std::prev( ordered_list.end() ) );
        trim( limit );
    } else {
        // Splice existing entry to the back.  Does not invalidate the
        // iterator, so no need to change the map.
        auto list_iterator = found->second;
        touch( list_iterator );
        // Update the moved item
        list_iterator->second = t;
    }
}

template<typename Key, typename Value>
inline void lru_cache<Key, Value>::trim( int limit )
{
    while( map.size() > static_cast<size_t>( limit ) ) {
        map.erase( ordered_list.front().first );
        ordered_list.pop_front();
    }
}

template<typename Key, typename Value>
inline void lru_cache<Key, Value>::touch( typename std::list<Pair>::iterator iter ) const
{
    ordered_list.splice( ordered_list.end(), ordered_list, iter );
}

template<typename Key, typename Value>
inline void lru_cache<Key, Value>::clear()
{
    map.clear();
    ordered_list.clear();
}

#endif // CATA_SRC_LRU_CACHE_H
