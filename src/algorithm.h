#pragma once
// Pragma once is supported on all modern compilers
// TODO: decide whether to use include guards anyway

#include <algorithm>

//--------------------------------------------------------------------------------------------------
// Container-wide versions of std algoithms.
//--------------------------------------------------------------------------------------------------

template <typename Container, typename Predicate>
inline bool any_of(Container&& c, Predicate p) {
    using std::begin; //ADL
    using std::end;   //ADL

    return std::any_of(begin(c), end(c), p);
}

template <typename Container, typename Predicate>
inline bool all_of(Container&& c, Predicate p) {
    using std::begin; //ADL
    using std::end;   //ADL

    return std::all_of(begin(c), end(c), p);
}

template <typename Container, typename Predicate, typename Iterator>
inline Iterator copy_if(Container&& c, Iterator dst, Predicate p) {
    using std::begin; //ADL
    using std::end;   //ADL

    return std::copy_if(begin(c), end(c), dst, p);
}

//--------------------------------------------------------------------------------------------------
// Generalized infix delimited container stream output.
//--------------------------------------------------------------------------------------------------
namespace detail {

struct identity_t {
    template <typename T>
    T&& operator()(T&& t) const noexcept {
        return std::forward<T>(t);
    }
};

template <typename Container, typename Transform = identity_t>
struct delimited_values_t {
    delimited_values_t(Container const &cont, char const *delim, Transform trans)
      : cont  {cont}
      , delim {delim}
      , trans {trans}
    {
    }

    template <typename Stream>
    friend Stream& operator<<(Stream &out, delimited_values_t const& d) {
        using std::begin; //ADL
        using std::end;   //ADL

        auto it = begin(d.cont);
        auto const last = end(d.cont);
        
        if (it == last) {
            return out;
        }

        out << d.trans(*it);
        while (++it != last) {
            out << d.delim << d.trans(*it);
        }

        return out;
    }

    Container const &cont;
    char const      *delim;
    Transform       trans;
};

} //namespace detail

//--------------------------------------------------------------------------------------------------
/**
 * Ouputs the contents of a container to a stream with an infix delimiter.
 * i.e "a, b, c" not "a, b, c,".
 */
template <typename Container, typename Transform = detail::identity_t>
detail::delimited_values_t<Container, Transform>
infix_delimited(Container const &c, char const *delim, Transform t = Transform {}) {
    return detail::delimited_values_t<Container, Transform>(c, delim, t);
}
