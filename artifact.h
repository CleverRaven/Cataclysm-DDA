#ifndef _ARTIFACT_H_
#define _ARTIFACT_H_

#include "itype.h"

struct artifact : public item
{
 int dam_bonus, cut_bonus, hit_bonus;

 virtual bool is_artifact()  { return true; }
// Can have one or two invoked uses
 void (iuse::*use1)(game *, item *, bool);
 void (iuse::*use2)(game *, item *, bool);
// *wield is every-turn effect while wielded or worn
 void (iuse::*wield)(game *, item *, bool);
// *unwield is effect upon unwielding or taking off
 void (iuse::*unwield)(game *, item *, bool);
// *carry is every-turn effect while it's in your inventory
 void (iuse::*carry)(game *, item *, bool);
 
};
  
#endif
