#pragma once
#ifndef DISPERSION_H
#define DISPERSION_H

#include <iosfwd>
#include <vector>

// Note: 4/30/2019, changing dispersion sources to reflect a normal 
// distribution, where the sum of all normal_sources is the standard
// deviation of the distribution.  add_range should become depricated
class dispersion_sources
{
    private:
        std::vector<double> normal_sources;
        std::vector<double> linear_sources;
        std::vector<double> multipliers;
    public:
        dispersion_sources( double normal_source = 0.0 ) {
            if( normal_source != 0.0 ) {
                normal_sources.push_back( normal_source );
            }
        }
        void add_normal( double new_source ) {
            normal_sources.push_back( new_source );
        }
        void add_range( double new_source ) {
            linear_sources.push_back( new_source );
        }
        void add_multiplier( double new_multiplier ) {
            multipliers.push_back( new_multiplier );
        }
        double roll() const;
        double max() const;
        double avg() const;
        double stdev() const;

        friend std::ostream &operator<<( std::ostream &stream, const dispersion_sources &sources );
};

#endif
