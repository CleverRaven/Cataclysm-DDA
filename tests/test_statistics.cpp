#include "test_statistics.h"

#include <sstream>

BinomialMatcher::BinomialMatcher( const int num_samples, const double p,
                                  const double max_deviation ) :
    num_samples_( num_samples ),
    p_( p ),
    max_deviation_( max_deviation )
{
    // Binomial expectation is np, variance is np(1-p).
    // So we want | obs - np | <= max_deviation * sqrt( np(1-p) )
    expected_ = num_samples_ * p_;
    margin_ = max_deviation_ * sqrt( num_samples_ * p_ * ( 1 - p_ ) );
}

bool BinomialMatcher::match( const int &obs ) const
{
    return expected_ - margin_ <= obs && obs <= expected_ + margin_;
}

std::string BinomialMatcher::describe() const
{
    std::ostringstream os;
    os << "is from Bin(" << num_samples_ << ", " << p_ << ") [" <<
       ( expected_ - margin_ ) << " <= obs <= " << ( expected_ + margin_ ) << "]";
    return os.str();
}
