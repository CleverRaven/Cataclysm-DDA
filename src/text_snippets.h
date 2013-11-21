#ifndef _TEXT_SNIPPET_H_
#define _TEXT_SNIPPET_H_

#include "json.h"

#include <map>
#include <string>

class snippet_library
{
public:
    snippet_library();

    void load_snippet(JsonObject &jsobj);
    int assign( const std::string category ) const;
    std::string get( const int index ) const;

private:
    // Snippets holds a map from the strings hash to the string.
    // This is so the reference to the string remains stable across
    // changes to the layout and contents of the snippets json file.
    std::map<int, std::string> snippets;
    // Categories groups snippets by well, category.
    std::multimap<std::string, int> categories;
};

extern snippet_library SNIPPET;

#endif // _TEXT_SNIPPET_H_
