#ifndef CATA_SRC_CLONE_PTR_H
#define CATA_SRC_CLONE_PTR_H

#include <memory>

namespace cata
{

template<typename T>
class clone_ptr
{
    public:
        clone_ptr() = default;
        clone_ptr( std::nullptr_t ) {}
        clone_ptr( const clone_ptr &other ) : p_( other.p_ ? other.p_->clone() : nullptr ) {}
        clone_ptr( clone_ptr && ) = default;
        clone_ptr &operator=( const clone_ptr &other ) {
            p_ = other.p_ ? other.p_->clone() : nullptr;
            return *this;
        }
        clone_ptr &operator=( clone_ptr && ) = default;

        // implicit conversion from unique_ptr
        template<typename U>
        clone_ptr( std::unique_ptr<U> p ) : p_( std::move( p ) ) {}

        T &operator*() {
            return *p_;
        }
        const T &operator*() const {
            return *p_;
        }
        T *operator->() {
            return p_.get();
        }
        const T *operator->() const {
            return p_.get();
        }
        T *get() {
            return p_.get();
        }
        const T *get() const {
            return p_.get();
        }

        explicit operator bool() const {
            return !!*this;
        }
        bool operator!() const {
            return !p_;
        }

        friend bool operator==( const clone_ptr &l, const clone_ptr &r ) {
            return l.p_ == r.p_;
        }
        friend bool operator!=( const clone_ptr &l, const clone_ptr &r ) {
            return l.p_ != r.p_;
        }
    private:
        std::unique_ptr<T> p_;
};

} // namespace cata

#endif // CATA_SRC_CLONE_PTR_H
