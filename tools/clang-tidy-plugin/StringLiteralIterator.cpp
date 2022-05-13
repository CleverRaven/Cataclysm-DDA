#include "StringLiteralIterator.h"

#include <algorithm>
#include <clang/AST/Expr.h>
#include <cstddef>
#include <cstdint>

namespace clang
{
class LangOptions;
class SourceManager;

namespace tidy
{
namespace cata
{

StringLiteralIterator::StringLiteralIterator( const StringLiteral &str, const size_t ind )
    : str( str ), ind( ind )
{
}

SourceLocation StringLiteralIterator::toSourceLocation( const SourceManager &SrcMgr,
        const LangOptions &LangOpts, const TargetInfo &Info ) const
{
    return str.get().getLocationOfByte( ind, SrcMgr, LangOpts, Info );
}

uint32_t StringLiteralIterator::operator*() const
{
    uint32_t ch = str.get().getCodeUnit( ind );
    unsigned int n;
    if( ch >= 0xFC ) {
        ch &= 0x01;
        n = 5;
    } else if( ch >= 0xF8 ) {
        ch &= 0x03;
        n = 4;
    } else if( ch >= 0xF0 ) {
        ch &= 0x07;
        n = 3;
    } else if( ch >= 0xE0 ) {
        ch &= 0x0F;
        n = 2;
    } else if( ch >= 0xC0 ) {
        ch &= 0x1F;
        n = 1;
    } else {
        n = 0;
    }
    for( size_t i = 1; i <= n; ++i ) {
        if( ind + i >= str.get().getLength() ) {
            return 0xFFFD; // unknown unicode
        }
        ch = ( ch << 6 ) | ( str.get().getCodeUnit( ind + i ) & 0x3F );
    }
    return ch;
}

bool StringLiteralIterator::operator<( const StringLiteralIterator &rhs ) const
{
    return ind < rhs.ind;
}

bool StringLiteralIterator::operator>( const StringLiteralIterator &rhs ) const
{
    return rhs.operator < ( *this );
}

bool StringLiteralIterator::operator<=( const StringLiteralIterator &rhs ) const
{
    return !rhs.operator < ( *this );
}

bool StringLiteralIterator::operator>=( const StringLiteralIterator &rhs ) const
{
    return !operator<( rhs );
}

bool StringLiteralIterator::operator==( const StringLiteralIterator &rhs ) const
{
    return ind == rhs.ind;
}

bool StringLiteralIterator::operator!=( const StringLiteralIterator &rhs ) const
{
    return !operator==( rhs );
}

StringLiteralIterator &StringLiteralIterator::operator+=( ptrdiff_t inc )
{
    if( inc == 0 ) {
        return *this;
    } else if( inc > 0 ) {
        while( inc > 0 && ind < str.get().getLength() ) {
            uint32_t byte = str.get().getCodeUnit( ind );
            if( byte >= 0xFC ) {
                ind += 6;
            } else if( byte >= 0xF8 ) {
                ind += 5;
            } else if( byte >= 0xF0 ) {
                ind += 4;
            } else if( byte >= 0xE0 ) {
                ind += 3;
            } else if( byte >= 0xC0 ) {
                ind += 2;
            } else {
                ++ind;
            }
            --inc;
        }
        ind = std::min<size_t>( ind, str.get().getLength() );
    } else {
        while( inc < 0 && ind > 0 ) {
            uint32_t byte = str.get().getCodeUnit( ind - 1 );
            if( byte < 0x80 || byte >= 0xC0 ) {
                ++inc;
            }
            --ind;
        }
    }
    return *this;
}

StringLiteralIterator &StringLiteralIterator::operator-=( ptrdiff_t dec )
{
    return operator+=( -dec );
}

StringLiteralIterator StringLiteralIterator::operator+( ptrdiff_t inc ) const
{
    StringLiteralIterator ret = *this;
    ret.operator += ( inc );
    return ret;
}

StringLiteralIterator StringLiteralIterator::operator-( ptrdiff_t dec ) const
{
    return operator+( -dec );
}

StringLiteralIterator &StringLiteralIterator::operator++()
{
    return operator+=( 1 );
}

StringLiteralIterator StringLiteralIterator::operator++( int )
{
    StringLiteralIterator ret = *this;
    operator++();
    return ret;
}

StringLiteralIterator &StringLiteralIterator::operator--()
{
    return operator-=( 1 );
}

StringLiteralIterator StringLiteralIterator::operator--( int )
{
    StringLiteralIterator ret = *this;
    operator--();
    return ret;
}

StringLiteralIterator StringLiteralIterator::begin( const StringLiteral &str )
{
    return StringLiteralIterator( str, 0 );
}

StringLiteralIterator StringLiteralIterator::end( const StringLiteral &str )
{
    return StringLiteralIterator( str, str.getLength() );
}

ptrdiff_t StringLiteralIterator::distance( const StringLiteralIterator &beg,
        const StringLiteralIterator &end )
{
    if( beg <= end ) {
        ptrdiff_t dist = 0;
        for( auto it = beg; it < end; ++it, ++dist ) {
        }
        return dist;
    } else {
        return -distance( end, beg );
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
