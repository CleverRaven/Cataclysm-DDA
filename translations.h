#ifndef _TRANSLATIONS_H_
#define _TRANSLATIONS_H_

#ifdef LOCALIZE

// MingW flips out if you don't define this before you try to statically link libintl.
// This should prevent 'undefined reference to `_imp__libintl_gettext`' errors.
#if defined _WIN32 || defined __CYGWIN__
 #ifndef LIBINTL_STATIC
  #define LIBINTL_STATIC
 #endif
#endif

#include <cstdio>
#include <libintl.h>
#include <clocale>
#define _(STRING) gettext(STRING)
inline static const char *
pgettext_aux (const char *domain,
              const char *msg_ctxt_id, const char *msgid,
              int category)
{
    const char *translation = dcgettext (domain, msg_ctxt_id, category);
    if (translation == msg_ctxt_id) {
        return msgid;
    } else {
        return translation;
    }
}
#define pgettext(STRING1, STRING2) \
    pgettext_aux(NULL, STRING1 "\004" STRING2, STRING2, LC_MESSAGES)

#else // !LOCALIZE

#define _(STRING) STRING
#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? STRING1 : STRING2)
#define pgettext(STRING1, STRING2) STRING2

#endif // LOCALIZE

#endif // _TRANSLATIONS_H_
