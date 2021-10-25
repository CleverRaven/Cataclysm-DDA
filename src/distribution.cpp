#include "distribution.h"

#include <random>

#include "json.h"
#include "rng.h"

struct int_distribution_impl {
    virtual ~int_distribution_impl() = default;
    virtual int minimum() const = 0;
    virtual int sample() = 0;
    virtual std::string description() const = 0;
};

struct fixed_distribution : int_distribution_impl {
    int value;

    explicit fixed_distribution( int v )
        : value( v )
    {}

    int minimum() const override {
        return value;
    }

    int sample() override {
        return value;
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Fixed(%d)", value );
    }
};

struct poisson_distribution : int_distribution_impl {
    std::poisson_distribution<int> dist;

    explicit poisson_distribution( double mean )
        : dist( mean )
    {}

    int minimum() const override {
        return 0;
    }

    int sample() override {
        return dist( rng_get_engine() );
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Poisson(%.0f)", dist.mean() );
    }
};

int_distribution::int_distribution()
    : impl_( make_shared_fast<fixed_distribution>( 0 ) )
{}

int_distribution::int_distribution( int v )
    : impl_( make_shared_fast<fixed_distribution>( v ) )
{}

int int_distribution::minimum() const
{
    return impl_->minimum();
}

int int_distribution::sample() const
{
    return impl_->sample();
}

std::string int_distribution::description() const
{
    return impl_->description();
}

void int_distribution::deserialize( const JsonValue &jin )
{
    if( jin.test_int() ) {
        int v = jin.get_int();
        impl_ = make_shared_fast<fixed_distribution>( v );
    } else if( jin.test_object() ) {
        JsonObject jo = jin.get_object();
        if( jo.has_member( "poisson" ) ) {
            double mean = jo.get_float( "poisson", true );
            impl_ = make_shared_fast<poisson_distribution>( mean );
        } else {
            jo.throw_error( R"(Expected "poisson" member)" );
        }
    } else {
        jin.throw_error( "expected number or object" );
    }
}
