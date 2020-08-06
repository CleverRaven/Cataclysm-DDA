#pragma once
#ifndef CATA_SRC_MAGIC_TER_FURN_TRANSFORM_H
#define CATA_SRC_MAGIC_TER_FURN_TRANSFORM_H

#include <map>
#include <vector>

#include "optional.h"
#include "type_id.h"
#include "weighted_list.h"

class Creature;
class JsonObject;
struct tripoint;

// this is a small class that contains the "results" of a terrain transform.
// T can be either ter_str_id or furn_str_id
template<class T>
class ter_furn_data
{
    private:
        weighted_int_list<T> list;
        std::string message;
        bool message_good;
    public:
        ter_furn_data() = default;
        ter_furn_data( const weighted_int_list<T> &list, const std::string &message,
                       const bool message_good ) :
            list( list ), message( message ), message_good( message_good ) {}

        bool has_msg() const;
        void add_msg( const Creature &critter ) const;
        cata::optional<T> pick() const;
        void load( const JsonObject &jo );
};

class ter_furn_transform
{

    private:

        std::string fail_message;

        std::map<ter_str_id, ter_furn_data<ter_str_id>> ter_transform;
        std::map<std::string, ter_furn_data<ter_str_id>> ter_flag_transform;

        std::map<furn_str_id, ter_furn_data<furn_str_id>> furn_transform;
        std::map<std::string, ter_furn_data<furn_str_id>> furn_flag_transform;

        cata::optional<ter_str_id> next_ter( const ter_str_id &ter ) const;
        cata::optional<ter_str_id> next_ter( const std::string &flag ) const;
        cata::optional<furn_str_id> next_furn( const furn_str_id &furn ) const;
        cata::optional<furn_str_id> next_furn( const std::string &flag ) const;

        template<class T, class K>
        cata::optional<ter_furn_data<T>> find_transform( const std::map<K, ter_furn_data<T>> &list,
                                      const K &key ) const;

        template <class T, class K>
        cata::optional<T> next( const std::map<K, ter_furn_data<T>> &list, const K &key ) const;

        // return value is success of message found
        template <class T, class K>
        bool add_message( const std::map<K, ter_furn_data<T>> &list, const K &key, const Creature &critter,
                          const tripoint &location ) const;

    public:

        ter_furn_transform_id id;
        bool was_loaded = false;

        void add_all_messages( const Creature &critter, const tripoint &location ) const;
        void add_all_messages( const map &m, const Creature &critter, const tripoint &location ) const;

        void transform( const tripoint &location ) const;
        void transform( map &m, const tripoint &location ) const;

        static void load_transform( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string & );

        static const std::vector<ter_furn_transform> &get_all();
        static void reset_all();
        bool is_valid() const;
};

#endif // CATA_SRC_MAGIC_TER_FURN_TRANSFORM_H
