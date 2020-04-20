#pragma once
#ifndef CATA_TESTS_OPTIONS_HELPERS_H
#define CATA_TESTS_OPTIONS_HELPERS_H

#include <string>

// RAII class to temporarily override a particular option value
// The previous value will be restored in the destructor
class override_option
{
    public:
        override_option( const std::string &option, const std::string &value );
        override_option( const override_option & ) = delete;
        override_option &operator=( const override_option & ) = delete;
        ~override_option();
    private:
        std::string option_;
        std::string old_value_;
};

#endif // CATA_TESTS_OPTIONS_HELPERS_H
