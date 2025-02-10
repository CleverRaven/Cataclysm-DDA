#pragma once
#ifndef CATA_SRC_DISTRIBUTION_H
#define CATA_SRC_DISTRIBUTION_H

#include <string>

#include "memory_fast.h"  // IWYU pragma: keep

class JsonValue;
struct int_distribution_impl;

// This represents a probability distribution over the integers, which is
// abstract and can be read from a JSON definition
class int_distribution
{
    public:
        int_distribution();
        explicit int_distribution( int value );
        int minimum() const;
        int sample() const;
        std::string description() const;

        void deserialize( const JsonValue & );
    private:
        shared_ptr_fast<int_distribution_impl> impl_;
};

#endif // CATA_SRC_DISTRIBUTION_H
