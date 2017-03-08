#include "catch/catch.hpp"
#include "output.h"

TEST_CASE("Test string_format") {
    CHECK(string_format("%%hello%%") == "%hello%");
    CHECK(string_format("The %1$s impales your torso!", "albino penguin") == "The albino penguin impales your torso!");
    CHECK(string_format("%1$s slaps %2$s with their tentacle", "Urist", "zombie") == "Urist slaps zombie with their tentacle");
    CHECK(string_format("Needs <color_%1$s>%2$s</color>, a <color_%3$s>wrench</color>, either a <color_%4$s>powered welder</color> (and <color_%5$s>welding goggles</color>) or <color_%6$s>duct tape</color>, and level <color_%7$s>%8$d</color> skill in mechanics.%9$s%10$s",
                         "red", "hammer", "green", "white", "purple", "magenta", "rainbow", "8", "that is", "it") ==
          "Needs <color_red>hammer</color>, a <color_green>wrench</color>, either a <color_white>powered welder</color> (and <color_purple>welding goggles</color>) or <color_magenta>duct tape</color>, and level <color_rainbow>8</color> skill in mechanics.that is it");
}
