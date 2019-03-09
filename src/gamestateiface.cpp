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
std::mutex otherMutex;
std::mutex writeout;

void gsi::serialize(JsonOut &jsout) const
{
    jsout.start_object();
    //jsout.member("provdier");
    //jsout.start_object();
    //jsout.member("name", "cataclysm");
    //jsout.member("appid", -1);
    //jsout.end_object();

    jsout.member("keybinds");
    jsout.start_object();
//    jsout.member("input_context", context_stack.front);
    jsout.member("input_context", ctxt.top());
    jsout.member("menu_context", mctxt.top());
    //jsout.member("localization");
    //jsout.start_object();
//    jsout.member("main_world", vWorldHotkeys);
//    jsout.member("main_settings", vSettingsHotkeys);
//    jsout.member("main_menu", vMenuHotkeys);
//    jsout.member("main_newgame", vNewGameHotkeys);
    //jsout.end_object();
    jsout.end_object();

    //jsout.start_array();
    //jsout.write(x);
    //jsout.write(y);
    //jsout.end_array();

    jsout.end_object();
}

void gsi::write_out()
{
    FILE* fp = NULL;
#ifdef _WINDOWS
    HANDLE gsiHandle = CreateFileA( // Create a memory-mapped file.  This will not use the disk wherever possible.
        "temp\\gamestate.json",
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (gsiHandle != INVALID_HANDLE_VALUE)
    {
        int file_descriptor = _open_osfhandle((intptr_t)gsiHandle, _O_TEXT);
        if (file_descriptor != -1)
            fp = _fdopen(file_descriptor, "w");
        else
            _close(file_descriptor);
    }
    else
        CloseHandle(gsiHandle);
#endif
    if (fp != NULL)
    {
        std::ostringstream ostream(std::ostringstream::ate);
        JsonOut jout(ostream, true);
        gsi::get().serialize(jout);
        fprintf(fp, "%s", ostream.str().c_str());
        fclose(fp);
    }

#ifdef _WINDOWS
//    CloseHandle(gsiHandle);
#endif
}

bool outofdate = false;

void gsi_thread::worker()
{
    std::thread writeout_thread(&gsi_thread::prep_out);
    while (true)
    {
        std::unique_lock<std::mutex> lock(m);
        gsi::get().gsi_update.wait(lock); // wait until notified to actually write out data
        // TODO: Add condition to CV to avoid unnecessary writeouts?
        outofdate = true;
        gsi::get().gsi_writer.notify_one();
    }
}

void gsi_thread::prep_out()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(otherMutex);
        if(!outofdate)
            gsi::get().gsi_writer.wait(lock);
        outofdate = false;
        gsi::get().write_out();
    }
}





