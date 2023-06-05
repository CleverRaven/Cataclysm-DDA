#pragma once
#ifndef CATA_SRC_BODY_PART_SET_H
#define CATA_SRC_BODY_PART_SET_H

#include "flat_set.h"
#include "type_id.h"

class JsonOut;
class JsonValue;

class body_part_set
{
    private:

        cata::flat_set<bodypart_str_id> parts;

        explicit body_part_set( const cata::flat_set<bodypart_str_id> &other ) : parts( other ) {}

    public:
        body_part_set() = default;
        body_part_set( std::initializer_list<bodypart_str_id> bps ) {
            for( const bodypart_str_id &bp : bps ) {
                set( bp );
            }
        }
        body_part_set unify_set( const body_part_set &rhs );
        body_part_set intersect_set( const body_part_set &rhs );

        body_part_set make_intersection( const body_part_set &rhs ) const;
        body_part_set substract_set( const body_part_set &rhs );

        void fill( const std::vector<bodypart_id> &bps );

        bool test( const bodypart_str_id &bp ) const {
            return parts.count( bp ) > 0;
        }
        void set( const bodypart_str_id &bp ) {
            parts.insert( bp );
        }
        void reset( const bodypart_str_id &bp ) {
            parts.erase( bp );
        }
        bool any() const {
            return !parts.empty();
        }
        bool none() const {
            return parts.empty();
        }
        size_t count() const {
            return parts.size();
        }

        cata::flat_set<bodypart_str_id>::iterator begin() const {
            return parts.begin();
        }

        cata::flat_set<bodypart_str_id>::iterator end() const {
            return parts.end();
        }

        void clear() {
            parts.clear();
        }

        void serialize( JsonOut &s ) const;
        void deserialize( const JsonValue &s );
};

#endif // CATA_SRC_BODY_PART_SET_H
