#include "event_statistics.h"

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "cata_assert.h"
#include "cata_variant.h"
#include "debug.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_field_transformations.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "hash_utils.h"
#include "output.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "translations.h"

// event_transformation and event_statistic are both objects defined in json
// and managed via generic_factory.
// They both use the pimpl idiom and dynamic polymorphism to support their
// implementation.
// Their values can be calculated directly, or watched for updates.
// To register something that watches for updates, call
// stats_tracker::add_watcher.
//
// The way watching works is as follows:
// - Each event_transformation and event_statistic has a corresponding state
//   object, which can be stored in the stats_tracker to hold the current
//   state of that transformation or statistic.
// - Also, the stats_tracker stores all the watchers; any object watching any
//   event type or the value of any transformation or statistic.
// - Each stat object will also register itself as a watcher for any event
//   type, transformation, or statistic it depends on.
// - When an event occurs, watchers of that event_type will be informed.
// - When stats are thus informed, they update their value if appropriate, and
//   inform the stats_tracker of the new value.
// - The stats_tracker then notifies watchers again in turn.
//
// In this way, the updates cascade to all relevant objects and ultimately the
// end-users (also in the form of watchers) are informed.  For example, an
// achievement might watch in this way so that it can trigger itself when its
// conditions become true.

namespace
{

generic_factory<event_transformation> event_transformation_factory( "event_transformation" );
generic_factory<event_statistic> event_statistic_factory( "event_statistic" );
generic_factory<score> score_factory( "score" );

} // namespace

template<>
const event_transformation &string_id<event_transformation>::obj() const
{
    return event_transformation_factory.obj( *this );
}

template<>
bool string_id<event_transformation>::is_valid() const
{
    return event_transformation_factory.is_valid( *this );
}

void event_transformation::load_transformation( const JsonObject &jo, const std::string &src )
{
    event_transformation_factory.load( jo, src );
}

void event_transformation::check_consistency()
{
    event_transformation_factory.check();
}

void event_transformation::reset()
{
    event_transformation_factory.reset();
}

template<>
const event_statistic &string_id<event_statistic>::obj() const
{
    return event_statistic_factory.obj( *this );
}

template<>
bool string_id<event_statistic>::is_valid() const
{
    return event_statistic_factory.is_valid( *this );
}

void event_statistic::load_statistic( const JsonObject &jo, const std::string &src )
{
    event_statistic_factory.load( jo, src );
}

void event_statistic::check_consistency()
{
    event_statistic_factory.check();
}

void event_statistic::reset()
{
    event_statistic_factory.reset();
}

template<>
const score &string_id<score>::obj() const
{
    return score_factory.obj( *this );
}

template<>
bool string_id<score>::is_valid() const
{
    return score_factory.is_valid( *this );
}

void score::load_score( const JsonObject &jo, const std::string &src )
{
    score_factory.load( jo, src );
}

void score::check_consistency()
{
    score_factory.check();
}

const std::vector<score> &score::get_all()
{
    return score_factory.get_all();
}

void score::reset()
{
    score_factory.reset();
}

class event_transformation::impl
{
    public:
        virtual ~impl() = default;
        virtual event_multiset initialize( stats_tracker & ) const = 0;
        virtual std::unique_ptr<stats_tracker_state> watch( stats_tracker & ) const = 0;
        virtual void check( const std::string &/*name*/ ) const {}
        virtual cata::event::fields_type fields() const = 0;
        virtual monotonically monotonicity() const = 0;
        virtual std::unique_ptr<impl> clone() const = 0;
};

class event_statistic::impl
{
    public:
        virtual ~impl() = default;
        virtual cata_variant value( stats_tracker & ) const = 0;
        virtual std::unique_ptr<stats_tracker_state> watch( stats_tracker & ) const = 0;
        virtual void check( const std::string &/*name*/ ) const {}
        virtual cata_variant_type type() const = 0;
        virtual monotonically monotonicity() const = 0;
        virtual std::unique_ptr<impl> clone() const = 0;
};

struct value_constraint {
    enum comparator { lt, lteq, gteq, gt };
    std::vector<cata_variant> equals_any_;
    std::optional<string_id<event_statistic>> equals_statistic_;
    std::optional<std::pair<comparator, cata_variant>> val_comp_;

    void deserialize( const JsonObject &jo ) {
        cata_variant single_val;
        const auto check_comp_int = [&]( const std::string & comp ) -> bool {
            bool ret = single_val.type() == cata_variant_type::int_;
            if( !ret ) {
                jo.throw_error( string_format( "The value constraint \"%s\" may only be used with int type!",
                                               comp ) );
            }
            return ret;
        };
        if( jo.read( "equals", single_val, false ) ) {
            equals_any_ = { single_val };
        } else if( jo.read( "lt", single_val, false ) && check_comp_int( "lt" ) ) {
            val_comp_ = std::pair<comparator, cata_variant>( comparator::lt, single_val );
        } else if( jo.read( "lteq", single_val, false ) && check_comp_int( "lteq" ) ) {
            val_comp_ = std::pair<comparator, cata_variant>( comparator::lteq, single_val );
        } else if( jo.read( "gteq", single_val, false ) && check_comp_int( "gteq" ) ) {
            val_comp_ = std::pair<comparator, cata_variant>( comparator::gteq, single_val );
        } else if( jo.read( "gt", single_val, false ) && check_comp_int( "gt" ) ) {
            val_comp_ = std::pair<comparator, cata_variant>( comparator::gt, single_val );
        }

        std::pair<cata_variant_type, std::vector<std::string>> equals_any;
        if( jo.read( "equals_any", equals_any ) ) {
            equals_any_.reserve( equals_any.second.size() );
            for( std::string &s : equals_any.second ) {
                equals_any_.push_back(
                    cata_variant::from_string( equals_any.first, std::move( s ) ) );
            }
        }

        string_id<event_statistic> stat;
        if( jo.read( "equals_statistic", stat ) ) {
            equals_statistic_ = stat;
        }

        if( equals_any_.empty() && !equals_statistic_ && !val_comp_ ) {
            jo.throw_error( "No valid value constraint found" );
        }
    }

    void check( const std::string &name, const cata_variant_type input_type ) const {
        if( equals_statistic_ ) {
            if( !equals_statistic_->is_valid() ) {
                debugmsg( "constraint for event_transformation %s refers to invalid statistic %s",
                          name, equals_statistic_->str() );
                return;
            }

            cata_variant_type stat_type = ( *equals_statistic_ )->type();
            if( stat_type != input_type ) {
                debugmsg( "constraint for event_transformation %s matches statistic %s of "
                          "different type.  Statistic has type %s but value compared with it has "
                          "type %s",
                          name, equals_statistic_->str(), io::enum_to_string( stat_type ),
                          io::enum_to_string( input_type ) );
            }
        }

        const auto check_one = [&]( const cata_variant & e ) -> void {
            if( input_type != e.type() ) {
                debugmsg( "constraint for event_transformation %s matches constant of type %s "
                          "but value compared with it has type %s",
                          name, io::enum_to_string( e.type() ),
                          io::enum_to_string( input_type ) );
            }
            if( !e.is_valid() ) {
                debugmsg( "constraint for event_transformation %s matches constant %s of type %s "
                          "but that is not a valid value of that type",
                          name, e.get_string(), io::enum_to_string( e.type() ) );
            }
        };

        for( const cata_variant &e : equals_any_ ) {
            check_one( e );
        }

        if( val_comp_ ) {
            check_one( ( *val_comp_ ).second );
        }
    }

    bool is_constant() const {
        return !equals_statistic_ ||
               ( *equals_statistic_ )->monotonicity() == monotonically::constant;
    }
};

struct new_field {
    event_field_transformation transformation;
    std::string input_field;

    void deserialize( const JsonObject &jo ) {
        if( jo.size() != 1 ) {
            jo.throw_error( "new field specifications must have exactly one entry" );
        }
        for( const JsonMember m : jo ) {
            const std::string &transformation_name = m.name();
            auto it = event_field_transformations.find( transformation_name );
            if( it == event_field_transformations.end() ) {
                m.throw_error(
                    string_format( "unknown event field transformation '%s'", transformation_name )
                );
            }
            transformation = it->second;
            input_field = m.get_string();
        }
    }

    void check( const std::string &context_name,
                const cata::event::fields_type &input_fields ) const {
        auto it = input_fields.find( input_field );
        if( it == input_fields.end() ) {
            debugmsg( "event_transformation %s specifies transformation on field %s but no such "
                      "field exists on the input", context_name, input_field );
            return;
        }
        if( transformation.argument_types.size() != 1 ) {
            debugmsg( "event_field_transformations of arity not 1 not supported" );
            return;
        }
        if( it->second != transformation.argument_types[0] ) {
            debugmsg( "event_transformation %s specifies transformation on field %s of incorrect "
                      "type.  Transformation expects %s; field was %s.", context_name, input_field,
                      io::enum_to_string( transformation.argument_types[0] ),
                      io::enum_to_string( it->second ) );
        }
    }

    std::vector<cata::event::data_type> transform( const cata::event::data_type &data,
            const std::string &new_field_name ) const {
        std::vector<cata::event::data_type> result;
        auto it = data.find( input_field );
        if( it == data.end() ) {
            // Field missing, probably due to stale data.  Reporting an error
            // would spam, so we rely on the fact that it should already have
            // been reported at startup time.
            return result;
        }
        if( !it->second.get_string().empty() && !it->second.is_valid() ) {
            // Field value is non-empty and invalid; event_transformation_functions are not
            // required to handle invalid values, so just skip it
            return result;
        }
        for( const cata_variant &v : transformation.function( it->second ) ) {
            result.push_back( data );
            result.back().emplace( new_field_name, v );
        }
        return result;
    }

    cata_variant_type type( const cata::event::fields_type &/*input*/ ) const {
        return transformation.return_type;
    }
};

// Interface to abstract the two possible sources of event_multisets:
// event_types and event_transformations
struct event_source {
    static std::unique_ptr<event_source> load( const JsonObject &jo );

    virtual ~event_source() = default;

    virtual event_multiset get( stats_tracker &stats ) const = 0;
    virtual std::string debug_description() const = 0;
    virtual bool is_game_start() const = 0;
    virtual void add_watcher( stats_tracker &stats, event_multiset_watcher *watcher ) const = 0;
    virtual cata::event::fields_type fields() const = 0;
    virtual monotonically monotonicity() const = 0;
    virtual std::unique_ptr<event_source> clone() const = 0;
};

struct event_type_event_source : event_source {
    explicit event_type_event_source( event_type t ) : type( t ) {}

    event_type type;

    event_multiset get( stats_tracker &stats ) const override {
        return stats.get_events( type );
    }

    std::string debug_description() const override {
        return "event type " + io::enum_to_string( type );
    }

    bool is_game_start() const override {
        return type == event_type::game_start;
    }

    void add_watcher( stats_tracker &stats, event_multiset_watcher *watcher ) const override {
        stats.add_watcher( type, watcher );
    }

    cata::event::fields_type fields() const override {
        return cata::event::get_fields( type );
    }

    monotonically monotonicity() const override {
        return monotonically::increasing;
    }

    std::unique_ptr<event_source> clone() const override {
        return std::make_unique<event_type_event_source>( *this );
    }
};

struct event_transformation_event_source : event_source {
    explicit event_transformation_event_source( const string_id<event_transformation> &t ) :
        transformation( t ) {}

    string_id<event_transformation> transformation;

    event_multiset get( stats_tracker &stats ) const override {
        return stats.get_events( transformation );
    }

    std::string debug_description() const override {
        return "event_transformation " + transformation->id.str();
    }

    bool is_game_start() const override {
        return false;
    }

    void add_watcher( stats_tracker &stats, event_multiset_watcher *watcher ) const override {
        stats.add_watcher( transformation, watcher );
    }

    cata::event::fields_type fields() const override {
        return transformation->fields();
    }

    monotonically monotonicity() const override {
        return transformation->monotonicity();
    }

    std::unique_ptr<event_source> clone() const override {
        return std::make_unique<event_transformation_event_source>( *this );
    }
};

std::unique_ptr<event_source> event_source::load( const JsonObject &jo )
{
    if( jo.has_member( "event_type" ) == jo.has_member( "event_transformation" ) ) {
        jo.throw_error( "Must specify exactly one of 'event_type' or 'event_transformation'" );
    }
    if( jo.has_member( "event_type" ) ) {
        event_type type = event_type::num_event_types;
        jo.read( "event_type", type, true );
        return std::make_unique<event_type_event_source>( type );
    } else {
        string_id<event_transformation> transformation;
        jo.read( "event_transformation", transformation, true );
        return std::make_unique<event_transformation_event_source>( transformation );
    }
}

struct event_transformation_impl : public event_transformation::impl {
    template<typename NewFields, typename Constraints, typename DropFields>
    event_transformation_impl( const string_id<event_transformation> &id,
                               std::unique_ptr<event_source> source,
                               const NewFields &new_fields, const Constraints &constraints,
                               const DropFields &drop_fields ) :
        id_( id ),
        source_( std::move( source ) ),
        new_fields_( new_fields.begin(), new_fields.end() ),
        constraints_( constraints.begin(), constraints.end() ),
        drop_fields_( drop_fields.begin(), drop_fields.end() )
    {}

    string_id<event_transformation> id_;
    cata::clone_ptr<event_source> source_;
    std::vector<std::pair<std::string, new_field>> new_fields_;
    std::vector<std::pair<std::string, value_constraint>> constraints_;
    std::vector<std::string> drop_fields_;

    struct updatable_value_constraint;

    struct state : stats_tracker_multiset_state, event_multiset_watcher {
        state( const event_transformation_impl *trans, stats_tracker &stats ) :
            transformation_( trans ),
            data_( trans->initialize( stats ) ),
            cached_value_constraints_( trans->make_cached_value_constraints( this, stats ) ) {
            cata_assert( cached_value_constraints_.size() == trans->constraints_.size() );
            trans->source_->add_watcher( stats, this );
        }

        void value_constraint_changed( stats_tracker &stats ) {
            data_ = transformation_->initialize( cached_value_constraints_, stats );
            stats.transformed_set_changed( transformation_->id_, data_ );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            EventVector transformed =
                transformation_->match_and_transform( e.data(), cached_value_constraints_ );
            for( cata::event::data_type &d : transformed ) {
                cata::event new_event( e.type(), e.time(), std::move( d ) );
                data_.add( new_event );
                stats.transformed_set_changed( transformation_->id_, new_event );
            }
        }

        void events_reset( const event_multiset &new_value, stats_tracker &stats ) override {
            data_ = transformation_->initialize(
                        new_value.counts(), cached_value_constraints_, stats );
            stats.transformed_set_changed( transformation_->id_, data_ );
        }

        const event_transformation_impl *transformation_;
        event_multiset data_;
        std::list<updatable_value_constraint> cached_value_constraints_;
    };

    // This is a helper class that wraps a value_constraint but caches the
    // value of any target statistic and is notified whenever that statistic
    // value changes.  This allows us to avoid recomputing the value of that
    // target statistic.
    struct updatable_value_constraint : stat_watcher {
        updatable_value_constraint(
            const std::pair<std::string, value_constraint> &p,
            state *st,
            stats_tracker &stats
        )
            : parent_constraint_( &p.second )
            , parent_state_( st )
            , field_name_( p.first ) {
            const value_constraint &val = p.second;
            if( val.equals_statistic_ ) {
                cached_value_ = stats.value_of( *val.equals_statistic_ );
                stats.add_watcher( *val.equals_statistic_, this );
            }
        }

        const std::string &field_name() const {
            return field_name_;
        }

        void new_value( const cata_variant &new_val, stats_tracker &stats ) override {
            cached_value_ = new_val;
            parent_state_->value_constraint_changed( stats );
        }

        bool permits( const cata_variant &v ) const {
            const value_constraint &p = *parent_constraint_;
            if( std::find( p.equals_any_.begin(), p.equals_any_.end(), v ) != p.equals_any_.end() ) {
                return true;
            }
            // NOLINTNEXTLINE(readability-simplify-boolean-expr)
            if( cached_value_ ) {
                return *cached_value_ == v;
            }
            if( p.val_comp_ ) {
                const int v_int = v.get<int>();
                const int c_int = p.val_comp_->second.get<int>();
                using comparator = value_constraint::comparator;
                switch( p.val_comp_->first ) {
                    case comparator::lt:
                        return v_int < c_int;
                    case comparator::lteq:
                        return v_int <= c_int;
                    case comparator::gteq:
                        return v_int >= c_int;
                    case comparator::gt:
                        return v_int > c_int;
                }
            }
            return false;
        }

        std::optional<cata_variant> cached_value_;
        const value_constraint *parent_constraint_;
        state *parent_state_;
        std::string field_name_;
    };

    using EventVector = std::vector<cata::event::data_type>;

    EventVector match_and_transform(
        const cata::event::data_type &input_data,
        const std::list<updatable_value_constraint> &cached_value_constraints ) const {
        cata_assert( cached_value_constraints.size() == constraints_.size() );

        EventVector result = { input_data };
        for( const std::pair<std::string, new_field> &p : new_fields_ ) {
            EventVector before_this_pass = std::move( result );
            result.clear();
            for( const cata::event::data_type &data : before_this_pass ) {
                EventVector from_this_event = p.second.transform( data, p.first );
                result.insert( result.end(), from_this_event.begin(), from_this_event.end() );
            }
        }

        auto violates_constraints = [&]( const cata::event::data_type & data ) {
            for( const updatable_value_constraint &constraint : cached_value_constraints ) {
                const std::string &field = constraint.field_name();
                const auto it = data.find( field );
                if( it == data.end() || !constraint.permits( it->second ) ) {
                    return true;
                }
            }
            return false;
        };

        result.erase( std::remove_if( result.begin(), result.end(), violates_constraints ),
                      result.end() );

        for( cata::event::data_type &data : result ) {
            for( const std::string &drop_field_name : drop_fields_ ) {
                data.erase( drop_field_name );
            }
        }

        return result;
    }

    std::list<updatable_value_constraint> make_cached_value_constraints(
        state *st, stats_tracker &stats ) const {
        // Need to use list here rather than vector because
        // updatable_value_constraint is not movable.
        std::list<updatable_value_constraint> result;
        for( const auto &p : constraints_ ) {
            result.emplace_back( p, st, stats );
        }
        return result;
    }

    event_multiset initialize(
        const event_multiset::summaries_type &input,
        const std::list<updatable_value_constraint> &cached_value_constraints,
        stats_tracker & ) const {
        event_multiset result;

        for( const std::pair<const cata::event::data_type, event_summary> &p : input ) {
            cata::event::data_type event_data = p.first;
            EventVector transformed = match_and_transform( event_data, cached_value_constraints );
            for( cata::event::data_type &d : transformed ) {
                result.add( { d, p.second } );
            }
        }
        return result;
    }

    event_multiset initialize(
        const std::list<updatable_value_constraint> &cached_value_constraints,
        stats_tracker &stats ) const {
        return initialize( source_->get( stats ).counts(), cached_value_constraints, stats );
    }

    event_multiset initialize( stats_tracker &stats ) const override {
        return initialize( make_cached_value_constraints( nullptr, stats ), stats );
    }

    void check( const std::string &name ) const override {
        const cata::event::fields_type input_fields = source_->fields();

        for( const std::pair<std::string, new_field> &p : new_fields_ ) {
            if( input_fields.count( p.first ) ) {
                debugmsg( "event_transformation %s tries to add field with name %s but a field "
                          "with that name already exists", name, p.first );
            }
            p.second.check( name, input_fields );
        }

        for( const std::pair<std::string, value_constraint> &p : constraints_ ) {
            auto it = input_fields.find( p.first );
            if( it == input_fields.end() ) {
                debugmsg( "event_transformation %s applies constraint to field %s, but no field "
                          "with that name exists in the input", name, p.first );
            } else {
                p.second.check( name, it->second );
            }
        }

        for( const std::string &drop_field_name : drop_fields_ ) {
            if( input_fields.find( drop_field_name ) == input_fields.end() ) {
                debugmsg( "event_transformation %s lists field %s to be dropped, but no field "
                          "with that name exists in the input", name, drop_field_name );
            }
        }
    }

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    cata::event::fields_type fields() const override {
        cata::event::fields_type result = source_->fields();

        for( const std::pair<std::string, new_field> &p : new_fields_ ) {
            result.emplace( p.first, p.second.type( result ) );
        }

        for( const std::string &drop_field_name : drop_fields_ ) {
            result.erase( drop_field_name );
        }

        return result;
    }

    monotonically monotonicity() const override {
        for( const std::pair<std::string, value_constraint> &constraint : constraints_ ) {
            if( !constraint.second.is_constant() ) {
                return monotonically::unknown;
            }
        }
        return monotonically::increasing;
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_transformation_impl>( *this );
    }
};

event_multiset event_transformation::value( stats_tracker &stats ) const
{
    return impl_->initialize( stats );
}

std::unique_ptr<stats_tracker_state> event_transformation::watch( stats_tracker &stats ) const
{
    return impl_->watch( stats );
}

void event_transformation::load( const JsonObject &jo, std::string_view )
{
    std::map<std::string, new_field> new_fields;
    optional( jo, was_loaded, "new_fields", new_fields );
    std::map<std::string, value_constraint> constraints;
    optional( jo, was_loaded, "value_constraints", constraints );
    std::vector<std::string> drop_fields;
    optional( jo, was_loaded, "drop_fields", drop_fields );

    if( new_fields.empty() && constraints.empty() && drop_fields.empty() ) {
        jo.throw_error( "Event transformation is a no-op." );
    }

    impl_ = std::make_unique<event_transformation_impl>(
                id, event_source::load( jo ), new_fields, constraints, drop_fields );
}

void event_transformation::check() const
{
    impl_->check( id.str() );
}

cata::event::fields_type event_transformation::fields() const
{
    return impl_->fields();
}

monotonically event_transformation::monotonicity() const
{
    return impl_->monotonicity();
}

struct event_statistic_count : event_statistic::impl {
    event_statistic_count( const string_id<event_statistic> &i, std::unique_ptr<event_source> s ) :
        id( i ),
        source( std::move( s ) )
    {}

    string_id<event_statistic> id;
    cata::clone_ptr<event_source> source;

    cata_variant value( stats_tracker &stats ) const override {
        int count = source->get( stats ).count();
        return cata_variant::make<cata_variant_type::int_>( count );
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_count *s, stats_tracker &stats ) :
            stat( s ),
            value( s->value( stats ) ),
            value_int( value.get<int>() ) {
            stat->source->add_watcher( stats, this );
        }

        void event_added( const cata::event &, stats_tracker &stats ) override {
            ++value_int;
            value = cata_variant( value_int );
            stats.stat_value_changed( stat->id, value );
        }

        void events_reset( const event_multiset &new_set, stats_tracker &stats ) override {
            value_int = new_set.count();
            value = cata_variant( value_int );
            stats.stat_value_changed( stat->id, value );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_count *stat;
        cata_variant value;
        int value_int;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    cata_variant_type type() const override {
        return cata_variant_type::int_;
    }

    monotonically monotonicity() const override {
        return monotonically::increasing;
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_count>( *this );
    }
};

// A bunch of the event_statistic derived classes below share in common that
// they operate on a single field of some event_source.  Use this intermediate
// derived class to contain the common code from those
template<bool IntField>
struct event_statistic_field_summary : event_statistic::impl {
    event_statistic_field_summary(
        const string_id<event_statistic> &i, std::unique_ptr<event_source> s,
        const std::string &f
    ) : id( i ), source( std::move( s ) ), field( f )
    {}

    string_id<event_statistic> id;
    cata::clone_ptr<event_source> source;
    std::string field;

    void check( const std::string &name ) const override {
        cata::event::fields_type event_fields = source->fields();
        auto it = event_fields.find( field );
        if( it == event_fields.end() ) {
            auto get_first = []( const std::pair<const std::string, cata_variant_type> &p ) {
                return p.first;
            };
            debugmsg( "event_statistic %s refers to field %s in event source %s, but that source "
                      "has no such field.  Its fields are %s.",
                      name, field, source->debug_description(),
                      enumerate_as_string( event_fields, get_first ) );
        } else if( IntField && it->second != cata_variant_type::int_ ) {
            debugmsg( "event_statistic %s refers to field %s in event source %s, and uses that "
                      "field as if it were an int, but in fact that field has type %s.",
                      name, field, source->debug_description(),
                      io::enum_to_string( it->second ) );
        }
    }

    cata_variant_type type() const override {
        if( IntField ) {
            return cata_variant_type::int_;
        } else {
            cata::event::fields_type source_fields = source->fields();
            auto it = source_fields.find( field );
            if( it == source_fields.end() ) {
                return cata_variant_type::void_;
            } else {
                return it->second;
            }
        }
    }
};

struct event_statistic_total : event_statistic_field_summary<true> {
    using event_statistic_field_summary::event_statistic_field_summary;

    cata_variant value( stats_tracker &stats ) const override {
        int total = source->get( stats ).total( field );
        return cata_variant::make<cata_variant_type::int_>( total );
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_total *s, stats_tracker &stats ) :
            stat( s ),
            value( s->value( stats ) ),
            value_int( value.get<int>() ) {
            stat->source->add_watcher( stats, this );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            value_int += e.get<int>( stat->field );
            value = cata_variant( value_int );
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        void events_reset( const event_multiset &new_set, stats_tracker &stats ) override {
            value_int = new_set.total( stat->field );
            value = cata_variant( value_int );
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_total *stat;
        cata_variant value;
        int value_int;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    monotonically monotonicity() const override {
        return source->monotonicity();
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_total>( *this );
    }
};

struct event_statistic_maximum : event_statistic_field_summary<true> {
    using event_statistic_field_summary::event_statistic_field_summary;

    cata_variant value( stats_tracker &stats ) const override {
        int maximum = source->get( stats ).maximum( field );
        return cata_variant::make<cata_variant_type::int_>( maximum );
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_maximum *s, stats_tracker &stats ) :
            stat( s ),
            value( s->value( stats ) ),
            value_int( value.get<int>() ) {
            stat->source->add_watcher( stats, this );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            const int new_value = std::max( e.get<int>( stat->field ), value_int );
            if( new_value != value_int ) {
                value_int = new_value;
                value = cata_variant( value_int );
                stats.stat_value_changed( stat->id, cata_variant( value ) );
            }
        }

        void events_reset( const event_multiset &new_set, stats_tracker &stats ) override {
            value_int = new_set.maximum( stat->field );
            value = cata_variant( value_int );
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_maximum *stat;
        cata_variant value;
        int value_int;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    monotonically monotonicity() const override {
        return source->monotonicity();
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_maximum>( *this );
    }
};

struct event_statistic_minimum : event_statistic_field_summary<true> {
    using event_statistic_field_summary::event_statistic_field_summary;

    cata_variant value( stats_tracker &stats ) const override {
        int minimum = source->get( stats ).minimum( field );
        return cata_variant::make<cata_variant_type::int_>( minimum );
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_minimum *s, stats_tracker &stats ) :
            stat( s ),
            value( s->value( stats ) ),
            value_int( value.get<int>() ) {
            stat->source->add_watcher( stats, this );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            const int new_value = std::min( e.get<int>( stat->field ), value_int );
            if( new_value != value_int ) {
                value_int = new_value;
                value = cata_variant( value_int );
                stats.stat_value_changed( stat->id, cata_variant( value ) );
            }
        }

        void events_reset( const event_multiset &new_set, stats_tracker &stats ) override {
            value_int = new_set.minimum( stat->field );
            value = cata_variant( value_int );
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_minimum *stat;
        cata_variant value;
        int value_int;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    monotonically monotonicity() const override {
        // If the source is increasing (adding more events)
        if( is_increasing( source->monotonicity() ) ) {
            // Then the minimum value will be decreasing
            return monotonically::decreasing;
        } else {
            return monotonically::unknown;
        }
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_minimum>( *this );
    }
};

struct event_statistic_unique_value : event_statistic_field_summary<false> {
    using event_statistic_field_summary::event_statistic_field_summary;

    cata_variant value( stats_tracker &stats ) const override {
        const event_multiset::summaries_type summaries = source->get( stats ).counts();
        if( summaries.size() != 1 ) {
            return cata_variant();
        }

        const cata::event::data_type &d = summaries.begin()->first;
        auto it = d.find( field );
        if( it == d.end() ) {
            return cata_variant();
        }
        return it->second;
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_unique_value *s, stats_tracker &stats ) :
            stat( s ) {
            init( stats );
            stat->source->add_watcher( stats, this );
        }

        void init( stats_tracker &stats ) {
            count = stat->source->get( stats ).count();
            value = stat->value( stats );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            ++count;
            if( count == 1 ) {
                value = e.get_variant_or_void( stat->field );
            } else if( count == 2 ) {
                value = cata_variant();
            } else {
                return;
            }
            stats.stat_value_changed( stat->id, value );
        }

        void events_reset( const event_multiset &, stats_tracker &stats ) override {
            init( stats );
            stats.stat_value_changed( stat->id, value );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_unique_value *stat;
        int count = 0;
        cata_variant value;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    monotonically monotonicity() const override {
        if( source->is_game_start() ) {
            return monotonically::constant;
        } else {
            return monotonically::unknown;
        }
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_unique_value>( *this );
    }
};

struct event_statistic_first_value : event_statistic_field_summary<false> {
    using event_statistic_field_summary::event_statistic_field_summary;

    cata_variant value( stats_tracker &stats ) const override {
        const event_multiset &events = source->get( stats );
        const std::optional<event_multiset::summaries_type::value_type> d = events.first();
        if( d ) {
            auto it = d->first.find( field );
            if( it == d->first.end() ) {
                return cata_variant();
            }
            return it->second;
        }
        return cata_variant();
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_first_value *s, stats_tracker &stats ) :
            stat( s ) {
            init( stats );
            stat->source->add_watcher( stats, this );
        }

        void init( stats_tracker &stats ) {
            count = stat->source->get( stats ).count();
            value = stat->value( stats );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            ++count;
            if( count == 1 ) {
                value = e.get_variant_or_void( stat->field );
                stats.stat_value_changed( stat->id, value );
            } else {
                return;
            }
        }

        void events_reset( const event_multiset &, stats_tracker &stats ) override {
            init( stats );
            stats.stat_value_changed( stat->id, value );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_first_value *stat;
        int count = 0;
        cata_variant value;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    monotonically monotonicity() const override {
        if( source->is_game_start() ) {
            return monotonically::constant;
        } else {
            return monotonically::unknown;
        }
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_first_value>( *this );
    }
};

struct event_statistic_last_value : event_statistic_field_summary<false> {
    using event_statistic_field_summary::event_statistic_field_summary;

    cata_variant value( stats_tracker &stats ) const override {
        const event_multiset &events = source->get( stats );
        const std::optional<event_multiset::summaries_type::value_type> d = events.last();
        if( d ) {
            auto it = d->first.find( field );
            if( it == d->first.end() ) {
                return cata_variant();
            }
            return it->second;
        }
        return cata_variant();
    }

    struct state : stats_tracker_value_state, event_multiset_watcher {
        state( const event_statistic_last_value *s, stats_tracker &stats ) :
            stat( s ) {
            init( stats );
            stat->source->add_watcher( stats, this );
        }

        void init( stats_tracker &stats ) {
            value = stat->value( stats );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            value = e.get_variant_or_void( stat->field );
            stats.stat_value_changed( stat->id, value );
        }

        void events_reset( const event_multiset &, stats_tracker &stats ) override {
            init( stats );
            stats.stat_value_changed( stat->id, value );
        }

        const cata_variant &get_value() const override {
            return value;
        }

        const event_statistic_last_value *stat;
        cata_variant value;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    monotonically monotonicity() const override {
        if( source->is_game_start() ) {
            return monotonically::constant;
        } else {
            return monotonically::unknown;
        }
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_last_value>( *this );
    }
};

cata_variant event_statistic::value( stats_tracker &stats ) const
{
    return impl_->value( stats );
}

std::unique_ptr<stats_tracker_state> event_statistic::watch( stats_tracker &stats ) const
{
    return impl_->watch( stats );
}

void event_statistic::load( const JsonObject &jo, std::string_view )
{
    std::string type;
    mandatory( jo, was_loaded, "stat_type", type );

    description_.make_plural();
    optional( jo, was_loaded, "description", description_ );

    if( type == "count" ) {
        impl_ = std::make_unique<event_statistic_count>( id, event_source::load( jo ) );
    } else if( type == "total" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_total>( id, event_source::load( jo ), field );
    } else if( type == "minimum" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_minimum>( id, event_source::load( jo ), field );
    } else if( type == "maximum" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_maximum>( id, event_source::load( jo ), field );
    } else if( type == "unique_value" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_unique_value>(
                    id, event_source::load( jo ), field );
    } else if( type == "first_value" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_first_value>(
                    id, event_source::load( jo ), field );
    } else if( type == "last_value" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_last_value>(
                    id, event_source::load( jo ), field );
    } else {
        jo.throw_error( "Invalid stat_type '" + type + "'" );
    }
}

void event_statistic::check() const
{
    impl_->check( id.str() );
}

cata_variant_type event_statistic::type() const
{
    return impl_->type();
}

monotonically event_statistic::monotonicity() const
{
    return impl_->monotonicity();
}

std::string score::description( stats_tracker &stats ) const
{
    cata_variant val = value( stats );
    std::string value_string = value( stats ).get_string();
    std::string desc;
    if( val.type() == cata_variant_type::int_ ) {
        desc = stat_->description().translated( val.get<int>() );
    } else {
        desc = stat_->description().translated();
    }
    if( description_.empty() ) {
        //~ Default format for scores.  %1$s is statistic description; %2$s is value.
        return string_format( _( "%2$s %1$s" ), desc, value_string );
    } else {
        return string_format( desc, value_string );
    }
}

cata_variant score::value( stats_tracker &stats ) const
{
    return stats.value_of( stat_ );
}

void score::load( const JsonObject &jo, std::string_view )
{
    optional( jo, was_loaded, "description", description_ );
    mandatory( jo, was_loaded, "statistic", stat_ );
}

void score::check() const
{
    if( !stat_.is_valid() ) {
        debugmsg( "score %s refers to invalid statistic %s", id.str(), stat_.str() );
    }
}
