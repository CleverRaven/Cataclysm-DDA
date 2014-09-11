#ifndef _MONATTACK_H_
#define _MONATTACK_H_

class mattack
{
    public:
        void none               (monster *) {};
        void antqueen           (monster *z);
        void shriek             (monster *z);
        void howl               (monster *z);
        void rattle             (monster *z);
        void acid               (monster *z);
        void shockstorm         (monster *z);
        void smokecloud         (monster *z);
        void boomer             (monster *z);
        void resurrect          (monster *z);
        void science            (monster *z);
        void growplants         (monster *z);
        void grow_vine          (monster *z);
        void vine               (monster *z);
        void spit_sap           (monster *z);
        void triffid_heartbeat  (monster *z);
        void fungus             (monster *z); // Generic fungal spore-launch
        void fungus_haze        (monster *z); // Broadly scatter aerobics
        void fungus_big_blossom (monster *z); // Aerobic & anaerobic, as needed
        void fungus_inject      (monster *z); // Directly inject the spores
        void fungus_bristle     (monster *z); // Fungal hedgrow rake & inject
        void fungus_growth      (monster *z); // Sporeling -> fungaloid
        void fungus_sprout      (monster *z); // Grow fungal walls
        void fungus_fortify     (monster *z); // Grow fungal hedgerows
        void leap               (monster *z);
        void dermatik           (monster *z);
        void dermatik_growth    (monster *z);
        void plant              (monster *z);
        void disappear          (monster *z);
        void formblob           (monster *z);
        void callblobs          (monster *z);
        void jackson            (monster *z);
        void dance              (monster *z);
        void dogthing           (monster *z);
        void tentacle           (monster *z);
        void vortex             (monster *z);
        void gene_sting         (monster *z);
        void para_sting         (monster *z);
        void triffid_growth     (monster *z);
        void stare              (monster *z);
        void fear_paralyze      (monster *z);
        void photograph         (monster *z);
        void tazer              (monster *z);
        void smg                (monster *z); // Automated MP5
        void laser              (monster *z);
        void rifle_tur          (monster *z); // Automated M4
        void frag_tur           (monster *z); // Automated MGL
        void bmg_tur            (monster *z); // Automated M107 >:-D
        void tank_tur           (monster *z); // Tankbot primary.
        void searchlight        (monster *z);
        void flamethrower       (monster *z);
        void copbot             (monster *z);
        void chickenbot         (monster *z); // Pick from tazer, M4, MGL
        void multi_robot        (monster *z); // Tazer, flame, M4, MGL, or 120mm!
        void ratking            (monster *z);
        void generator          (monster *z);
        void upgrade            (monster *z);
        void breathe            (monster *z);
        void bite               (monster *z);
        void brandish           (monster *z);
        void flesh_golem        (monster *z);
        void lunge              (monster *z);
        void longswipe          (monster *z);
        void parrot             (monster *z);
        void darkman            (monster *z);
        void slimespring        (monster *z);
        void bio_op_takedown    (monster *z);
        void suicide            (monster *z);
        bool thrown_by_judo     (monster *z); //handles zombie getting thrown when u.is_throw_immune()
        void riotbot            (monster *z);
};

#endif
