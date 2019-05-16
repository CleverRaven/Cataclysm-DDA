#ifndef GSI_H
#define GSI_H

#include <vector>
#include <string>
#include <stack>
#include <condition_variable>
#include <string>
#include <list>
#include <mutex>
#include <queue>

#include "json.h"
#include "color.h"
#include "pldata.h"
#include "bodypart.h"
#include "character.h"
#include "game.h"
#include "input.h"

#ifdef _WINDOWS
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

/**
 * Holds and formats game data for eventual output and reading by other applications.
 *
 * This class contains functions for updating its internal info at appropriate times, and
 * is intended to obfuscate info where appropriate to the same degree enforced by in-game 
 * UI elements.
 */
class gsi : public JsonSerializer
{
public:

    static gsi& get()
    {
        static gsi instance;
        return instance;
    }

    gsi(gsi const&) = delete;
    void operator=(gsi const&) = delete;


  /**
   * Update game parameters that are expected to change only by the end of a turn.
   * 
   * This is intended as a catch-all for updating all values where:
   *  - the value can only change during turn processing
   *  - getting the value during turn processing will catch most cases, and the changes 
   *    necessary to handle special cases do not or cannot handle general cases.
   *
   * Handling as many cases of as many value changes in this function is preferable, 
   * as it would result in less socket calls.
   *
   * It is preferable that as many values as possible are handled within self-contained 
   * functions in order to make it easier to make the function public should that become
   * necessary in the future.
   *
   * @return whether a socket call should be performed after this function completes
   */
    bool update_turn();
    void update_safemode(bool forceupdate);

  /**
   * Update the list of currently bound actions to be passed to interested applications.
   *
   * This is intended to be called at a point within an input_context function where 
   * @ref input_context::registered_actions can be relied upon to match what inputs 
   * are currently valid.  It should be called whenever this changes, and ideally only 
   * when it changes.
   *
   * @param registered_actions registered_actions member from @ref input_context
   * @param category category member from @ref input_context
   * @return true if a socket call is necessary for update
   */
    bool update_input(std::vector<std::string> registered_actions, std::string category);

  /**
   * Update attributes relevant to the advanced inventory UI.
   *
   *
   *
   *
   */
    bool update_ui_adv_inventory();
    
    // Valid (not red) focuses.  [1]-[9], then [C]ontainer and [D]ragged
    std::vector<bool> advinv_isvalid;
    // Whether a given focus has a <V>ehicle tile. [1]-[9].
    std::vector<int> advinv_isvehicle;
  /**
   * Updates attributes from choose adjacent prompts.
   *
   *
   */
    bool update_choose_adjacent(std::vector<bool> points);

    // In order: 1-9, <, >
    std::vector<bool> adjacents;

  /**
   * Output the current game state information in JSON format.
   *
   * The format for the JSON output is what is used by Aurora, but the output can easily
   * be read by any application.  The output is organized into an arbitrary number of top
   * level objects known as "nodes", each with multiple members of whatever data type is 
   * appropriate.
   *
   * The "provider" node is specific to Aurora, but can again be used by any application to 
   * distinguish between different games, or ignored.
   *
   * It is the responsibility of the receiving application to handle cases where there are
   * more members than expected, if a specific member is missing, or if a specific member is
   * out of expected range.  As systems get reworked, it is reasonable to assume that several
   * tracked parameters will change over time. (TODO: Consider adding a version number to
   * JSON output?)  Changing the valid range of values for a variable should be avoided unless 
   * it is due to more detailed info being available to the player.
   * 
   * Performing operations on variables in this function should be avoided.  Variables should
   * be formatted when they are updated.

   * @param [inout] jsout JsonOut stream
   */
    void serialize(JsonOut &jsout) const;

private:
    // Blank constructor
    gsi() { }

    bool is_self_aware;
    std::string ctxt; // current input context
    int power_level, max_power_level;
    int morale;
    int safe_mode;
    std::vector<std::string> bound_actions;
    std::vector<std::vector<std::string>> bound_keys;

    /**
     * Update the parameters for hunger, thirst, and fatigue.
     *
     * This function's output should reflect what is shown in the sidebar.  Currently, this
     * is just the string shown in the sidebar.
     *
     * @see {hunger_level, thirst_level, fatigue_level}
     */
    void update_needs();
    std::string hunger_level, thirst_level, fatigue_level;

    /**
     * Update the parameter for stamina.
     * 
     * This function's output should reflect what is shown in the sidebar.  Currently, this
     * means an integer between 0 and 10 representing the drawn stamina bar.
     * 
     * @see {stamina, stamina_max}
     */
    void update_stamina();
    int stamina, stamina_max;

    /**
     * Update parameters for body part health and limb status.
     *
     * This function respects self-awareness, as health values are displayed at higher precision
     * for characters with self-awareness.
     */
    void update_body();
    
    std::array<int, num_hp_parts> hp_cur_level, hp_max_level;
    std::array<int, num_hp_parts> limb_state;
    std::array<float, num_hp_parts> splint_state;

    /**
     * Update parameters for player's current body temperature.
     *
     * This function updates a value for current temperature (Cold, Warm, Comfortable, etc) and for 
     * temperature change (Rising, Rising!!, Falling, etc.), as displayed on the sidebar.  Ambient 
     * temperature is not touched here.
     */
    void update_temp();
    int temp_level, temp_change;

    /**
     * Update parameters for invlets for the player's inventory.
     *
     *
     */
    void update_invlets();
    std::vector<std::string> invlets;        // inventory letters in use
    std::vector<std::string> invlets_c;  // inventory letters' corresponding colors
    std::vector<std::string> invlets_s;  // Special status of inventory item:

    /**
     * Update parameters for light level, ambient temperature, and weather.
     *
     *
     */
    void update_weather();
    std::string light_level;
    std::string weather;
    int temp;

    /**
     * Update parameters for pain.
     */
    void update_pain();
    int pain;
    std::string pain_string;
};


/**
 * Controls socket interactions for Cataclysm.
 *
 * The IPC framework as it exists currently has two components:
 *  - a TCP socket, running on port 3441, which is processed asynchronously and is for
 *    input of commands.
 *  - UDP sockets that are created upon request, and send a datagram with an HTTP POST
 *    request on a specified port with JSON data.
 */
class gsi_socket
{
public:
    static gsi_socket& get()
    {
        static gsi_socket instance;
        return instance;
    }

    gsi_socket(gsi_socket const&) = delete;
    void operator=(gsi_socket const&) = delete;

    std::queue<std::string> commandqueue;
    void sockListen();
    void processCommands();
    void sockout();

private:
    gsi_socket() {}
    bool initialized = false;

    SOCKET ListenSocket;
    struct sockaddr_in listenaddress;
    std::set<int> ports;
    std::vector<SOCKET> sockets;
    int nfds;
    void tryReceive();
    void sockInit();
    void sockQuit();
};

#endif
