#include "event_statistics.h"

#include "event.h"
#include "generic_factory.h"
#include "stats_tracker.h"

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
        virtual std::unique_ptr<impl> clone() const = 0;
};

class event_statistic::impl
{
    public:
        virtual ~impl() = default;
        virtual cata_variant value( stats_tracker & ) const = 0;
        virtual std::unique_ptr<stats_tracker_state> watch( stats_tracker & ) const = 0;
        virtual void check( const std::string &/*name*/ ) const {}
        virtual std::unique_ptr<impl> clone() const = 0;
};

struct value_constraint {
    cata::optional<cata_variant> equals_;
    cata::optional<std::string> equals_string_;
    cata::optional<string_id<event_statistic>> equals_statistic_;

    bool permits( const cata_variant &v, stats_tracker &stats ) const {
        if( equals_ && *equals_ != v ) {
            return false;
        }
        if( equals_string_ && *equals_string_ != v.get_string() ) {
            return false;
        }
        if( equals_statistic_ && stats.value_of( *equals_statistic_ ) != v ) {
            return false;
        }
        return true;
    }

    void deserialize( JsonIn &jsin ) {
        JsonObject jo = jsin.get_object();
        int equals_int;
        if( jo.read( "equals", equals_int, false ) ) {
            equals_ = cata_variant::make<cata_variant_type::int_>( equals_int );
        }

        std::string equals_string;
        if( jo.read( "equals", equals_string, false ) ) {
            equals_string_ = equals_string;
        }

        string_id<event_statistic> stat;
        if( jo.read( "equals_statistic", stat ) ) {
            equals_statistic_ = stat;
        }

        if( !equals_ && !equals_string_ && !equals_statistic_ ) {
            jo.throw_error( "No valid value constraint found" );
        }
    }

    void check( const std::string &name ) const {
        if( equals_statistic_ && !equals_statistic_->is_valid() ) {
            debugmsg( "constraint for event_transformation %s refers to invalid statistic %s",
                      name, equals_statistic_->str() );
        }
    }
};

struct event_transformation_match : public event_transformation::impl {
    template<typename Constriants>
    event_transformation_match( const string_id<event_transformation> &id, event_type type,
                                const Constriants &constriants ) :
        id_( id ),
        type_( type ),
        constraints_( constriants.begin(), constriants.end() )
    {}

    string_id<event_transformation> id_;
    event_type type_;
    std::vector<std::pair<std::string, value_constraint>> constraints_;

    bool matches( const cata::event::data_type &data, stats_tracker &stats ) const {
        for( const std::pair<std::string, value_constraint> &p : constraints_ ) {
            const std::string &field = p.first;
            const value_constraint &constraint = p.second;
            const auto it = data.find( field );
            if( it == data.end() || !constraint.permits( it->second, stats ) ) {
                return false;
            }
        }
        return true;
    }

    event_multiset initialize( const event_multiset::counts_type &input,
                               stats_tracker &stats ) const {
        event_multiset result( type_ );

        for( const std::pair<const cata::event::data_type, int> &p : input ) {
            if( matches( p.first, stats ) ) {
                result.add( p );
            }
        }
        return result;
    }

    event_multiset initialize( stats_tracker &stats ) const override {
        return initialize( stats.get_events( type_ ).counts(), stats );
    }

    struct state : stats_tracker_state, event_multiset_watcher, stat_watcher {
        state( const event_transformation_match *trans, stats_tracker &stats ) :
            transformation_( trans ),
            data_( trans->initialize( stats ) ) {
            stats.add_watcher( trans->type_, this );
            for( const auto &p : trans->constraints_ ) {
                if( p.second.equals_statistic_ ) {
                    stats.add_watcher( *p.second.equals_statistic_, this );
                }
            }
        }

        void new_value( const cata_variant &, stats_tracker &stats ) override {
            data_ = transformation_->initialize( stats );
            stats.transformed_set_changed( transformation_->id_, data_ );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            if( transformation_->matches( e.data(), stats ) ) {
                data_.add( e );
                stats.transformed_set_changed( transformation_->id_, e );
            }
        }

        void events_reset( const event_multiset &new_value, stats_tracker &stats ) override {
            data_ = transformation_->initialize( new_value.counts(), stats );
            stats.transformed_set_changed( transformation_->id_, data_ );
        }

        const event_transformation_match *transformation_;
        event_multiset data_;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    void check( const std::string &name ) const override {
        for( const std::pair<std::string, value_constraint> &p : constraints_ ) {
            p.second.check( name );
        }
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_transformation_match>( *this );
    }
};

event_multiset event_transformation::initialize( stats_tracker &stats ) const
{
    return impl_->initialize( stats );
}

std::unique_ptr<stats_tracker_state> event_transformation::watch( stats_tracker &stats ) const
{
    return impl_->watch( stats );
}

void event_transformation::load( const JsonObject &jo, const std::string & )
{
    event_type type = event_type::num_event_types;
    mandatory( jo, was_loaded, "event_type", type );
    std::map<std::string, value_constraint> constraints;
    mandatory( jo, was_loaded, "value_constraints", constraints );

    impl_ = std::make_unique<event_transformation_match>( id, type, constraints );
}

void event_transformation::check() const
{
    impl_->check( id.str() );
}

// Helper struct to abstract the two possible sources of event_multisets:
// event_types and event_transformations
struct event_source {
    event_source( event_type t ) : type( t ) {}
    event_source( const string_id<event_transformation> &t ) : transformation( t ) {}

    event_type type = event_type::num_event_types;
    string_id<event_transformation> transformation;

    event_multiset get( stats_tracker &stats ) const {
        if( transformation.is_empty() ) {
            return stats.get_events( type );
        } else {
            return stats.get_events( transformation );
        }
    }

    void add_watcher( stats_tracker &stats, event_multiset_watcher *watcher ) const {
        if( transformation.is_empty() ) {
            stats.add_watcher( type, watcher );
        } else {
            stats.add_watcher( transformation, watcher );
        }
    }
};

struct event_statistic_count : event_statistic::impl {
    event_statistic_count( const string_id<event_statistic> &i, const event_source &s ) :
        id( i ),
        source( s )
    {}

    string_id<event_statistic> id;
    event_source source;

    cata_variant value( stats_tracker &stats ) const override {
        int count = source.get( stats ).count();
        return cata_variant::make<cata_variant_type::int_>( count );
    }

    struct state : stats_tracker_state, event_multiset_watcher {
        state( const event_statistic_count *s, stats_tracker &stats ) :
            stat( s ),
            value( s->value( stats ).get<int>() ) {
            stat->source.add_watcher( stats, this );
        }

        void event_added( const cata::event &, stats_tracker &stats ) override {
            ++value;
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        void events_reset( const event_multiset &new_set, stats_tracker &stats ) override {
            value = new_set.count();
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        const event_statistic_count *stat;
        int value;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_count>( *this );
    }
};

struct event_statistic_total : event_statistic::impl {
    event_statistic_total( const string_id<event_statistic> &i, const event_source &s,
                           const std::string &f ) :
        id( i ), source( s ), field( f )
    {}

    string_id<event_statistic> id;
    event_source source;
    std::string field;

    cata_variant value( stats_tracker &stats ) const override {
        int total = source.get( stats ).total( field );
        return cata_variant::make<cata_variant_type::int_>( total );
    }

    struct state : stats_tracker_state, event_multiset_watcher {
        state( const event_statistic_total *s, stats_tracker &stats ) :
            stat( s ),
            value( s->value( stats ).get<int>() ) {
            stat->source.add_watcher( stats, this );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            value += e.get<int>( stat->field );
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        void events_reset( const event_multiset &new_set, stats_tracker &stats ) override {
            value = new_set.total( stat->field );
            stats.stat_value_changed( stat->id, cata_variant( value ) );
        }

        const event_statistic_total *stat;
        int value;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_total>( *this );
    }
};

struct event_statistic_unique_value : event_statistic::impl {
    event_statistic_unique_value( const string_id<event_statistic> &id, event_type type,
                                  const std::string &field ) :
        id_( id ), type_( type ), field_( field )
    {}

    string_id<event_statistic> id_;
    event_type type_;
    std::string field_;

    cata_variant value( stats_tracker &stats ) const override {
        const event_multiset::counts_type &counts = stats.get_events( type_ ).counts();
        if( counts.size() != 1 ) {
            return cata_variant();
        }

        const cata::event::data_type &d = counts.begin()->first;
        auto it = d.find( field_ );
        if( it == d.end() ) {
            return cata_variant();
        }
        return it->second;
    }

    struct state : stats_tracker_state, event_multiset_watcher {
        state( const event_statistic_unique_value *s, stats_tracker &stats ) :
            stat( s ),
            count( stats.get_events( s->type_ ).count() ),
            value( s->value( stats ) ) {
            stats.add_watcher( stat->type_, this );
        }

        void event_added( const cata::event &e, stats_tracker &stats ) override {
            ++count;
            if( count == 1 ) {
                value = e.get_variant( stat->field_ );
            } else if( count == 2 ) {
                value = cata_variant();
            } else {
                return;
            }
            stats.stat_value_changed( stat->id_, value );
        }

        void events_reset( const event_multiset &, stats_tracker &stats ) override {
            *this = state( stat, stats );
            stats.stat_value_changed( stat->id_, value );
        }

        const event_statistic_unique_value *stat;
        int count;
        cata_variant value;
    };

    std::unique_ptr<stats_tracker_state> watch( stats_tracker &stats ) const override {
        return std::make_unique<state>( this, stats );
    }

    void check( const std::string &name ) const override {
        std::map<std::string, cata_variant_type> event_fields = cata::event::get_fields( type_ );
        auto it = event_fields.find( field_ );
        if( it == event_fields.end() ) {
            debugmsg( "event_statistic %s refers to field %s in event_type %s, but that type has "
                      "no such field", name, field_, io::enum_to_string( type_ ) );
        }
    }

    std::unique_ptr<impl> clone() const override {
        return std::make_unique<event_statistic_unique_value>( *this );
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

void event_statistic::load( const JsonObject &jo, const std::string & )
{
    std::string type;
    mandatory( jo, was_loaded, "stat_type", type );

    auto get_event_source = [&]() {
        event_type event_t = event_type::num_event_types;
        optional( jo, was_loaded, "event_type", event_t, event_type::num_event_types );
        string_id<event_transformation> event_transformation;
        optional( jo, was_loaded, "event_transformation", event_transformation );

        if( ( event_t == event_type::num_event_types ) == event_transformation.is_empty() ) {
            jo.throw_error( "Must specify exactly one of 'event_type' or 'event_transformation' in "
                            "event_statistic of type 'count'" );
        }

        if( event_transformation.is_empty() ) {
            return event_source( event_t );
        } else {
            return event_source( event_transformation );
        }
    };

    if( type == "count" ) {
        impl_ = std::make_unique<event_statistic_count>( id, get_event_source() );
    } else if( type == "total" ) {
        std::string field;
        mandatory( jo, was_loaded, "field", field );
        impl_ = std::make_unique<event_statistic_total>( id, get_event_source(), field );
    } else if( type == "unique_value" ) {
        event_type event_t = event_type::num_event_types;
        mandatory( jo, was_loaded, "event_type", event_t );
        std::string field;
        mandatory( jo, was_loaded, "field", field );

        impl_ = std::make_unique<event_statistic_unique_value>( id, event_t, field );
    } else {
        jo.throw_error( "Invalid stat_type '" + type + "'" );
    }
}

void event_statistic::check() const
{
    impl_->check( id.str() );
}

std::string score::description( stats_tracker &stats ) const
{
    return string_format( description_.translated(), value( stats ).get_string() );
}

cata_variant score::value( stats_tracker &stats ) const
{
    return stats.value_of( stat_ );
}

void score::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "description", description_ );
    mandatory( jo, was_loaded, "statistic", stat_ );
}

void score::check() const
{
    if( !stat_.is_valid() ) {
        debugmsg( "score %s refers to invalid statistic %s", id.str(), stat_.str() );
    }
}
