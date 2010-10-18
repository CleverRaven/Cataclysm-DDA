#ifndef _COMPUTER_H_
#define _COMPUTER_H_

//#include "game.h"
#include <vector>
#include <string>

struct computerk;
class game;

struct computer_option
{
 std::string name;
 bool (computerk::*action)(game *, int);

 computer_option(std::string N, bool (computerk::*A)(game *, int)) :
  name (N), action (A) {}
};

struct computerk
{
 int difficulty;
 std::vector<computer_option> options;
 void (computerk::*failure)(game *);

 void add_opt(std::string s, bool (computerk::*a)(game *, int)) {
  options.push_back(computer_option(s, a));
 };

 std::vector<std::string> list_opts() {
  std::vector<std::string> ret;
  for (int i = 0; i < options.size(); i++)
   ret.push_back(options[i].name);
  return ret;
 };

// Failure functions
 void spawn_manhacks	(game *);
 void spawn_secubots	(game *);
// Option functions - return false if the computer is to be broken
 bool release	(game *, int);	// Remove nearby glass walls
 bool terminate (game *, int);	// Kill nearby monsters
 bool portal	(game *, int);	// Create a portal between nearby radio towers
 bool cascade	(game *, int);	// Resonance cascade
 bool research	(game *, int);	// Display lines from LAB_NOTES
 bool maps	(game *, int);	// Reveal the world map
 bool launch	(game *, int);	// Launch a missile
 bool disarm	(game *, int);	// Disarm this missile
};

#endif
