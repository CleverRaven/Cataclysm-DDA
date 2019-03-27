#include "gamestateiface.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <sstream>

#include <functional>
#include <iostream>

#include "json.h"
#include "cata_utility.h"
#include "character.h"
#include "player.h"
#include "weather.h"
#include "inventory.h"
#include "color.h"
#include "output.h"
#include "effect.h"
#include "options.h"
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
#include <sys/time.h>
#endif

#define LISTEN_PORT 3441

bool gsi::update_turn()
{
#ifdef GSI
    // Player Stats
    // MUST update first -- others will decide to hide info based on the trait
    is_self_aware = g->u.has_trait((trait_id)"SELFAWARE");
    
    update_hunger(g->u.get_hunger(), g->u.get_starvation());
    update_thirst(g->u.get_thirst());
    update_fatigue(g->u.get_fatigue());

    static std::array<body_part, 6> part = { {
            bp_head, bp_torso, bp_arm_l, bp_arm_r, bp_leg_l, bp_leg_r
        }
    };
    std::array<nc_color, num_hp_parts> bp_status = { c_black, c_black, c_black, c_black, c_black, c_black };
    std::array<float, num_hp_parts> splints = { 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < part.size(); i++)
    {
        bp_status[i] = g->u.limb_color(part[i], true, true, true);
    }

    for (int i = 0; i < part.size(); i++)
    {
        if (!(g->u.worn_with_flag("SPLINT", part[i])))
            splints[i] = -1;
        else
        {
            static const efftype_id effect_mending("mending");
            const auto &eff = g->u.get_effect(effect_mending, part[i]);
            splints[i] = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();
        }
    }
    update_body(g->u.hp_cur, g->u.hp_max, bp_status, splints);
    update_temp(g->u.temp_cur, g->u.temp_conv);
    update_invlets(g->u);
    
    stamina = g->u.stamina;
    stamina_max = g->u.get_stamina_max();
    power_level = g->u.power_level;
    max_power_level = g->u.max_power_level;
    pain = g->u.get_perceived_pain();
    morale = g->u.get_morale_level();

    if (g->safe_mode == SAFE_MODE_ON)
        safe_mode = 4;
    else
        safe_mode = g->turnssincelastmon * 4 / get_option<int>("AUTOSAFEMODETURNS");

    // Environment Stats
    update_light(g->u);
    if (g->get_levz() < 0)
        weather = "Underground";
    else
        weather = weather_data(g->weather).name.c_str();

    if (g->u.has_item_with_flag("THERMOMETER") ||
        g->u.has_bionic(bionic_id("bio_meteorologist")))
        temp = g->get_temperature(g->u.pos());
    else
        temp = -500;

    gsi_socket::get().sockout();
    return true;
#endif
    return false;
}

bool gsi::update_safemode(safe_mode_type _safe_mode, int turnssincelastmon)
{
#ifdef GSI
    if (_safe_mode == SAFE_MODE_ON)
        safe_mode = 4;
    else
        safe_mode = turnssincelastmon * 4 / get_option<int>("AUTOSAFEMODETURNS");
    return true;
#endif
    return false;
}

bool gsi::update_input(std::vector<std::string> registered_actions, std::string category)
{
#ifdef GSI

    // TODO: Try to return false if things haven't changed.
    // Maybe see if the last input was a timeout?
    std::vector<std::string> _bound_actions;
    std::vector<std::vector<std::string>> _bound_keys;
    std::for_each(registered_actions.begin(), registered_actions.end(), [category, &_bound_actions, &_bound_keys](std::string action)
    {

        std::vector<std::string> binds;
        const std::vector<input_event> &events = inp_mngr.get_input_for_action(action,
            category);
        for (const auto &events_event : events) {
            // Ignore multi-key input and non-keyboard input
            // TODO: fix for Unicode.
            if (events_event.type == CATA_INPUT_KEYBOARD &&
                events_event.sequence.front() < 0xFF && isprint(events_event.sequence.front())) {
                binds.push_back(inp_mngr.get_keyname(events_event.sequence.front(), CATA_INPUT_KEYBOARD));
            }
        }
        if (binds.size() != 0)
        {
            _bound_actions.push_back(action);
            _bound_keys.push_back(binds);
        }
    }
    );
    bound_actions = _bound_actions;
    bound_keys = _bound_keys;
    ctxt = category;
    return true;
#endif
    return false;
}

void gsi::update_hunger(int hunger, int starvation)
{
    if (hunger >= 300 && starvation > 2500)
        hunger_level = -5;
    else if (hunger >= 300 && starvation > 1100)
        hunger_level = -4;
    else if (hunger > 250)
        hunger_level = -3;
    else if (hunger > 100)
        hunger_level = -2;
    else if (hunger > 40)
        hunger_level = -1;
    else if (hunger < -60)
        hunger_level = 3;
    else if (hunger < -20)
        hunger_level = 2;
    else if (hunger < 0)
        hunger_level = 1;
    else
        hunger_level = 0;
}

void gsi::update_thirst(int thirst)
{
    if (thirst > 520)
        thirst_level = -4;
    else if (thirst > 240)
        thirst_level = -3;
    else if (thirst > 80)
        thirst_level = -2;
    else if (thirst > 40)
        thirst_level = -1;
    else if (thirst < -60)
        thirst_level = 3;
    else if (thirst < -20)
        thirst_level = 2;
    else if (thirst < 0)
        thirst_level = 1;
    else
        thirst_level = 0;
}

void gsi::update_fatigue(int fatigue)
{
    if (fatigue > EXHAUSTED)
        fatigue_level = -3;
    else if (fatigue > DEAD_TIRED)
        fatigue_level = -2;
    else if (fatigue > TIRED)
        fatigue_level = -1;
    else
        fatigue_level = 0;
}

void gsi::update_body(std::array<int, num_hp_parts> hp_cur, std::array<int, num_hp_parts> hp_max, std::array<nc_color, num_hp_parts> bp_status, std::array<float, num_hp_parts> splints)
{
    for (int i = 0; i < num_hp_parts; i++)
    {
        if (is_self_aware) // Don't obfuscate data if player is self-aware.
        {
            hp_cur_level[i] = hp_cur[i];
            hp_max_level[i] = hp_max[i];
            splint_state[i] = splints[i];
        }
        else // If player isn't self aware, obfuscate data to match sidebar's detail.
        {
            double hp_ratio = static_cast<double>(hp_cur[i]) / (hp_max[i] ? hp_max[i] : 1);
            hp_cur_level[i] =
                (hp_ratio >= 1.0) ? 10 :
                (hp_ratio >= 0.9) ? 9 :
                (hp_ratio >= 0.8) ? 8 :
                (hp_ratio >= 0.7) ? 7 :
                (hp_ratio >= 0.6) ? 6 :
                (hp_ratio >= 0.5) ? 5 :
                (hp_ratio >= 0.4) ? 4 :
                (hp_ratio >= 0.3) ? 3 :
                (hp_ratio >= 0.2) ? 2 :
                (hp_ratio >= 0.1) ? 1 : 0;
            hp_max_level[i] = 10;
            if (splints[i] >= 0)
                splint_state[i] = splints[i] * 5 / 100;
        }
        if (hp_cur[i] == 0 && splints[i] == -1)
            splint_state[i] = -100;
        
        if (hp_cur[i] == 0) // limb broken, must make sure this applies first
            limb_state[i] = -1;
        else if (bp_status[i] == c_red) // bleeding
            limb_state[i] = 1;
        else if (bp_status[i] == c_blue) // bitten
            limb_state[i] = 2;
        else if (bp_status[i] == c_green) // infected
            limb_state[i] = 4;
        else if (bp_status[i] == c_magenta)// bleeding + bitten
            limb_state[i] = 3;
        else if (bp_status[i] == c_yellow) // bleeding + infected
            limb_state[i] = 5;
        else
            limb_state[i] = 0;
    }
}

int define_temp_level_gsi(const int lvl)
{
    if (lvl > BODYTEMP_SCORCHING) {
        return 7;
    }
    else if (lvl > BODYTEMP_VERY_HOT) {
        return 6;
    }
    else if (lvl > BODYTEMP_HOT) {
        return 5;
    }
    else if (lvl > BODYTEMP_COLD) {
        return 4;
    }
    else if (lvl > BODYTEMP_VERY_COLD) {
        return 3;
    }
    else if (lvl > BODYTEMP_FREEZING) {
        return 2;
    }
    return 1;
}

void gsi::update_temp(std::array<int, num_bp> temp_cur, std::array<int, num_bp> temp_conv)
{
    /// Find hottest/coldest bodypart
    // Calculate the most extreme body temperatures
    int current_bp_extreme = 0;
    int conv_bp_extreme = 0;
    for (int i = 0; i < num_bp; i++) {
        if (abs(temp_cur[i] - BODYTEMP_NORM) > abs(temp_cur[current_bp_extreme] - BODYTEMP_NORM))
            current_bp_extreme = i;
        if (abs(temp_conv[i] - BODYTEMP_NORM) > abs(temp_conv[conv_bp_extreme] - BODYTEMP_NORM))
            conv_bp_extreme = i;
    }

    // Assign zones for comparisons
    const int cur_zone = define_temp_level_gsi(temp_cur[current_bp_extreme]);
    const int conv_zone = define_temp_level_gsi(temp_conv[conv_bp_extreme]);

    // delta will be positive if temp_cur is rising
    int delta = conv_zone - cur_zone;
    if (delta > 2)
        delta = 3;
    if (delta < -2)
        delta = -3;
    temp_change = delta;

    // printCur the hottest/coldest bodypart, and if it is rising or falling in temperature
    if (temp_cur[current_bp_extreme] > BODYTEMP_SCORCHING)
        temp_level = 3;
    else if (temp_cur[current_bp_extreme] > BODYTEMP_VERY_HOT)
        temp_level = 2;
    else if (temp_cur[current_bp_extreme] > BODYTEMP_HOT)
        temp_level = 1;
    else if (temp_cur[current_bp_extreme] >
        BODYTEMP_COLD) // If you're warmer than cold, you are comfortable
        temp_level = 0;
    else if (temp_cur[current_bp_extreme] > BODYTEMP_VERY_COLD)
        temp_level = -1;
    else if (temp_cur[current_bp_extreme] > BODYTEMP_FREEZING)
        temp_level = -2;
    else if (temp_cur[current_bp_extreme] <= BODYTEMP_FREEZING)
        temp_level = -3;
}

void gsi::update_invlets(Character & character)
{
    invlets.clear();
    invlets_c.clear();
    invlets_s.clear();

    std::set<char> invlets_temp = character.allocated_invlets();
    std::vector<char> invlets_i;
    std::copy(invlets_temp.begin(), invlets_temp.end(), std::back_inserter(invlets_i));
    for (int i = 0; i < invlets_temp.size(); i++)
    {
        item t; 
        if (character.weapon.invlet == invlets_i[i])
            t = character.weapon;
        else if (character.inv.invlet_to_position(invlets_i[i]) == INT_MIN) // if not weapon and it isnt in inventory it is worn
            t = *std::find_if(character.worn.begin(), character.worn.end(), [invlets_i, i](item k) { return k.invlet == invlets_i[i]; });
        else
            t = character.inv.find_item(character.inv.invlet_to_position(invlets_i[i]));
        invlets_c.push_back(get_all_colors().get_name(t.color()));
        invlets_s.push_back(get_all_colors().get_name(t.color_in_inventory()));
        invlets.push_back(std::string(1, invlets_i[i]));
    }
}

void gsi::update_light(player & player)
{
    light_level = get_all_colors().get_name(get_light_level(player.fine_detail_vision_mod()).second);
}

void gsi::serialize(JsonOut &jsout) const
{
    jsout.start_object();
    jsout.member("provider"); // Provider Node -- provides information for Aurora mainly.
    jsout.start_object();
    jsout.member("name", "cataclysm");
    jsout.member("appid", -1); // supposed to be a Steam app ID, with -1 denoting non steam -- should that ever happen update this
    jsout.end_object();

    jsout.member("keybinds");
    jsout.start_object();
    jsout.member("input_context", ctxt);
    jsout.member("invlets", invlets);
    jsout.member("invlets_color", invlets_c);
    jsout.member("invlets_status", invlets_s);
    jsout.member("actions", bound_actions);
    jsout.member("binds", bound_keys);
    jsout.end_object();

    jsout.member("player");
    jsout.start_object();
    jsout.member("is_self_aware", is_self_aware);
    jsout.member("hunger", hunger_level);
    jsout.member("thirst", thirst_level);
    jsout.member("fatigue", fatigue_level);
    jsout.member("temp_level", temp_level);
    jsout.member("temp_change", temp_change);
    jsout.member("hp_cur", hp_cur_level);
    jsout.member("hp_max", hp_max_level);
    jsout.member("splints", splint_state);
    jsout.member("limbs", limb_state);

    jsout.member("stamina", stamina);
    jsout.member("stamina_max", stamina_max);
    jsout.member("power_level", power_level);
    jsout.member("max_power_level", max_power_level);
    jsout.member("pain", pain);
    jsout.member("morale", morale);
    jsout.member("safe_mode", safe_mode);
    jsout.end_object();

    jsout.member("world");
    jsout.start_object();
    jsout.member("temperature", temp);
    jsout.member("weather", weather);
    jsout.member("light_level", light_level);
    jsout.end_object();
    //jsout.start_array();
    //jsout.write(x);
    //jsout.write(y);
    //jsout.end_array();

    jsout.end_object();
}

void gsi_socket::sockListen()
{
    sockInit();
    

    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    bool option = true;
    setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &option, 1);

#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(ListenSocket, FIONBIO, &mode);
#else
    fcntl(ListenSocket, F_SETFL, O_NONBLOCK);
#endif
    listenaddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(listenaddress.sin_addr));
    listenaddress.sin_port = htons(LISTEN_PORT);
    if (ListenSocket == INVALID_SOCKET)
        return;
    if (bind(ListenSocket, (struct sockaddr *)&listenaddress, sizeof(listenaddress)) == SOCKET_ERROR)
    {
        closesocket(ListenSocket);
        return;
    }
    int ret = listen(ListenSocket, 5);
    int error = WSAGetLastError();
    initialized = TRUE;
}

void gsi_socket::tryReceive() 
{
    if (ListenSocket != INVALID_SOCKET)
    {
        fd_set fdset;
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

        FD_ZERO(&fdset);
        FD_SET(ListenSocket, &fdset);
        nfds = ListenSocket;
        for (int i = 0; i < sockets.size(); i++)
        {
            FD_SET(sockets[i], &fdset);
            if (sockets[i] > nfds)
                nfds = sockets[i];
        }

        if (select(nfds + 1, &fdset, NULL, NULL, &timeout) > 0)
        {
            // process new connection attempt
            if (FD_ISSET(ListenSocket, &fdset))
            {
                SOCKET new_client;
                new_client = accept(ListenSocket, NULL, NULL);
                sockets.push_back(new_client);
            }

            for (int i = 0; i < sockets.size(); i++)
            {
                SOCKET s = sockets[i];
                const int buflen = 1024;
                char buf[buflen];

                if (FD_ISSET(s, &fdset))
                {
                    // read returning 0 indicates socket is closed
                    if (recv(s, buf, buflen, NULL) == 0)
                    {
                        closesocket(s);
                        sockets.erase(sockets.begin() + i);
                    }
                    // add all commands to queue
                    else
                    {
                        std::vector<std::string> command;
                        std::stringstream ss(buf);
                        std::string substring;
                        while (std::getline(ss, substring, ';'))
                            commandqueue.push(substring);
                    }
                }
            }
        }
    }
    int error = WSAGetLastError();
}

void gsi_socket::processCommands()
{
    tryReceive();
    while (!commandqueue.empty())
    {
        try
        {
            std::vector<std::string> command;
            std::stringstream ss(commandqueue.front());
            std::string substring;
            while (std::getline(ss, substring, ' '))
                command.push_back(substring);
            commandqueue.pop();

            if (command[0] == "gsi")
                if(std::stoi(command[1]) >= 1024 && std::stoi(command[1]) <= 49151 )
                ports.insert(std::stoi(command[1]));
        }
        catch (int e)
        {

        }
    }
}

void gsi_socket::sockInit()
{
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(1, 1), &wsa_data);
#endif
    return;
}

void gsi_socket::sockQuit()
{
#ifdef _WIN32
    WSACleanup();
#endif
    return;
}

void gsi_socket::sockout()
{
    try {

        if(!initialized)
            sockListen();

        processCommands();
        if (ports.size() != 0)
        {
            std::vector<int> v;
            std::copy(ports.begin(), ports.end(), std::back_inserter(v));
            for (int i = 0; i < v.size(); i++)
            {
                int sock;
                sockInit();
                sock = socket(AF_INET, SOCK_STREAM, 0);
                int error = WSAGetLastError();
                struct sockaddr_in address;
                address.sin_family = AF_INET;
                inet_pton(AF_INET, "127.0.0.1", &(address.sin_addr));
                address.sin_port = htons(v[i]);

                if (sock == INVALID_SOCKET)
                {
                    sockQuit();
                    return;
                }

                if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
                    closesocket(sock);
                    sockQuit();
                    return;
                }
                std::ostringstream ostream(std::ostringstream::ate);
                JsonOut jout(ostream, true);
                gsi::get().serialize(jout);

                std::stringstream ss;
                ss.imbue(std::locale::classic()); // this prevents STL from formatting numbers
                ss << "POST http://localhost HTTP/1.1\r\n"
                    << "Host: http://localhost\r\n"
                    << "Content-Type: application/json\r\n"
                    << "Content-Length: " << ostream.str().length() << "\r\n\r\n"
                    << ostream.str();

                std::string request = ss.str();
                if (send(sock, request.c_str(), request.length(), 0) != (int)request.length())
                {
                    sockQuit();
                    return;
                }

                char buf[1024];
                std::cout << recv(sock, buf, 1024, 0) << std::endl;
                std::cout << buf << std::endl;

                sockQuit();
            }
        }
    }
    catch(...)
    {

    }
    
    return;
}




