#ifndef MONATTACK_H
#define MONATTACK_H

class monster;
class Creature;

class mattack
{
    public:
        void none               (monster *, int) {};
        void antqueen           (monster *z, int index);
        void shriek             (monster *z, int index);
        void howl               (monster *z, int index);
        void rattle             (monster *z, int index);
        void acid               (monster *z, int index);
        void shockstorm         (monster *z, int index);
        void smokecloud         (monster *z, int index);
        void boomer             (monster *z, int index);
        void resurrect          (monster *z, int index);
        void smash              (monster *z, int index);
        void science            (monster *z, int index);
        void growplants         (monster *z, int index);
        void grow_vine          (monster *z, int index);
        void vine               (monster *z, int index);
        void spit_sap           (monster *z, int index);
        void triffid_heartbeat  (monster *z, int index);
        void fungus             (monster *z, int index); // Generic fungal spore-launch
        void fungus_haze        (monster *z, int index); // Broadly scatter aerobics
        void fungus_big_blossom (monster *z, int index); // Aerobic & anaerobic, as needed
        void fungus_inject      (monster *z, int index); // Directly inject the spores
        void fungus_bristle     (monster *z, int index); // Fungal hedgrow rake & inject
        void fungus_growth      (monster *z, int index); // Sporeling -> fungaloid
        void fungus_sprout      (monster *z, int index); // Grow fungal walls
        void fungus_fortify     (monster *z, int index); // Grow fungal hedgerows
        void leap               (monster *z, int index);
        void dermatik           (monster *z, int index);
        void dermatik_growth    (monster *z, int index);
        void plant              (monster *z, int index);
        void disappear          (monster *z, int index);
        void formblob           (monster *z, int index);
        void callblobs          (monster *z, int index);
        void jackson            (monster *z, int index);
        void dance              (monster *z, int index);
        void dogthing           (monster *z, int index);
        void tentacle           (monster *z, int index);
        void vortex             (monster *z, int index);
        void gene_sting         (monster *z, int index);
        void para_sting         (monster *z, int index);
        void triffid_growth     (monster *z, int index);
        void stare              (monster *z, int index);
        void fear_paralyze      (monster *z, int index);
        void photograph         (monster *z, int index);
        void tazer              (monster *z, int index);
        void smg                (monster *z, int index); // Automated MP5
        void laser              (monster *z, int index);
        void rifle_tur          (monster *z, int index); // Automated M4
        void bmg_tur            (monster *z, int index); // Automated M107 >:-D
        void flamethrower       (monster *z, int index);
        void searchlight        (monster *z, int index);
        void copbot             (monster *z, int index);
        void chickenbot         (monster *z, int index); // Pick from tazer, M4, MGL
        void multi_robot        (monster *z, int index); // Tazer, flame, M4, MGL, or 120mm!
        void ratking            (monster *z, int index);
        void generator          (monster *z, int index);
        void upgrade            (monster *z, int index);
        void breathe            (monster *z, int index);
        void bite               (monster *z, int index);
        void brandish           (monster *z, int index);
        void flesh_golem        (monster *z, int index);
        void lunge              (monster *z, int index);
        void longswipe          (monster *z, int index);
        void parrot             (monster *z, int index);
        void darkman            (monster *z, int index);
        void slimespring        (monster *z, int index);
        void bio_op_takedown    (monster *z, int index);
        void suicide            (monster *z, int index);
        bool thrown_by_judo     (monster *z, int index); //handles zombie getting thrown when u.is_throw_immune()
        void riotbot            (monster *z, int index);
    private:
        void taze               (monster *z, Creature *target);
        void rifle              (monster *z, Creature *target); // Automated M4
        void frag               (monster *z, Creature *target); // Automated MGL
        void tankgun            (monster *z, Creature *target); // Tankbot primary.
        void flame              (monster *z, Creature *target);
};

#endif
