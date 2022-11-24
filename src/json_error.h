#pragma once
#ifndef CATA_SRC_JSON_ERROR_H
#define CATA_SRC_JSON_ERROR_H

#include <stdexcept>

class JsonError : public std::runtime_error
{
    public:
        explicit JsonError( const std::string &msg );
        const char *c_str() const noexcept {
            return what();

        }
};

#endif // CATA_SRC_JSON_ERROR_H
