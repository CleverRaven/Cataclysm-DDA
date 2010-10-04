#ifndef _ARTIFACT_H_
#define _ARTIFACT_H_

#include "itype.h"

struct it_artifact : itype
{
 itype base;
 virtual bool is_artifact()  { return true; }
// is_* returns the value for the base type
 virtual bool is_food()      { return base.is_food();      };
 virtual bool is_ammo()      { return base.is_ammo();      };
 virtual bool is_armor()     { return base.is_armor();     };
 virtual bool is_book()      { return base.is_book();      };
 virtual bool is_tool()      { return base.is_tool();      };
 virtual bool is_container() { return base.is_container(); };
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
