#ifndef CATA_SRC_MATH_PARSER_DIAG_H
#define CATA_SRC_MATH_PARSER_DIAG_H

#include <array>
#include <functional>
#include <string_view>
#include <vector>

template<class D>
struct dialogue_func {
    dialogue_func<D>( std::string_view s_, std::string_view sc_, int n_ ) : symbol( s_ ),
        scopes( sc_ ), num_params( n_ ) {}
    std::string_view symbol;
    std::string_view scopes;
    int num_params{};
};

template <class D>
struct dialogue_func_eval : dialogue_func<D> {
    using f_t = std::function<double( D const & )> ( * )( char scope,
                std::vector<std::string> const & );

    dialogue_func_eval<D>( std::string_view s_, std::string_view sc_, int n_, f_t f_ )
        : dialogue_func<D>( s_, sc_, n_ ), f( f_ ) {}

    f_t f;
};

template <class D>
struct dialogue_func_ass : dialogue_func<D> {
    using f_t = std::function<void( D const &, double )> ( * )( char scope,
                std::vector<std::string> const & );

    dialogue_func_ass<D>( std::string_view s_, std::string_view sc_, int n_, f_t f_ )
        : dialogue_func<D>( s_, sc_, n_ ), f( f_ ) {}

    f_t f;
};

template<class D>
using pdiag_func_eval = dialogue_func_eval<D> const *;
template<class D>
using pdiag_func_ass = dialogue_func_ass<D> const *;

bool is_beta( char scope );

template<class D>
std::function<double( D const & )> u_val( char scope,
        std::vector<std::string> const &params );
template<class D>
std::function<void( D const &, double )> u_val_ass( char scope,
        std::vector<std::string> const &params );

template<class D>
std::function<double( D const & )> pain_eval( char scope,
        std::vector<std::string> const &/* params */ )
{
    return [beta = is_beta( scope )]( D const & d ) {
        return d.actor( beta )->pain_cur();
    };
}

template<class D>
std::function<void( D const &, double )> pain_ass( char scope,
        std::vector<std::string> const &/* params */ )
{
    return [beta = is_beta( scope )]( D const & d, double val ) {
        d.actor( beta )->set_pain( val );
    };
}

template<class D>
inline std::array<dialogue_func_eval<D>, 2> const dialogue_eval_f{
    dialogue_func_eval{ "val", "un", -1, u_val<D> },
    dialogue_func_eval{ "pain", "un", 0, pain_eval<D> },
};

template<class D>
inline std::array<dialogue_func_ass<D>, 2> const dialogue_assign_f{
    dialogue_func_ass{ "val", "un", -1, u_val_ass<D> },
    dialogue_func_ass{ "pain", "un", 0, pain_ass<D> },
};

#endif // CATA_SRC_MATH_PARSER_DIAG_H
