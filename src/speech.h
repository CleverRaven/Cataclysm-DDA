#pragma once
#ifndef CATA_SRC_SPEECH_H
#define CATA_SRC_SPEECH_H

#include <string>

#include "translation.h"

class JsonObject;

struct SpeechBubble {
    translation text;
    int volume = 0;
};

void load_speech( const JsonObject &jo );
void reset_speech();
const SpeechBubble &get_speech( const std::string &label );

#endif // CATA_SRC_SPEECH_H
