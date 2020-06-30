#ifndef CATA_TOOLS_CLANG_TIDY_PLUGIN_STRINGLITERALITERATOR_H
#define CATA_TOOLS_CLANG_TIDY_PLUGIN_STRINGLITERALITERATOR_H

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/TargetInfo.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>

namespace clang
{
class LangOptions;
class SourceManager;
class StringLiteral;

namespace tidy
{
namespace cata
{

// Currently only supports utf8 becasue StringLiteral only has full support
// for single-byte ascii/utf8 strings
class StringLiteralIterator
{
    public:
        // This assumes that ind points to the start of a valid utf8 sequence
        StringLiteralIterator( const StringLiteral &str, size_t ind );

        // Get the source location corresponding to the character pointed to
        // by the iterator.
        // Do note that clang crashes when the string literal is a predefined
        // expression such as __func__, so you may want to exclude them using
        // the matcher `unless( hasAncestor( predefinedExpr() ) )` or something
        // to that effect.
        SourceLocation toSourceLocation( const SourceManager &SrcMgr, const LangOptions &LangOpts,
                                         const TargetInfo &Info ) const;

        // All following operators assume that StringLiteral contains a valid
        // utf8 string
        uint32_t operator*() const;

        bool operator<( const StringLiteralIterator &rhs ) const;
        bool operator>( const StringLiteralIterator &rhs ) const;
        bool operator<=( const StringLiteralIterator &rhs ) const;
        bool operator>=( const StringLiteralIterator &rhs ) const;
        bool operator==( const StringLiteralIterator &rhs ) const;
        bool operator!=( const StringLiteralIterator &rhs ) const;

        StringLiteralIterator &operator+=( ptrdiff_t inc );
        StringLiteralIterator &operator-=( ptrdiff_t dec );
        StringLiteralIterator operator+( ptrdiff_t inc ) const;
        StringLiteralIterator operator-( ptrdiff_t dec ) const;
        StringLiteralIterator &operator++();
        StringLiteralIterator operator++( int );
        StringLiteralIterator &operator--();
        StringLiteralIterator operator--( int );

        static StringLiteralIterator begin( const StringLiteral &str );
        static StringLiteralIterator end( const StringLiteral &str );

        static ptrdiff_t distance( const StringLiteralIterator &beg,
                                   const StringLiteralIterator &end );

    private:
        std::reference_wrapper<const StringLiteral> str;
        size_t ind;
};

} // namespace cata
} // namespace tidy
} // namespace clang

namespace std
{
template<>
struct iterator_traits<clang::tidy::cata::StringLiteralIterator> {
    using difference_type = ptrdiff_t;
    using value_type = uint32_t;
    using pointer = const uint32_t *;
    using reference = const uint32_t &;
    // randome_access_iterator_tag requires constant increment/decrement time,
    // which StringLiteralIterator doesn't satisfy.
    using iterator_category = bidirectional_iterator_tag;
};
} // namespace std

#endif // CATA_TOOLS_CLANG_TIDY_PLUGIN_STRINGLITERALITERATOR_H
