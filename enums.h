#ifndef _ENUMS_H_
#define _ENUMS_H_

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

enum material {
MNULL = 0,
//Food Materials
LIQUID, VEGGY, FLESH, POWDER,
//Clothing
COTTON, WOOL, LEATHER, KEVLAR,
//Other
STONE, PAPER, WOOD, PLASTIC, GLASS, IRON, STEEL, SILVER
};

struct point {
 int x;
 int y;
 point(int X = 0, int Y = 0) : x (X), y (Y) {}
 ~point(){}
};

#endif
