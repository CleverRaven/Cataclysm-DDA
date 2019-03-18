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
#include "weather.h"
#include "inventory.h"
#include "color.h"

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

std::mutex m;
std::mutex otherMutex;
std::mutex writeout;
bool outofdate = false;

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

void gsi::serialize(JsonOut &jsout) const
{
    jsout.start_object();
    jsout.member("provider");
    jsout.start_object();
    jsout.member("name", "cataclysm");
    jsout.member("appid", -1);
    jsout.end_object();

    jsout.member("keybinds");
    jsout.start_object();
    jsout.member("input_context", ctxt.top());
    jsout.member("menu_context", mctxt.top());
    jsout.member("invlets", invlets);
    jsout.member("invlets_color", invlets_c);
    jsout.member("invlets_status", invlets_s);
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
        if(!outofdate)
            fprintf(fp, "%s", ostream.str().c_str());
        fclose(fp);
    }
}

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

void gsi_socket::sockListen()
{
    sockInit();
    struct sockaddr_in address;
    ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(ListenSocket, FIONBIO, &mode);
#else
    fcntl(ListenSocket, F_SETFL, O_NONBLOCK);
#endif
    address.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &(address.sin_addr));
    address.sin_port = htons(3441);
    if (ListenSocket == INVALID_SOCKET)
        return;
    if (bind(ListenSocket, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        closesocket(ListenSocket);
        return;
    }
}

void gsi_socket::tryReceive() 
{
    if (ListenSocket != INVALID_SOCKET)
    {
        const int buflen = 1024;
        char buf[buflen];
        SOCKADDR sender;
        int sender_len = sizeof(sender);
        while (recvfrom(ListenSocket, buf, buflen, 0, &sender, &sender_len) > 0)
        {
            commandqueue.push(std::string(buf));
        }
    }
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
                ports.insert(std::stoi(command[1]));
        }
        catch (int e)
        {

        }
    }
}

int gsi_socket::sockInit(void)
{
#ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
    return 0;
#endif
}

int gsi_socket::sockQuit(void)
{
#ifdef _WIN32
    return WSACleanup();
#else
    return 0;
#endif
}

void gsi_socket::sockout()
{
    processCommands();
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
            _close(sock);
            sockQuit();
            return;
        }
        std::ostringstream ostream(std::ostringstream::ate);
        JsonOut jout(ostream, true);
        gsi::get().serialize(jout);

        std::stringstream ss;
        ss.imbue(std::locale::classic());
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
    
    return;
}




