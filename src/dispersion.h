#pragma once
#ifndef CATA_SRC_DISPERSION_H
#define CATA_SRC_DISPERSION_H

#include <iosfwd>
#include <vector>

class dispersion_sources
{
    private:
        std::vector<double> normal_sources;
        std::vector<double> linear_sources;
        std::vector<double> multipliers;
        double spread_sources = 0.0;
    public:
        explicit dispersion_sources( double normal_source = 0.0 ) {
            if( normal_source != 0.0 ) {
                normal_sources.push_back( normal_source );
            }
        }
        void add_range( double new_source ) {
            linear_sources.push_back( new_source );
        }
        void add_multiplier( double new_multiplier ) {
            multipliers.push_back( new_multiplier );
        }
        void add_spread( double new_spread ) {
            spread_sources = new_spread;
        }
        double roll() const;
        double max() const;
        double avg() const;

        friend std::ostream &operator<<( std::ostream &stream, const dispersion_sources &sources );
};

#endif // CATA_SRC_DISPERSION_H
