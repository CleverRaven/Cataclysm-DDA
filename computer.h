#ifndef _COMPUTER_H_
#define _COMPUTER_H_

#include "texthash.h"
#include "output.h"
#include <vector>
#include <string>

#define DEFAULT_COMPUTER_NAME ""

class game;
class player;

enum computer_action
{
 COMPACT_NULL = 0,
 COMPACT_OPEN,
 COMPACT_RELEASE,
 COMPACT_TERMINATE,
 COMPACT_PORTAL,
 COMPACT_CASCADE,
 COMPACT_RESEARCH,
 COMPACT_MAPS,
 COMPACT_MISS_LAUNCH,
 COMPACT_MISS_DISARM,
 NUM_COMPUTER_ACTIONS
};

enum computer_failure
{
 COMPFAIL_NULL = 0,
 COMPFAIL_SHUTDOWN,
 COMPFAIL_ALARM,
 COMPFAIL_MANHACKS,
 COMPFAIL_SECUBOTS,
 COMPFAIL_DAMAGE,
 NUM_COMPUTER_FAILURES
};

struct computer_option
{
 std::string name;
 computer_action action;
 int security;

 computer_option() { name = "Unknown", action = COMPACT_NULL, security = 0; };
 computer_option(std::string N, computer_action A, int S) :
   name (N), action (A), security (S) {};
};

class computer
{
public:
 computer();
 computer(std::string Name, int Security);
 ~computer();

 computer & operator=(const computer &rhs);
// Initialization
 void set_security(int Security);
 void add_option(std::string opt_name, computer_action action, int Security);
 void add_failure(computer_failure failure);
// Basic usage
 void shutdown_terminal(); // Shutdown (free w_terminal, etc)
 void use(game *g);
 bool hack_attempt(player *p, int Security = -1);// -1 defaults to main security
// Save/load
 std::string save_data();
 void load_data(std::string data);

 std::string name; // "Jon's Computer", "Lab 6E77-B Terminal Omega"

private:
 int security; // Difficulty of simply logging in
 std::vector<computer_option> options;   // Things we can do
 std::vector<computer_failure> failures; // Things that happen if we fail a hack
 WINDOW *w_terminal; // Output window

// Called by use()
 void activate_function      (game *g, computer_action action);
// Generally called when we fail a hack attempt
 void activate_random_failure(game *g);
// ...but we can also choose a specific failure.
 void activate_failure       (game *g, computer_failure fail);

// OUTPUT/INPUT
// Reset to a blank terminal (e.g. at start of usage loop)
 void reset_terminal();
// Prints a line to the terminal (with printf flags)
 void print_line(const char *text, ...);
// For now, the same as print_line but in red (TODO: change this?)
 void print_error(const char *text, ...);
// Prints a line and waits for Y/N/Q
 char query_ynq(const char *text, ...);
};

#endif
