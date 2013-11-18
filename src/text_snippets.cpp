#include "json.h"
#include "rng.h"
#include "text_snippets.h"
#include "translations.h"

snippet_library SNIPPET;

snippet_library::snippet_library() {}

void snippet_library::load_snippet(JsonObject &jsobj)
{
    std::string category = jsobj.get_string("category");
    std::string text = _(jsobj.get_string("text").c_str());
    int hash = djb2_hash( (const unsigned char*)text.c_str() );

    snippets.insert( std::pair<int, std::string>(hash, text) );
    categories.insert( std::pair<std::string, int>(category, hash) );
}

void snippet_library::clear_snippets()
{
    snippets.clear();
    categories.clear();
}

int snippet_library::assign( const std::string category ) const
{
    std::multimap<std::string,int>::const_iterator category_start = categories.lower_bound(category);
    if( category_start == categories.end() )
    {
        return 0;
    }
    const int selected_text = rng( 0, categories.count(category) - 1 );
    std::multimap<std::string,int>::const_iterator it = category_start;
    for( int index = 0; index < selected_text; ++index )
    {
        ++it;
    }
    return it->second;
}

std::string snippet_library::get( const int index ) const
{
    std::map<int, std::string>::const_iterator chosen_snippet = snippets.find(index);
    if( chosen_snippet == snippets.end() )
    {
        return "";
    }
    return chosen_snippet->second;
}
