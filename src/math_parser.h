#ifndef CATA_SRC_MATH_PARSER_H
#define CATA_SRC_MATH_PARSER_H

#include <memory>
#include <string_view>

struct dialogue;
struct mission_goal_condition_context;

template<class D>
class math_exp
{
    public:
        ~math_exp();
        math_exp();
        math_exp( math_exp const &other );
        math_exp( math_exp &&/* other */ ) noexcept;
        math_exp &operator=( math_exp const &other );
        math_exp &operator=( math_exp &&/* other */ ) noexcept;

        bool parse( std::string_view str, bool assignment = false );
        double eval( D const &d ) const;
        void assign( D const &d, double val ) const;

    private:
        class math_exp_impl;
        std::unique_ptr<math_exp_impl> impl;
};

extern template class math_exp<dialogue>;
extern template class math_exp<mission_goal_condition_context>;

#endif // CATA_SRC_MATH_PARSER_H
