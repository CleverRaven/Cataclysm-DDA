#pragma once
#ifndef CATA_SRC_MAPGEN_PARAMETER_H
#define CATA_SRC_MAPGEN_PARAMETER_H

#include "cata_variant.h"

struct mapgen_arguments;
template <typename Id> class mapgen_value;
class mapgendata;

enum class mapgen_parameter_scope {
    overmap_special,
    omt,
    nest,
    last
};

template<>
struct enum_traits<mapgen_parameter_scope> {
    static constexpr mapgen_parameter_scope last = mapgen_parameter_scope::last;
};

class mapgen_parameter
{
    public:
        void deserialize( JsonIn & );

        mapgen_parameter_scope scope() const {
            return scope_;
        }
        cata_variant_type type() const;
        cata_variant get( const mapgendata & ) const;

        void check_consistent_with( const mapgen_parameter &, const std::string &context ) const;
    private:
        mapgen_parameter_scope scope_;
        cata_variant_type type_;
        // Using a pointer here mostly to move the definition of mapgen_value to the
        // cpp file
        std::shared_ptr<const mapgen_value<std::string>> default_;
};

struct mapgen_parameters {
    std::unordered_map<std::string, mapgen_parameter> map;

    mapgen_parameters params_for_scope( mapgen_parameter_scope scope ) const;
    mapgen_arguments get_args( const mapgendata &, mapgen_parameter_scope scope ) const;
    void check_and_merge( const mapgen_parameters &, const std::string &context );
};

#endif // CATA_SRC_MAPGEN_PARAMETER_H
