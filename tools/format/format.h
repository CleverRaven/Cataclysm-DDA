#ifndef CATA_TOOLS_FORMAT_H
#define CATA_TOOLS_FORMAT_H

class TextJsonIn;
class JsonOut;

namespace formatter
{
void format( TextJsonIn &jsin, JsonOut &jsout, int depth = -1, bool force_wrap = false );
}

#endif
