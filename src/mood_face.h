#pragma once
#ifndef CATA_SRC_MOOD_FACE_H
#define CATA_SRC_MOOD_FACE_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "type_id.h"

class JsonObject;
class mood_face_value;
template<typename T>
class generic_factory;

class mood_face
{
    public:
        static void load_mood_faces( const JsonObject &jo, const std::string &src );
        static void reset();

        void load( const JsonObject &jo, std::string_view src );

        static const std::vector<mood_face> &get_all();

        const mood_face_id &getId() const {
            return id;
        }

        const std::vector<mood_face_value> &values() const {
            return values_;
        }

    private:
        friend class generic_factory<mood_face>;
        friend struct mod_tracker;

        mood_face_id id;
        std::vector<std::pair<mood_face_id, mod_id>> src;
        bool was_loaded = false;

        // Always sorted with highest value first
        std::vector<mood_face_value> values_;
};

class mood_face_value
{
    public:
        bool was_loaded = false;
        void load( const JsonObject &jo );
        void deserialize( const JsonObject &jo );

        int value() const {
            return value_;
        }

        const std::string &face() const {
            return face_;
        }

    private:
        int value_ = 0;
        std::string face_;
};

#endif // CATA_SRC_MOOD_FACE_H
