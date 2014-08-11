#ifndef _MONDEFENSE_H_
#define _MONDEFENSE_H_

class monster;
struct projectile;

class mdefense {
public:
    /**
     * @pram m The monster the defends itself.
     * @param proj The projectile it was hit by or NULL if it
     * was attacked with a melee attack.
     */
    void none               (monster *, const projectile*) {};
    void zapback            (monster *m, const projectile* proj);
};

#endif
