#pragma once
#ifndef EVENT_H
#define EVENT_H

#include <map>
#include <utility>

#include "calendar.h"
#include "cata_variant.h"
#include "type_id.h"

using itype_id = std::string;

// An event is something to be passed via the event_bus to subscribers
// interested in being notified about events.
//
// Each event is of a specific type, taken from the event_type enum.

enum class event_type {
    character_kills_character,
    character_kills_monster,
    num_event_types // last
};

class event;

namespace event_detail
{

// An event has various data stored in a map keyed by strings.  The specific
// keys and corresponding data types are specified in a specialization of
// event_spec.

template<event_type Type>
struct event_spec;

template<>
struct event_spec<event_type::character_kills_monster> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 2> fields = {{
            { "killer_id", cata_variant_type::character_id },
            { "victim_type", cata_variant_type::mtype_id },
        }
    };
};

template<>
struct event_spec<event_type::character_kills_character> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "killer_id", cata_variant_type::character_id },
            { "victim_id", cata_variant_type::character_id },
            { "victim_name", cata_variant_type::string },
        }
    };
};

template<event_type Type, typename IndexSequence>
struct make_event_helper;

} // namespace event_detail

class event
{
    public:
        event( event_type type, time_point time, std::map<std::string, cata_variant> &&data )
            : type_( type )
            , time_( time )
            , data_( std::move( data ) )
        {}

        // Call this to construct an event in a type-safe manner.  It will
        // verify that the types you pass match the expected types for the
        // event_type you pass as a template parameter.
        template<event_type Type, typename... Args>
        static event make( time_point time, Args &&... args ) {
            using Spec = event_detail::event_spec<Type>;
            // Using is_empty mostly just to verify that the type is defined at
            // all, but it so happens that it ought to be empty too.
            static_assert( std::is_empty<Spec>::value,
                           "spec for this event type must be defined and empty" );
            static_assert( sizeof...( Args ) == Spec::fields.size(),
                           "wrong number of arguments for event type" );

            return event_detail::make_event_helper <
                   Type, std::make_index_sequence<sizeof...( Args )>
                   > ()( time, std::forward<Args>( args )... );
        }

        event_type type() const {
            return type_;
        }
        time_point time() const {
            return time_;
        }

        cata_variant get_variant( const std::string &key ) const {
            auto it = data_.find( key );
            if( it == data_.end() ) {
                debugmsg( "No such key %s in event of type %d", key, type_ );
                abort();
            }
            return it->second;
        }

        template<cata_variant_type Type>
        auto get( const std::string &key ) const {
            return get_variant( key ).get<Type>();
        }

        template<typename T>
        auto get( const std::string &key ) const {
            return get_variant( key ).get<T>();
        }
    private:
        event_type type_;
        time_point time_;
        std::map<std::string, cata_variant> data_;
};

namespace event_detail
{

template<event_type Type, size_t... I>
struct make_event_helper<Type, std::index_sequence<I...>> {
    using Spec = event_spec<Type>;

    template<typename... Args>
    event operator()( time_point time, Args &&... args ) {
        return event(
                   Type,
                   time,
        std::map<std::string, cata_variant> { {
                Spec::fields[I].first,
                cata_variant::make<Spec::fields[I].second>( args )
            } ...
        } );
    }
};

} // namespace event_detail

#endif // EVENT_H
