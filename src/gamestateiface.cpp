#include "gamestateiface.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <sstream>

#include "json.h"
#include "cata_utility.h"
#include <functional>
#include <iostream>

#ifdef _WINDOWS
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#endif

std::mutex m;

void gsi::serialize(JsonOut &jsout) const
{
    jsout.start_object();
    jsout.member("provdier");
    jsout.start_object();
    jsout.member("name", "cataclysm");
    jsout.member("appid", -1);
    jsout.end_object();

    jsout.member("keybinds");
    jsout.start_object();
//    jsout.member("input_context", context_stack.front);
    jsout.member("input_context", mctxt.top());
    jsout.member("menu_context", mctxt.top());
    jsout.member("localization");
    jsout.start_object();
//    jsout.member("main_world", vWorldHotkeys);
//    jsout.member("main_settings", vSettingsHotkeys);
//    jsout.member("main_menu", vMenuHotkeys);
//    jsout.member("main_newgame", vNewGameHotkeys);
    jsout.end_object();
    jsout.end_object();

    //jsout.start_array();
    //jsout.write(x);
    //jsout.write(y);
    //jsout.end_array();

    jsout.end_object();
}

void gsi::write_out(std::string json)
{
    FILE* fp = NULL;
#ifdef _WINDOWS
    HANDLE gsiHandle = CreateFileA( // Create a memory-mapped file.  This will not use the disk wherever possible.
        "temp\\gamestate.json",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (gsiHandle != INVALID_HANDLE_VALUE)
    {
        int file_descriptor = _open_osfhandle((intptr_t)gsiHandle, _O_TEXT);
        if (file_descriptor != 1)
            fp = _fdopen(file_descriptor, "w");
        else
            _close(file_descriptor);
    }
    else
        CloseHandle(gsiHandle);
#endif
    if (fp != NULL)
    {
        fprintf(fp, "%s", json.c_str());
        fclose(fp);
    }

#ifdef _WINDOWS
//    CloseHandle(gsiHandle);
#endif
}

void gsi_thread::worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m);
        gsi::get().gsi_update.wait(lock); // wait until notified to actually write out data
        // TODO: Add condition to CV to avoid unnecessary writeouts?
        //std::function <void(JsonOut &)> test = &gsi::get().serialize;
        std::ostringstream ostream;
        JsonOut jout(ostream, true);
        gsi::get().serialize(jout);
        gsi::get().write_out(ostream.str());
    }
}





