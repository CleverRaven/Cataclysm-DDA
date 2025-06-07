#include "distribution.h"

#include <algorithm>
#include <climits>
#include <memory>
#include <random>

#include "flexbuffer_json.h"
#include "memory_fast.h"
#include "rng.h"
#include "string_formatter.h"

struct int_distribution_impl {
    virtual ~int_distribution_impl() = default;
    virtual int minimum() const = 0;
    virtual int sample() = 0;
    virtual std::string description() const = 0;
    int blo = 0;
    int bhi = INT_MAX;
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

struct uniform_distribution : int_distribution_impl {
    std::uniform_int_distribution<int> dist;

    explicit uniform_distribution( int a, int b )
        : dist( a, b )
    {}

    int minimum() const override {
        return 0;
    }

    int sample() override {
        return dist( rng_get_engine() );
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Uniform(%d,%d)", dist.a(), dist.b() );
    }
};

struct binomial_distribution : int_distribution_impl {
    std::binomial_distribution<int> dist;
    explicit binomial_distribution( int lo, int hi, int t, double p )
        : dist( t, p ) {
        blo = lo;
        bhi = hi;
    }

    int minimum() const override {
        return 0;
    }

    int sample() override {
        int distvalue = dist( rng_get_engine() );
        int rvalue = std::min( distvalue, bhi );
        rvalue = std::max( rvalue, blo );
        return rvalue;
    }

    std::string description() const override {
        // NOLINTNEXTLINE(cata-translate-string-literal)
        return string_format( "Binomial(%d,%.0f)", dist.t(), dist.p() );
    }
};

struct poisson_distribution : int_distribution_impl {
    std::poisson_distribution<int> dist;

    explicit poisson_distribution( int lo, int hi, double mean )
        : dist( mean ) {
        blo = lo;
        bhi = hi;
    }

    int minimum() const override {
        return 0;
    }

    int sample() override {
        int rvalue = std::min( dist( rng_get_engine() ), bhi );
        rvalue = std::max( rvalue, blo );
        return rvalue;
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
    int lo = 0;
    int hi = INT_MAX;
    if( jin.test_int() ) {
        int v = jin.get_int();
        impl_ = make_shared_fast<fixed_distribution>( v );
    } else if( jin.test_array() ) {
        JsonArray jarr = jin.get_array();
        if( jarr.size() != 2 ) {
            jarr.throw_error( "uniform array has wrong number of elements, should be [ lo, hi ]" );
        }
        lo = jarr.get_int( 0 );
        hi = jarr.get_int( 1 );
        if( lo >= hi ) {
            jarr.throw_error( "uniform array should be in order [ lo, hi ]" );
        }
        if( lo < 0 || hi < 0 ) {
            jarr.throw_error( "both elements must be 0 or greater" );
        }
        impl_ = make_shared_fast<uniform_distribution>( lo, hi );
    } else if( jin.test_object() ) {
        JsonObject jo = jin.get_object();
        if( jo.has_member( "bounds" ) ) {
            if( jo.get_array( "bounds" ).size() != 2 ) {
                jo.throw_error( "bounds array has wrong number of elements, should be [ lo, hi ]" );
            }
            lo = jo.get_array( "bounds" ).get_int( 0 );
            hi = jo.get_array( "bounds" ).get_int( 1 );
            if( hi < 0 ) {
                hi = INT_MAX;
            }
            if( lo < 0 ) {
                lo = 0;
            }
            if( lo >= hi ) {
                jo.throw_error( "bounds array should be in order [ lo, hi ]" );
            }
        }
        if( jo.has_member( "poisson" ) ) {
            if( jo.has_member( "binomial" ) ) {
                jo.throw_error( "can't use multiple distribution types" );
            }
            double mean = jo.get_float( "poisson", 1.0 );
            if( mean <= 0.0 ) {
                jo.throw_error( "poisson mean must be greater than 0.0" );
            }
            impl_ = make_shared_fast<poisson_distribution>( lo, hi, mean );
        } else if( jo.has_member( "binomial" ) ) {
            JsonArray arr = jo.get_array( "binomial" );
            if( jo.get_array( "binomial" ).size() != 2 ) {
                jo.throw_error( "binomial array has wrong number of elements, should be [ t, p ]" );
            }
            int t = jo.get_array( "binomial" ).get_int( 0 );
            double p = jo.get_array( "binomial" ).get_float( 1 );
            if( t < 0 ) {
                jo.throw_error( "t trials must be 0 or greater" );
            }
            if( p < 0.0 || p > 1.0 ) {
                jo.throw_error( "success probability must be between 0.0 and 1.0" );
            }
            impl_ =  make_shared_fast<binomial_distribution>( lo, hi, t, p );
        } else {
            jo.throw_error( R"(Expected "poisson" or "binomial" member)" );
        }
    } else {
        jin.throw_error( "expected integer, array or object" );
    }
}
