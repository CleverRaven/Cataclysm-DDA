#ifndef _TRANSLATIONS_H_
#define _TRANSLATIONS_H_

#ifndef NONLOCALIZED
#include <cstdio>
#include <libintl.h>
#include <clocale>
#define _(STRING) gettext(STRING)
#else
#define _(STRING) STRING
#define ngettext(STRING1, STRING2, COUNT) (COUNT < 2 ? STRING1 : STRING2)
#endif

#endif // _TRANSLATIONS_H_
