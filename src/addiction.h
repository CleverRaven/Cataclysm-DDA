#ifndef ADDICTION_H
#define ADDICTION_H

#include <string>
#include <functional>

class addiction;
class player;
enum add_type : int;
enum morale_type : int;

constexpr int MIN_ADDICTION_LEVEL = 3; // Minimum intensity before effects are seen

// cancel_activity is called when the addication effect wants to interrupt the player
// with an optional pre-translated message.
void addict_effect(player &u, addiction &add,
                   std::function<void (char const*)> const &cancel_activity);

std::string addiction_type_name(add_type cur);

std::string addiction_name(addiction const &cur);

morale_type addiction_craving(add_type cur);

add_type addiction_type(std::string const &name);

std::string addiction_text(player const &u, addiction const &cur);

#endif
