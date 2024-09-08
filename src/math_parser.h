#pragma once
#ifndef CATA_SRC_MATH_PARSER_H
#define CATA_SRC_MATH_PARSER_H

#include <memory>
#include <string_view>

struct dialogue;

class math_exp
{
    public:
        class math_exp_impl;

        ~math_exp();
        math_exp();
        math_exp( math_exp const &other );
        math_exp( math_exp &&/* other */ ) noexcept;
        math_exp &operator=( math_exp const &other );
        math_exp &operator=( math_exp &&/* other */ ) noexcept;
        explicit math_exp( math_exp_impl impl_ );

        bool parse( std::string_view str, bool assignment = false );
        double eval( dialogue &d ) const;
        void assign( dialogue &d, double val ) const;

    private:
        std::unique_ptr<math_exp_impl> impl;
};

#endif // CATA_SRC_MATH_PARSER_H
