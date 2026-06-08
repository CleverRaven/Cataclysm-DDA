#pragma once
#ifndef CATA_SRC_ITEM_UID_H
#define CATA_SRC_ITEM_UID_H

#include <cstdint>
#include <iosfwd>

class JsonOut;

// Generate a fresh UID from the global counter.
// Returns 0 if the game state is not yet initialized.
int64_t generate_next_item_uid();

class item_uid
{
    public:
        item_uid() : value( 0 ) {}

        explicit item_uid( int64_t i ) : value( i ) {}

        // Copy: generate fresh UID (like safe_reference_anchor)
        item_uid( const item_uid & ) : value( generate_next_item_uid() ) {}

        // Move: transfer value, zero source
        item_uid( item_uid &&other ) noexcept : value( other.value ) {
            other.value = 0;
        }

        // Copy assignment: generate fresh UID
        item_uid &operator=( const item_uid & ) {
            value = generate_next_item_uid();
            return *this;
        }

        // Move assignment: transfer value, zero source
        item_uid &operator=( item_uid &&other ) noexcept {
            value = other.value;
            other.value = 0;
            return *this;
        }

        bool is_valid() const {
            return value > 0;
        }

        int64_t get_value() const {
            return value;
        }

        void serialize( JsonOut & ) const;
        void deserialize( int64_t );

    private:
        int64_t value;
};

inline bool operator==( const item_uid &l, const item_uid &r )
{
    return l.get_value() == r.get_value();
}

inline bool operator!=( const item_uid &l, const item_uid &r )
{
    return l.get_value() != r.get_value();
}

std::ostream &operator<<( std::ostream &o, const item_uid &id );

#endif // CATA_SRC_ITEM_UID_H
