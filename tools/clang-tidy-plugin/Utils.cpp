#include "Utils.h"

namespace clang
{
namespace tidy
{
namespace cata
{

NameConvention::NameConvention( StringRef xName )
{
    if( xName.endswith( "x" ) ) {
        root = xName.drop_back();
        capital = false;
        atEnd = true;
    } else if( xName.endswith( "X" ) ) {
        root = xName.drop_back();
        capital = true;
        atEnd = true;
    } else if( xName.startswith( "x" ) ) {
        root = xName.drop_front();
        capital = false;
        atEnd = false;
    } else if( xName.startswith( "X" ) ) {
        root = xName.drop_front();
        capital = true;
        atEnd = false;
    } else {
        valid = false;
    }
}

NameConvention::MatchResult NameConvention::Match( StringRef name )
{
    if( name.empty() ) {
        return None;
    }

    StringRef Root = atEnd ? name.drop_back() : name.drop_front();
    if( Root != root ) {
        return None;
    }

    char Dim = atEnd ? name.back() : name.front();
    switch( Dim ) {
        case 'x':
        case 'X':
            return XName;
        case 'y':
        case 'Y':
            return YName;
        case 'z':
        case 'Z':
            return ZName;
        default:
            return None;
    }
}

} // namespace cata
} // namespace tidy
} // namespace clang
