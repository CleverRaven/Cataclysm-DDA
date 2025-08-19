#include "speech.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "flexbuffer_json.h"
#include "rng.h"

static std::map<std::string, std::vector<SpeechBubble>> speech;

static SpeechBubble nullSpeech = { no_translation( "INVALID SPEECH" ), 0 };

void load_speech( const JsonObject &jo )
{
    translation sound;
    jo.read( "sound", sound );
    const int volume = jo.get_int( "volume" );
    for( const std::string &label : jo.get_tags( "speaker" ) ) {
        speech[label].emplace_back( SpeechBubble{ sound, volume } );
    }
}

void reset_speech()
{
    speech.clear();
}

const SpeechBubble &get_speech( const std::string &label )
{
    const std::map<std::string, std::vector<SpeechBubble> >::iterator speech_type = speech.find(
                label );

    if( speech_type == speech.end() || speech_type->second.empty() ) {
        // Bad lookup, return a fake sound, also warn?
        return nullSpeech;
    }

    return random_entry_ref( speech_type->second );
}
