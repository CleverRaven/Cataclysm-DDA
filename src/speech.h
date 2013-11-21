#ifndef _SPEECH_H_
#define _SPEECH_H_

#include "json.h"


struct SpeechBubble {
    std::string text;
    int volume;
};

void load_speech(JsonObject &jo);
const SpeechBubble& get_speech( const std::string label );

#endif /* _SPEECH_H_ */
