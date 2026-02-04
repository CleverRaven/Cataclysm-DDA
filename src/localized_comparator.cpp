#include <locale>

#if defined(__APPLE__)
// needed by localized_comparator
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "catacharset.h"
#include "localized_comparator.h"

#if defined(_WIN32)
#include "platform_win.h"

static std::wstring utf8_to_wide_string( const std::string &str )
{
    int sz = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, nullptr, 0 ) + 1;
    std::wstring result( sz, '\0' );
    MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, result.data(), sz );
    while( !result.empty() && result.back() == '\0' ) {
        result.pop_back();
    }
    return result;
}
#endif // defined(_WIN32)

bool localized_comparator::operator()( const std::string &l, const std::string &r ) const
{
    // We need different implementations on each platform.  MacOS seems to not
    // support localized comparison of strings via the standard library at all,
    // so resort to MacOS-specific solution.  Windows cannot be expected to be
    // using a UTF-8 locale (whereas our strings are always UTF-8) and so we
    // must convert to wstring for comparison there.  Linux seems to work as
    // expected on regular strings; no workarounds needed.
    // See https://github.com/CleverRaven/Cataclysm-DDA/pull/40041 for further
    // discussion.
#if defined(__APPLE__) // macOS and iOS
    CFStringRef lr = CFStringCreateWithCStringNoCopy( kCFAllocatorDefault, l.c_str(),
                     kCFStringEncodingUTF8, kCFAllocatorNull );
    CFStringRef rr = CFStringCreateWithCStringNoCopy( kCFAllocatorDefault, r.c_str(),
                     kCFStringEncodingUTF8, kCFAllocatorNull );
    bool result = CFStringCompare( lr, rr, kCFCompareLocalized ) < 0;
    CFRelease( lr );
    CFRelease( rr );
    return result;
#elif defined(_WIN32)
    return std::locale()( utf8_to_wide_string( l ), utf8_to_wide_string( r ) );
#else
    return std::locale()( l, r );
#endif
}

bool localized_comparator::operator()( const translation &l, const translation &r ) const
{
    return l.translated_lt( r );
}
