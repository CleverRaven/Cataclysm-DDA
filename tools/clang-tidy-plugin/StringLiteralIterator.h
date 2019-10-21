#ifndef CATA_TOOLS_CLANG_TIDY_STRINGLITERALVIEW_H
#define CATA_TOOLS_CLANG_TIDY_STRINGLITERALVIEW_H

#include "ClangTidy.h"

namespace clang
{
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

#endif // CATA_TOOLS_CLANG_TIDY_STRINGLITERALVIEW_H
