#include "Utils.h"

namespace clang::tidy::cata
{

bool isPointMethod( const FunctionDecl *d )
{
    if( const CXXMethodDecl *Method = dyn_cast_or_null<CXXMethodDecl>( d ) ) {
        const CXXRecordDecl *Record = Method->getParent();
        if( isPointType( Record ) ) {
            return true;
        }
    }
    return false;
}

NameConvention::NameConvention( StringRef xName )
{
    if( xName.ends_with( "x" ) ) {
        root = xName.drop_back().str();
        capital = false;
        atEnd = true;
    } else if( xName.ends_with( "X" ) ) {
        root = xName.drop_back().str();
        capital = true;
        atEnd = true;
    } else if( xName.starts_with( "x" ) ) {
        root = xName.drop_front().str();
        capital = false;
        atEnd = false;
    } else if( xName.starts_with( "X" ) ) {
        root = xName.drop_front().str();
        capital = true;
        atEnd = false;
    } else {
        valid = false;
    }
}

NameConvention::MatchResult NameConvention::Match( StringRef name ) const
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

} // namespace clang::tidy::cata
