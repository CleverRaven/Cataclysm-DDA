#ifndef MONATTACK_H
#define MONATTACK_H

class mattack
{
    public:
        void none               (monster *, int) {};
        void antqueen           (monster *z, int count);
        void shriek             (monster *z, int count);
        void howl               (monster *z, int count);
        void rattle             (monster *z, int count);
        void acid               (monster *z, int count);
        void shockstorm         (monster *z, int count);
        void smokecloud         (monster *z, int count);
        void boomer             (monster *z, int count);
        void resurrect          (monster *z, int count);
        void smash              (monster *z, int count);
        void science            (monster *z, int count);
        void growplants         (monster *z, int count);
        void grow_vine          (monster *z, int count);
        void vine               (monster *z, int count);
        void spit_sap           (monster *z, int count);
        void triffid_heartbeat  (monster *z, int count);
        void fungus             (monster *z, int count); // Generic fungal spore-launch
        void fungus_haze        (monster *z, int count); // Broadly scatter aerobics
        void fungus_big_blossom (monster *z, int count); // Aerobic & anaerobic, as needed
        void fungus_inject      (monster *z, int count); // Directly inject the spores
        void fungus_bristle     (monster *z, int count); // Fungal hedgrow rake & inject
        void fungus_growth      (monster *z, int count); // Sporeling -> fungaloid
        void fungus_sprout      (monster *z, int count); // Grow fungal walls
        void fungus_fortify     (monster *z, int count); // Grow fungal hedgerows
        void leap               (monster *z, int count);
        void dermatik           (monster *z, int count);
        void dermatik_growth    (monster *z, int count);
        void plant              (monster *z, int count);
        void disappear          (monster *z, int count);
        void formblob           (monster *z, int count);
        void callblobs          (monster *z, int count);
        void jackson            (monster *z, int count);
        void dance              (monster *z, int count);
        void dogthing           (monster *z, int count);
        void tentacle           (monster *z, int count);
        void vortex             (monster *z, int count);
        void gene_sting         (monster *z, int count);
        void para_sting         (monster *z, int count);
        void triffid_growth     (monster *z, int count);
        void stare              (monster *z, int count);
        void fear_paralyze      (monster *z, int count);
        void photograph         (monster *z, int count);
        void tazer              (monster *z, int count);
        void smg                (monster *z, int count); // Automated MP5
        void laser              (monster *z, int count);
        void rifle_tur          (monster *z, int count); // Automated M4
        void frag_tur           (monster *z, int count); // Automated MGL
        void bmg_tur            (monster *z, int count); // Automated M107 >:-D
        void tank_tur           (monster *z, int count); // Tankbot primary.
        void searchlight        (monster *z, int count);
        void flamethrower       (monster *z, int count);
        void copbot             (monster *z, int count);
        void chickenbot         (monster *z, int count); // Pick from tazer, M4, MGL
        void multi_robot        (monster *z, int count); // Tazer, flame, M4, MGL, or 120mm!
        void ratking            (monster *z, int count);
        void generator          (monster *z, int count);
        void upgrade            (monster *z, int count);
        void breathe            (monster *z, int count);
        void bite               (monster *z, int count);
        void brandish           (monster *z, int count);
        void flesh_golem        (monster *z, int count);
        void lunge              (monster *z, int count);
        void longswipe          (monster *z, int count);
        void parrot             (monster *z, int count);
        void darkman            (monster *z, int count);
        void slimespring        (monster *z, int count);
        void bio_op_takedown    (monster *z, int count);
        void suicide            (monster *z, int count);
        bool thrown_by_judo     (monster *z, int count); //handles zombie getting thrown when u.is_throw_immune()
        void riotbot            (monster *z, int count);
};

#endif
