#pragma once
#ifndef TEST_STATISTICS_H
#define TEST_STATISTICS_H

#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>
#include <string>
#include <type_traits>

#include "catch/catch.hpp"

// Z-value for confidence interval
constexpr double Z95 = 1.96;
constexpr double Z99 = 2.576;
constexpr double Z99_9 = 3.291;
constexpr double Z99_99 = 3.891;
constexpr double Z99_999 = 4.5;
constexpr double Z99_999_9 = 5.0;

// Useful to specify a range using midpoint +/- ε which is easier to parse how
// wide a range actually is vs just upper and lower
struct epsilon_threshold {
    double midpoint;
    double epsilon;
};

// Upper/lower bound threshold useful for asymmetric thresholds
struct upper_lower_threshold {
    double lower_thresh;
    double upper_thresh;
};

// we cache the margin of error so when adding a new value we must invalidate
// it so it gets calculated a again
static constexpr double invalid_err = -1;

template<typename T>
class statistics
{
    private:
        int _types;
        int _n;
        double _sum;
        double _error;
        const double _Z;
        const double _Zsq;
        T _max;
        T _min;
        std::vector< T > samples;
    public:
        statistics( const double Z = Z99_9 ) : _types( 0 ), _n( 0 ), _sum( 0 ), _error( invalid_err ),
            _Z( Z ),  _Zsq( Z * Z ), _max( std::numeric_limits<T>::min() ),
            _min( std::numeric_limits<T>::max() ) {}

        void new_type() {
            _types++;
        }
        void add( T new_val ) {
            _error = invalid_err;
            _n++;
            _sum += new_val;
            _max = std::max( _max, new_val );
            _min = std::min( _min, new_val );
            samples.push_back( new_val );
        }
        void add( statistics<T> & stat )
        {
            _error = invalid_err;
            _n += stat.n();
            _sum += stat.sum();
            _max = std::max( _max, stat.max() );
            _min = std::min( _min, stat.min() );
            for( T sample:stat.get_samples() )
            {
                samples.push_back( sample );
            }
        }

        // Adjusted Wald error is only valid for a discrete binary test. Note
        // because error takes into account population, it is only valid to
        // test against the upper/lower bound.
        //
        // The goal here is to get the most accurate statistics about the
        // smallest sample size feasible.  The tests end up getting run many
        // times over a short period, so any real issue may sometimes get a
        // false positive, but over a series of runs will get shaken out in an
        // obvious way.
        //
        // Outside of this class, this should only be used for debugging
        // purposes.
        template<typename U = T>
        typename std::enable_if< std::is_same< U, bool >::value, double >::type
        margin_of_error() {
            if( _error != invalid_err ) {
                return _error;
            }
            // Implementation of outline from https://measuringu.com/ci-five-steps/
            const double adj_numerator = ( _Zsq / 2 ) + _sum;
            const double adj_denominator = _Zsq + _n;
            const double adj_proportion = adj_numerator / adj_denominator;
            const double a = adj_proportion * ( 1.0 - adj_proportion );
            const double b = a / adj_denominator;
            const double c = std::sqrt( b );
            _error = c * _Z;
            return _error;
        }
        // Standard error is intended to be used with continuous data samples.
        // We're using an approximation here so it is only appropriate to use
        // the upper/lower bound to test for reasons similar to adjusted Wald
        // error.
        // Outside of this class, this should only be used for debugging purposes.
        // https://measuringu.com/ci-five-steps/
        template<typename U = T>
        typename std::enable_if < ! std::is_same< U, bool >::value, double >::type
        margin_of_error() {
            if( _error != invalid_err ) {
                return _error;
            }
            const double std_err = stddev() / std::sqrt( _n );
            _error = std_err * _Z;
            return _error;
        }

        /** Use to continue testing until we are sure whether the result is
         * inside or outside the target.
         *
         * Returns true if the confidence interval partially overlaps the target region.
         */
        bool uncertain_about( const epsilon_threshold &t ) {
            return !test_threshold( t ) && // Inside target
                   t.midpoint - t.epsilon < upper() && // Below target
                   t.midpoint + t.epsilon > lower(); // Above target
        }

        bool test_threshold( const epsilon_threshold &t ) {
            return ( ( t.midpoint - t.epsilon ) < lower() &&
                     ( t.midpoint + t.epsilon ) > upper() );
        }
        bool test_threshold( const upper_lower_threshold &t ) {
            return ( t.lower_thresh < lower() && t.upper_thresh > upper() );
        }
        double upper() {
            double result = avg() + margin_of_error();
            if( std::is_same<T, bool>::value ) {
                result = std::min( result, 1.0 );
            }
            return result;
        }
        double lower() {
            double result = avg() - margin_of_error();
            if( std::is_same<T, bool>::value ) {
                result = std::max( result, 0.0 );
            }
            return result;
        }
        // Test if some value is a member of the confidence interval of the
        // sample
        bool test_confidence_interval( const double v ) const {
            return is_within_epsilon( v, margin_of_error() );
        }

        bool is_within_epsilon( const double v, const double epsilon ) const {
            const double average = avg();
            return( ( average + epsilon > v ) &&
                    ( average - epsilon < v ) );
        }
        // Theoretically a one-pass formula is more efficient, however because
        // we keep handles onto _sum and _n as class members and calculate them
        // on the fly, a one-pass formula is unnecessary because we're already
        // one pass here.  It may not obvious that even though we're calling
        // the 'average()' function that's what is happening.
        double variance( const bool sample_variance = true ) const {
            double average = avg();
            double sigma_acc = 0;

            for( const auto v : samples ) {
                const double diff = v - average;
                sigma_acc += diff * diff;
            }

            if( sample_variance ) {
                return sigma_acc / static_cast<double>( _n - 1 );
            }
            return sigma_acc / static_cast<double>( _n );
        }
        // We should only be interested in the sample deviation most of the
        // time because we can always get more samples.  The way we use tests,
        // we attempt to use the sample data to generalize about the
        // population.
        double stddev( const bool sample_deviation = true ) const {
            return std::sqrt( variance( sample_deviation ) );
        }

        int types() const {
            return _types;
        }
        double sum() const {
            return _sum;
        }
        T max() const {
            return _max;
        }
        T min() const {
            return _min;
        }
        double avg() const {
            return _sum / static_cast<double>( _n );
        }
        int n() const {
            return _n;
        }
        std::vector<T> get_samples() {
            return samples;
        }
};

class BinomialMatcher : public Catch::MatcherBase<int>
{
    public:
        BinomialMatcher( int num_samples, double p, double max_deviation );
        bool match( const int &obs ) const override;
        std::string describe() const override;
    private:
        int num_samples_;
        double p_;
        double max_deviation_;
        double expected_;
        double margin_;
};

// Can be used to test that a value is a plausible observation from a binomial
// distribution.  Uses a normal approximation to the binomial, and permits a
// deviation up to max_deviation (measured in standard deviations).
inline BinomialMatcher IsBinomialObservation(
    const int num_samples, const double p, const double max_deviation = Z99_99 )
{
    return BinomialMatcher( num_samples, p, max_deviation );
}

#endif
