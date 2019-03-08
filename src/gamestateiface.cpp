#include "gamestateiface.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include "json.h"


std::mutex m;

void gsi_thread()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m);
        gsi::get().gsi_update.wait(lock); // wait until notified to actually write out data
        // TODO: Add condition to CV to avoid unnecessary writeouts?

        format_json();
    }
}


void format_json()
{

}
