#include "translations.h"

#include <string>
#ifdef LOCALIZE
#else // !LOCALIZE
#include <cstring> // strcmp
#include <map>
#endif // LOCALIZE


#ifdef LOCALIZE

const char * pgettext(const char *context, const char *msgid)
{
    // need to construct the string manually,
    // to correctly handle strings loaded from json.
    // could probably do this more efficiently without using std::string.
    std::string context_id(context);
    context_id += '\004';
    context_id += msgid;
    // null domain, uses global translation domain
    const char *msg_ctxt_id = context_id.c_str();
    const char *translation = dcgettext(NULL, msg_ctxt_id, LC_MESSAGES);
    if (translation == msg_ctxt_id) {
        return msgid;
    } else {
        return translation;
    }
}

#else // !LOCALIZE

// sanitized message cache
std::map<const char*,std::string> sanitized_messages;

const char * strip_positional_formatting(const char *msgid)
{
    // first check if we have it cached
    if (sanitized_messages.find(msgid) != sanitized_messages.end()) {
        if (sanitized_messages[msgid] == "") {
            return msgid;
        } else {
            return sanitized_messages[msgid].c_str();
        }
    }
    std::string s(msgid);
    // basic usage is just to change all "%{number}$" to "%".
    // thus for example "%2$s" will change to simply "%s".
    // strings must have their parameters in strict order,
    // or else this will not work correctly.
    size_t pos = 0;
    size_t len = s.length();
    bool changed = false;
    while (pos < len) {
        pos = s.find('%', pos);
        if (pos == std::string::npos || pos + 2 >= len) {
            break;
        }
        size_t dollarpos = pos + 1;
        while (dollarpos < len && s[dollarpos] >= '0' && s[dollarpos] <= '9') {
            ++dollarpos;
        }
        if (dollarpos >= len) {
            break;
        }
        if (s[dollarpos] != '$') {
            pos = dollarpos + 1; // skip format type (also skips %%)
            continue;
        }
        s.erase(pos + 1, dollarpos - pos);
        len = s.length(); // because it ain't da same no more
        changed = true;
        ++pos;
    }

    if (!changed) {
        sanitized_messages[msgid] = "";
        return msgid;
    } else {
        sanitized_messages[msgid] = s;
        return sanitized_messages[msgid].c_str();
    }
    return msgid;
}

#endif // LOCALIZE
