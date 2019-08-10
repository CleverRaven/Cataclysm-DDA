#pragma once
#ifndef IME_H
#define IME_H

/**
 * Enable or disable IME for text/keyboard input
 */
void enable_ime();
void disable_ime();

/**
 * used before text input to enable IME and auto-disable IME when leaving the scope
 */
class ime_sentry
{
    public:
        ime_sentry();
        ~ime_sentry();
};

#endif
