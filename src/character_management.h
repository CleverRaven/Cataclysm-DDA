#pragma once
#ifndef CATA_CHARACTER_MANAGEMENT_H
#define CATA_CHARACTER_MANAGEMENT_H

namespace char_autoreload
{

enum opt : int {
    RELOAD_CHOICE,
    RELOAD_LEAST,
    RELOAD_MOST
};

struct params {
    bool warn_on_last = true;
    bool warn_no_ammo = true;
    opt reload_style = RELOAD_LEAST;
};

} // namespace char_autoreload

#endif // CATA_CHARACTER_MANAGEMENT_H
