#pragma once
#ifndef CATA_SRC_MAGIC_TER_FURN_TRANSFORM_H
#define CATA_SRC_MAGIC_TER_FURN_TRANSFORM_H

#include <map>
#include <optional>
#include <vector>

#include "coordinates.h"
#include "type_id.h"
#include "weighted_list.h"

class Creature;
class JsonObject;
struct tripoint;

// this is a small class that contains the "results" of a terrain transform.
// T can be either ter_str_id, furn_str_id, trap_str_id, or field_type_id
template<class T>
class ter_furn_data
{
    public:
        weighted_int_list<T> list;
        translation message;
        bool message_good = false;
        ter_furn_data() = default;
        ter_furn_data( const weighted_int_list<T> &list, const translation &message,
                       const bool message_good ) :
            list( list ), message( message ), message_good( message_good ) {}

        bool has_msg() const;
        std::optional<T> pick() const;
        void load( const JsonObject &jo );
};

class ter_furn_transform
{
    private:
        std::map<ter_str_id, ter_furn_data<ter_str_id>> ter_transform;
        std::map<std::string, ter_furn_data<ter_str_id>> ter_flag_transform;

        std::map<furn_str_id, ter_furn_data<furn_str_id>> furn_transform;
        std::map<std::string, ter_furn_data<furn_str_id>> furn_flag_transform;

        std::map<trap_str_id, ter_furn_data<trap_str_id>> trap_transform;
        std::map<std::string, ter_furn_data<trap_str_id>> trap_flag_transform;

        std::map<field_type_id, ter_furn_data<field_type_id>> field_transform;

        std::optional<std::pair<ter_str_id, std::pair<translation, bool>>> next_ter(
            const ter_str_id &ter ) const;
        std::optional<std::pair<ter_str_id, std::pair<translation, bool>>> next_ter(
            const std::string &flag ) const;
        std::optional<std::pair<furn_str_id, std::pair<translation, bool>>> next_furn(
            const furn_str_id &furn ) const;
        std::optional<std::pair<furn_str_id, std::pair<translation, bool>>> next_furn(
            const std::string &flag ) const;
        std::optional<std::pair<trap_str_id, std::pair<translation, bool>>> next_trap(
            const trap_str_id &trap ) const;
        std::optional<std::pair<trap_str_id, std::pair<translation, bool>>> next_trap(
            const std::string &flag ) const;
        std::optional<std::pair<field_type_id, std::pair<translation, bool>>> next_field(
            const field_type_id &field )
        const;

        template<class T, class K>
        std::optional<ter_furn_data<T>> find_transform( const std::map<K, ter_furn_data<T>> &list,
                                     const K &key ) const;

        template <class T, class K>
        std::optional<std::pair<T, std::pair<translation, bool>>> next( const std::map<K, ter_furn_data<T>>
                &list,
                const K &key ) const;

    public:

        ter_furn_transform_id id;
        std::vector<std::pair<ter_furn_transform_id, mod_id>> src;
        bool was_loaded = false;

        void transform( map &m, const tripoint_bub_ms &location ) const;

        static void reset();
        static void load_transform( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, std::string_view );

        static const std::vector<ter_furn_transform> &get_all();
        static void reset_all();
        bool is_valid() const;
};

#endif // CATA_SRC_MAGIC_TER_FURN_TRANSFORM_H
