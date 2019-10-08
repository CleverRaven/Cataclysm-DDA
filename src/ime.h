#pragma once
#ifndef IME_H
#define IME_H

/**
 * Enable or disable IME for text/keyboard input
 */
void enable_ime();
void disable_ime();

/**
 * used before text input to change IME mode and auto-restore IME mode when leaving the scope
 */
class ime_sentry
{
    public:
        enum mode {
            enable = 0,
            disable = 1,
            keep = 2,
        };

        ime_sentry( mode m = enable );
        ~ime_sentry();
    private:
        bool previously_enabled;
};

#endif
