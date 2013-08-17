##0 Table of Contents

* 0 Table of Contents
* 1 Files
* 2 special_game class
* 3 Functions
	* 3.0 id()
	* 3.1 init()
	* 3.2 per_turn()
	* 3.3 pre_action()
	* 3.4 post_action()
	* 3.5 game_over()

##1 Files
The main file to be concerned with is gamemode.h.

This is where you can insert the class definition for your game mode, and any
enums it might use.

gamemode.cpp is also important, if short.  It contains two functions:
	special_game_name() returns the name of each special game.
	get_special_game() returns a pointer to a special game of the proper
	type.

Also of note are tutorial.h (houses text snippets for tutorials), and
tutorial.cpp and defense.cpp, which house code for tutorials and defense games
respectively.

If you add a new game mode, use <mode name>.cpp for its code and <mode name>.h
for any static data associated with it.

##2 special_game class
Every new game mode is a child class of special_game.  It may house any private
functions and variables it needs, as well as a constructor, but otherwise the
only public members should be the five functions described below.

The game class holds a pointer to a special_game.  The five functions are
called as is appropriate, and thus are cast to the proper function in a child
class.

There is an enum, special_game_id, at the top of gamemode.h.  There should be
one entry for every game mode in existance.  Add one for yours.

##3 Functions
###3.0 id()

virtual special_game_id id();

Returns the special_game_id assigned to your game mode.

###3.1 init()

virtual void init(game *g);

This is run when your game mode begins.

>Some ideas:
>Change the itype and mtype definitions--remove certain monsters from
  the game, make certain items weightless, make baseballs bats uber-powerful
>Change the recipes.  Add new ones, reduce the time crafting takes.
>Start the game in a special area.  Edit the world to be entirely swamp.

###3.2 per_turn()

virtual void per_turn(game *g);

This is run every turn, before anything else is done, including player actions.

>Some ideas:
>Reset the player's thirst, hunger, or fatigue to 0, eliminating these needs
>Spawn some monsters every 50 turns
>Make fire burn off faster
>Add a chance for a space debris to fall

###3.3 pre_action()

virtual void pre_action(game *g, action_id &act);

This is run after a keypress, but before the action is performed.  The action
is passed by reference, so you can modify it if you like.

>Some ideas:
>Rewrite the functionality of actions entirely; change the long wait menu,
  for example
>Disallow certain actions by setting act to ACTION_NULL
>Add a confirmation request to certain actions
>Make the availability of certain actions dependant on game state

###3.4 post_action()

virtual void post_action(game *g, action_id act);

This is run after an action is performed.

>Some ideas:
>Make certain actions perform faster by rebating movement points
>Add extra effects to certain actions
>Display a message upon execution of certain actions

###3.5 game_over()

virtual void game_over(game *g);

This is run when the game is over (player quit or died).

>Some ideas:
>Add a special achievement message, e.g. number of rounds survived
>Delete any special files created during play
