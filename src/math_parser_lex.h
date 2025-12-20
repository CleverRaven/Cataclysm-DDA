#pragma once
#ifndef CATA_SRC_MATH_PARSER_LEX_H
#define CATA_SRC_MATH_PARSER_LEX_H

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace math
{

enum class token_t : std::uint8_t {
    id = 0,
    number,
    oper,
    string,
};

struct token {
    token_t type;
    std::string_view str;

    token( token_t t_, std::string_view s_ ) : type( t_ ), str( s_ ) {}
};

class lexer
{
    public:
        explicit lexer( std::string_view str_ );
        std::vector<token> const &tokens();

    private:
        std::string_view str;
        std::vector<token> ret;

        void skip( std::size_t n );
        void tokenize( token_t type, std::size_t n );
        void read_number();
        void read_id();
        void read_string();
};

} // namespace math

#endif // CATA_SRC_MATH_PARSER_LEX_H
