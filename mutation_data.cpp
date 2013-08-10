#include "game.h"
#include "setvector.h"

#define MUTATION(mut) id = mut //What mutation/trait is it?

#define VALID(val) mutation_data[id].valid = val //Can it be mutated during the game?

#define PREREQS(...) \
  setvector(&(mutation_data[id].prereqs), __VA_ARGS__, NULL) //What do we need to have before we can mutate it?

#define CANCELS(...) \
  setvector(&(mutation_data[id].cancels), __VA_ARGS__, NULL) //What does it remove?

#define CHANGES_TO(...) \
  setvector(&(mutation_data[id].replacements), __VA_ARGS__, NULL) //What does it upgrade to?

#define LEADS_TO(...) \
  setvector(&(mutation_data[id].additions), __VA_ARGS__, NULL) //What is it a prereq for?

void game::init_mutations()
{
 int id = 0;
 
 MUTATION(PF_NULL);
  VALID(false);

 MUTATION(PF_FLEET);
  VALID(true);
  CANCELS (PF_PONDEROUS1, PF_PONDEROUS2, PF_PONDEROUS3);
  CHANGES_TO (PF_FLEET2);
  
 MUTATION(PF_PARKOUR);
  VALID(false);
  
 MUTATION(PF_QUICK);
  VALID(true);
  
 MUTATION(PF_OPTIMISTIC);
  VALID(false);

 MUTATION(PF_FASTHEALER);
  VALID(true);
  CANCELS (PF_ROT1, PF_ROT2, PF_ROT3);
  CHANGES_TO (PF_FASTHEALER2);

 MUTATION(PF_LIGHTEATER);
  VALID(true);
  
 MUTATION(PF_PAINRESIST);
  VALID(false);

 MUTATION(PF_NIGHTVISION);
  VALID(true);
  CHANGES_TO (PF_NIGHTVISION2);

 MUTATION(PF_POISRESIST);
  VALID(true);
  
 MUTATION(PF_FASTREADER);
  VALID(false);
  
 MUTATION(PF_TOUGH);
  VALID(false);

 MUTATION(PF_THICKSKIN);
  VALID(true);
  
 MUTATION(PF_PACKMULE);
  VALID(false);
  
 MUTATION(PF_FASTLEARNER);
  VALID(false);

 MUTATION(PF_DEFT);
  VALID(true);
  
 MUTATION(PF_DRUNKEN);
  VALID(false);
  
 MUTATION(PF_GOURMAND);
  VALID(false);

 MUTATION(PF_ANIMALEMPATH);
  VALID(true);

 MUTATION(PF_TERRIFYING);
  VALID(true);

 MUTATION(PF_DISRESISTANT);
  VALID(true);
  CHANGES_TO (PF_DISIMMUNE);
  
 MUTATION(PF_ADRENALINE);
  VALID(true);
  
 MUTATION(PF_SELFAWARE);
  VALID(false);
  
 MUTATION(PF_INCONSPICUOUS);
  VALID(false);
  
 MUTATION(PF_MASOCHIST);
  VALID(false);
  
 MUTATION(PF_LIGHTSTEP);
  VALID(true);
  
 MUTATION(PF_ANDROID);
  VALID(false);

 MUTATION(PF_ROBUST);
  VALID(true);
  
 MUTATION(PF_CANNIBAL);
  VALID(false);
  
 MUTATION(PF_MARTIAL_ARTS);
  VALID(false);
  
 MUTATION(PF_MARTIAL_ARTS2);
  VALID(false);
    
 MUTATION(PF_MARTIAL_ARTS3);
  VALID(false);
    
 MUTATION(PF_MARTIAL_ARTS4);
  VALID(false);

 MUTATION(PF_LIAR);
  VALID(true);
  CANCELS (PF_TRUTHTELLER);
  
 MUTATION(PF_PRETTY);
  VALID(true);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  CHANGES_TO (PF_BEAUTIFUL);
  
 MUTATION(PF_SPLIT); // Null trait
  VALID(false);
// Negative start traits begin here

 MUTATION(PF_MYOPIC);
  VALID(true);
  
 MUTATION(PF_HYPEROPIC);
  VALID(false);

 MUTATION(PF_HEAVYSLEEPER);
  VALID(true);
  
 MUTATION(PF_ASTHMA);
  VALID(false);
  
 MUTATION(PF_BADBACK);
  VALID(true);
  
 MUTATION(PF_ILLITERATE);
  VALID(false);
  
 MUTATION(PF_BADHEARING);
  VALID(true);
  
 MUTATION(PF_INSOMNIA);
  VALID(false);
  
 MUTATION(PF_VEGETARIAN);
  VALID(false);
  
 MUTATION(PF_GLASSJAW);
  VALID(true);
  
 MUTATION(PF_FORGETFUL);
  VALID(true);

 MUTATION(PF_LIGHTWEIGHT);
  VALID(true);
  
 MUTATION(PF_ADDICTIVE);
  VALID(false);
  
 MUTATION(PF_TRIGGERHAPPY);
  VALID(false);

 MUTATION(PF_SMELLY);
  VALID(true);
  CHANGES_TO (PF_SMELLY2);
  
 MUTATION(PF_CHEMIMBALANCE);
  VALID(true);
  
 MUTATION(PF_SCHIZOPHRENIC);
  VALID(false);
  
 MUTATION(PF_JITTERY);
  VALID(false);
  
 MUTATION(PF_HOARDER);
  VALID(false);
  
 MUTATION(PF_SAVANT);
  VALID(false);
  
 MUTATION(PF_MOODSWINGS);
  VALID(false);

 MUTATION(PF_WEAKSTOMACH);
  VALID(true);
  CHANGES_TO (PF_NAUSEA);
  
 MUTATION(PF_WOOLALLERGY);
  VALID(true);

 MUTATION(PF_TRUTHTELLER);
  VALID(true);
  CANCELS (PF_LIAR);  
  
 MUTATION(PF_UGLY);
  VALID(true);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  CHANGES_TO (PF_DEFORMED);
  
 MUTATION(PF_HARDCORE);
  VALID(false);

 MUTATION(PF_MAX); //Null trait
  VALID(false);
// Mutation-only traits start here

 MUTATION(PF_SKIN_ROUGH);
  VALID(true);
  CHANGES_TO (PF_SCALES, PF_FEATHERS, PF_LIGHTFUR, PF_CHITIN, PF_PLANTSKIN);

 MUTATION(PF_NIGHTVISION2);
  VALID(true);
  PREREQS (PF_NIGHTVISION);
  CHANGES_TO (PF_NIGHTVISION3);

 MUTATION(PF_NIGHTVISION3);
  VALID(true);
  PREREQS (PF_NIGHTVISION2);
  LEADS_TO (PF_INFRARED);
  
 MUTATION(PF_INFRARED);
  VALID(true);
  PREREQS (PF_NIGHTVISION3);

 MUTATION(PF_FASTHEALER2);
  VALID(true);
  CANCELS (PF_ROT1, PF_ROT2, PF_ROT3);
  PREREQS (PF_FASTHEALER);
  CHANGES_TO (PF_REGEN);

 MUTATION(PF_REGEN);
  VALID(true);
  CANCELS (PF_ROT1, PF_ROT2, PF_ROT3);
  PREREQS (PF_FASTHEALER2);

 MUTATION(PF_FANGS);
  VALID(true);
  CANCELS (PF_BEAK);

 MUTATION(PF_MEMBRANE);
  VALID(true);
  PREREQS (PF_EYEBULGE);
  
 MUTATION(PF_GILLS);
  VALID(true);

 MUTATION(PF_SCALES);
  VALID(true);
  PREREQS (PF_SKIN_ROUGH);
  CHANGES_TO (PF_THICK_SCALES, PF_SLEEK_SCALES);

 MUTATION(PF_THICK_SCALES);
  VALID(true);
  PREREQS (PF_SCALES);
  CANCELS (PF_SLEEK_SCALES, PF_FEATHERS);

 MUTATION(PF_SLEEK_SCALES);
  VALID(true);
  PREREQS (PF_SCALES);
  CANCELS (PF_THICK_SCALES, PF_FEATHERS);

 MUTATION(PF_LIGHT_BONES);
  VALID(true);
  CHANGES_TO (PF_HOLLOW_BONES);

 MUTATION(PF_FEATHERS);
  VALID(true);
  PREREQS (PF_SKIN_ROUGH);
  CANCELS(PF_THICK_SCALES, PF_SLEEK_SCALES);

 MUTATION(PF_LIGHTFUR);
  VALID(true);
  PREREQS (PF_SKIN_ROUGH);
  CANCELS (PF_SCALES, PF_FEATHERS, PF_CHITIN, PF_PLANTSKIN);
  CHANGES_TO (PF_FUR);

 MUTATION(PF_FUR);
  VALID(true);
  CANCELS (PF_SCALES, PF_FEATHERS, PF_CHITIN, PF_PLANTSKIN);
  PREREQS (PF_LIGHTFUR);

 MUTATION(PF_CHITIN);
  VALID(true);
  PREREQS (PF_SKIN_ROUGH);
  CANCELS (PF_SCALES, PF_FEATHERS, PF_LIGHTFUR, PF_PLANTSKIN);
  CHANGES_TO (PF_CHITIN2);

 MUTATION(PF_CHITIN2);
  VALID(true);
  PREREQS (PF_CHITIN);
  CHANGES_TO (PF_CHITIN3);

 MUTATION(PF_CHITIN3);
  VALID(true);
  PREREQS (PF_CHITIN2);

 MUTATION(PF_SPINES);
  VALID(true);
  CHANGES_TO (PF_QUILLS);

 MUTATION (PF_QUILLS);
  VALID(true);
  PREREQS (PF_SPINES);

 MUTATION (PF_PLANTSKIN);
  VALID(true);
  CANCELS (PF_FEATHERS, PF_LIGHTFUR, PF_FUR, PF_CHITIN, PF_CHITIN2, PF_CHITIN3,
           PF_SCALES);
  CHANGES_TO (PF_BARK);
  LEADS_TO (PF_THORNS, PF_LEAVES);

 MUTATION(PF_BARK);
  VALID(true);
  PREREQS (PF_PLANTSKIN);

 MUTATION(PF_THORNS);
  VALID(true);
  PREREQS (PF_PLANTSKIN, PF_BARK);

 MUTATION(PF_LEAVES);
  VALID(true);
  PREREQS (PF_PLANTSKIN, PF_BARK);

 MUTATION(PF_NAILS);
  VALID(true);
  CHANGES_TO (PF_CLAWS, PF_TALONS);

 MUTATION(PF_CLAWS);
  VALID(true);
  PREREQS (PF_NAILS);
  CANCELS (PF_TALONS);

 MUTATION(PF_TALONS);
  VALID(true);
  PREREQS (PF_NAILS);
  CANCELS (PF_CLAWS);
  
 MUTATION(PF_RADIOGENIC);
  VALID(true);
  
 MUTATION(PF_MARLOSS);
  VALID(false);

 MUTATION(PF_PHEROMONE_INSECT);
  VALID(true);
  PREREQS (PF_SMELLY2);
  CANCELS (PF_PHEROMONE_MAMMAL);

 MUTATION(PF_PHEROMONE_MAMMAL);
  VALID(true);
  PREREQS (PF_SMELLY2);
  CANCELS (PF_PHEROMONE_INSECT);

 MUTATION(PF_DISIMMUNE);
  VALID(true);
  PREREQS (PF_DISRESISTANT);

 MUTATION(PF_POISONOUS);
  VALID(true);
  PREREQS (PF_POISRESIST);

 MUTATION(PF_SLIME_HANDS);
  VALID(true);
  PREREQS (PF_SLIMY);

 MUTATION(PF_COMPOUND_EYES);
  VALID(true);
  PREREQS (PF_EYEBULGE);

 MUTATION(PF_PADDED_FEET);
  VALID(true);
  CANCELS (PF_HOOVES, PF_LEG_TENTACLES);

 MUTATION(PF_HOOVES);
  VALID(true);
  CANCELS (PF_PADDED_FEET, PF_LEG_TENTACLES);

 MUTATION(PF_SAPROVORE);
  VALID(true);
  PREREQS (PF_CARNIVORE);
  CANCELS (PF_HERBIVORE, PF_RUMINANT);

 MUTATION(PF_RUMINANT);
  VALID(true);
  PREREQS (PF_HERBIVORE);
  CANCELS (PF_CARNIVORE, PF_SAPROVORE);

 MUTATION(PF_HORNS);
  VALID(true);
  PREREQS (PF_HEADBUMPS);
  CANCELS (PF_ANTENNAE);
  CHANGES_TO (PF_HORNS_CURLED, PF_HORNS_POINTED, PF_ANTLERS);

 MUTATION(PF_HORNS_CURLED);
  VALID(true);
  PREREQS (PF_HORNS);
  CANCELS (PF_ANTENNAE, PF_HORNS_POINTED, PF_ANTLERS);

 MUTATION(PF_HORNS_POINTED);
  VALID(true);
  PREREQS (PF_HORNS);
  CANCELS (PF_ANTENNAE, PF_HORNS_CURLED, PF_ANTLERS);

 MUTATION(PF_ANTLERS);
  VALID(true);
  PREREQS (PF_HORNS);
  CANCELS (PF_ANTENNAE, PF_HORNS_CURLED, PF_HORNS_POINTED);

 MUTATION(PF_ANTENNAE);
  VALID(true);
  PREREQS (PF_HEADBUMPS);
  CANCELS (PF_HORNS, PF_HORNS_CURLED, PF_HORNS_POINTED, PF_ANTLERS);
  
 MUTATION(PF_FLEET2);
  VALID(true);
  PREREQS (PF_FLEET);
  CANCELS (PF_PONDEROUS1, PF_PONDEROUS2, PF_PONDEROUS3);

 MUTATION(PF_TAIL_STUB);
  VALID(true);
  CHANGES_TO (PF_TAIL_LONG, PF_TAIL_FIN);

 MUTATION(PF_TAIL_FIN);
  VALID(true);
  PREREQS (PF_TAIL_STUB);
  CANCELS (PF_TAIL_LONG, PF_TAIL_FLUFFY, PF_TAIL_STING, PF_TAIL_CLUB);

 MUTATION(PF_TAIL_LONG);
  VALID(true);
  PREREQS (PF_TAIL_STUB);
  CANCELS (PF_TAIL_FIN);
  CHANGES_TO (PF_TAIL_FLUFFY, PF_TAIL_STING, PF_TAIL_CLUB);

 MUTATION(PF_TAIL_FLUFFY);
  VALID(true);
  PREREQS (PF_TAIL_LONG);
  CANCELS (PF_TAIL_STING, PF_TAIL_CLUB, PF_TAIL_FIN);

 MUTATION(PF_TAIL_STING);
  VALID(true);
  PREREQS (PF_TAIL_LONG);
  CANCELS (PF_TAIL_FLUFFY, PF_TAIL_CLUB, PF_TAIL_FIN);

 MUTATION(PF_TAIL_CLUB);
  VALID(true);
  PREREQS (PF_TAIL_LONG);
  CANCELS (PF_TAIL_FLUFFY, PF_TAIL_STING, PF_TAIL_FIN);

 MUTATION(PF_PAINREC1);
  VALID(true);
  CHANGES_TO (PF_PAINREC2);

 MUTATION(PF_PAINREC2);
  VALID(true);
  PREREQS (PF_PAINREC1);
  CHANGES_TO (PF_PAINREC3);

 MUTATION(PF_PAINREC3);
  VALID(true);
  PREREQS (PF_PAINREC2);

 MUTATION(PF_WINGS_BIRD);
  VALID(true);
  PREREQS (PF_WINGS_STUB);
  CANCELS (PF_WINGS_BAT, PF_WINGS_INSECT);

 MUTATION(PF_WINGS_INSECT);
  VALID(true);
  PREREQS (PF_WINGS_STUB);
  CANCELS (PF_WINGS_BIRD, PF_WINGS_BAT);

 MUTATION(PF_MOUTH_TENTACLES);
  VALID(true);
  PREREQS (PF_MOUTH_FLAPS);
  CANCELS (PF_MANDIBLES);

 MUTATION(PF_MANDIBLES);
  VALID(true);
  PREREQS (PF_MOUTH_FLAPS);
  CANCELS (PF_BEAK, PF_FANGS, PF_MOUTH_TENTACLES);
  
 MUTATION(PF_CANINE_EARS);
  VALID(true);

 MUTATION(PF_WEB_WALKER);
  VALID(true);
  LEADS_TO (PF_WEB_WEAVER);

 MUTATION(PF_WEB_WEAVER);
  VALID(true);
  PREREQS (PF_WEB_WALKER);
  CANCELS (PF_SLIMY);
  
 MUTATION(PF_WHISKERS);
  VALID(true);

 MUTATION(PF_STR_UP);
  VALID(true);
  CHANGES_TO (PF_STR_UP_2);

 MUTATION(PF_STR_UP_2);
  VALID(true);
  PREREQS (PF_STR_UP);
  CHANGES_TO (PF_STR_UP_3);

 MUTATION(PF_STR_UP_3);
  VALID(true);
  PREREQS (PF_STR_UP_2);
  CHANGES_TO (PF_STR_UP_4);

 MUTATION(PF_STR_UP_4);
  VALID(true);
  PREREQS (PF_STR_UP_3);

 MUTATION(PF_DEX_UP);
  VALID(true);
  CHANGES_TO (PF_DEX_UP_2);

 MUTATION(PF_DEX_UP_2);
  VALID(true);
  PREREQS (PF_DEX_UP);
  CHANGES_TO (PF_DEX_UP_3);

 MUTATION(PF_DEX_UP_3);
  VALID(true);
  PREREQS (PF_DEX_UP_2);
  CHANGES_TO (PF_DEX_UP_4);

 MUTATION(PF_DEX_UP_4);
  VALID(true);
  PREREQS (PF_DEX_UP_3);

 MUTATION(PF_INT_UP);
  VALID(true);
  CHANGES_TO (PF_INT_UP_2);

 MUTATION(PF_INT_UP_2);
  VALID(true);
  PREREQS (PF_INT_UP);
  CHANGES_TO (PF_INT_UP_3);

 MUTATION(PF_INT_UP_3);
  VALID(true);
  PREREQS (PF_INT_UP_2);
  CHANGES_TO (PF_INT_UP_4);

 MUTATION(PF_INT_UP_4);
  VALID(true);
  PREREQS (PF_INT_UP_3);

 MUTATION(PF_PER_UP);
  VALID(true);
  CHANGES_TO (PF_PER_UP_2);

 MUTATION(PF_PER_UP_2);
  VALID(true);
  PREREQS (PF_PER_UP);
  CHANGES_TO (PF_PER_UP_3);

 MUTATION(PF_PER_UP_3);
  VALID(true);
  PREREQS (PF_PER_UP_2);
  CHANGES_TO (PF_PER_UP_4);

 MUTATION(PF_PER_UP_4);
  VALID(true);
  PREREQS (PF_PER_UP_3);

 MUTATION(PF_BEAUTIFUL);
  VALID(true);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  PREREQS (PF_PRETTY);
  CHANGES_TO (PF_BEAUTIFUL2);

 MUTATION(PF_BEAUTIFUL2);
  VALID(true);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  PREREQS (PF_BEAUTIFUL);
  CHANGES_TO (PF_BEAUTIFUL3);

 MUTATION(PF_BEAUTIFUL3);
  VALID(true);
  CANCELS (PF_UGLY, PF_DEFORMED, PF_DEFORMED2, PF_DEFORMED3);
  PREREQS (PF_BEAUTIFUL2);

// Bad mutations below this point

 MUTATION(PF_HEADBUMPS);
  VALID(true);
  CHANGES_TO (PF_HORNS, PF_ANTENNAE);
  
 MUTATION(PF_SLIT_NOSTRILS);
  VALID(true);
  
 MUTATION(PF_FORKED_TONGUE);
  VALID(true);

 MUTATION(PF_EYEBULGE);
  VALID(true);
  LEADS_TO (PF_MEMBRANE);
  CHANGES_TO (PF_COMPOUND_EYES);
  
 MUTATION(PF_MOUTH_FLAPS);
  VALID(true);
  LEADS_TO (PF_MOUTH_TENTACLES, PF_MANDIBLES);
  
 MUTATION(PF_WINGS_STUB);
  VALID(true);
  CHANGES_TO (PF_WINGS_BIRD, PF_WINGS_BAT, PF_WINGS_INSECT);

 MUTATION(PF_WINGS_BAT);
  VALID(true);
  PREREQS (PF_WINGS_STUB);
  CANCELS (PF_WINGS_BIRD, PF_WINGS_INSECT);

 MUTATION(PF_PALE);
  VALID(true);
  CHANGES_TO (PF_ALBINO);
  LEADS_TO (PF_TROGLO);

 MUTATION(PF_SPOTS);
  VALID(true);
  CHANGES_TO (PF_SORES);

 MUTATION(PF_SMELLY2);
  VALID(true);
  PREREQS (PF_SMELLY);
  LEADS_TO (PF_PHEROMONE_INSECT, PF_PHEROMONE_MAMMAL);

 MUTATION(PF_DEFORMED);
  VALID(true);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  PREREQS (PF_UGLY);
  CHANGES_TO (PF_DEFORMED2);

 MUTATION(PF_DEFORMED2);
  VALID(true);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  PREREQS (PF_DEFORMED);
  CHANGES_TO (PF_DEFORMED3);

 MUTATION(PF_DEFORMED3);
  VALID(true);
  CANCELS (PF_PRETTY, PF_BEAUTIFUL, PF_BEAUTIFUL2, PF_BEAUTIFUL3);
  PREREQS (PF_DEFORMED2);

 MUTATION(PF_HOLLOW_BONES);
  VALID(true);
  PREREQS (PF_LIGHT_BONES);

 MUTATION(PF_NAUSEA);
  VALID(true);
  PREREQS (PF_WEAKSTOMACH);
  CHANGES_TO (PF_VOMITOUS);

 MUTATION(PF_VOMITOUS);
  VALID(true);
  PREREQS (PF_NAUSEA);
  
 MUTATION(PF_HUNGER);
  VALID(true);
  
 MUTATION(PF_THIRST);
  VALID(true);

 MUTATION(PF_ROT1);
  VALID(true);
  CANCELS (PF_FASTHEALER, PF_FASTHEALER2, PF_REGEN);
  CHANGES_TO (PF_ROT2);

 MUTATION(PF_ROT2);
  VALID(true);
  CANCELS (PF_FASTHEALER, PF_FASTHEALER2, PF_REGEN);
  PREREQS (PF_ROT1);
  CHANGES_TO (PF_ROT3);

 MUTATION(PF_ROT3);
  VALID(true);
  CANCELS (PF_FASTHEALER, PF_FASTHEALER2, PF_REGEN);
  PREREQS (PF_ROT2);

 MUTATION(PF_ALBINO);
  VALID(true);
  PREREQS (PF_PALE);

 MUTATION(PF_SORES);
  VALID(true);
  PREREQS (PF_SPOTS);

 MUTATION(PF_TROGLO);
  VALID(true);
  CANCELS (PF_SUNLIGHT_DEPENDENT);
  CHANGES_TO (PF_TROGLO2);

 MUTATION(PF_TROGLO2);
  VALID(true);
  CANCELS (PF_SUNLIGHT_DEPENDENT);
  PREREQS (PF_TROGLO);
  CHANGES_TO (PF_TROGLO3);

 MUTATION(PF_TROGLO3);
  VALID(true);
  CANCELS (PF_SUNLIGHT_DEPENDENT);
  PREREQS (PF_TROGLO2);
  
 MUTATION(PF_WEBBED);
  VALID(true);

 MUTATION(PF_BEAK);
  VALID(true);
  CANCELS (PF_FANGS, PF_MANDIBLES);
  
 MUTATION(PF_UNSTABLE);
  VALID(true);

 MUTATION(PF_RADIOACTIVE1);
  VALID(true);
  CHANGES_TO (PF_RADIOACTIVE2);

 MUTATION(PF_RADIOACTIVE2);
  VALID(true);
  PREREQS (PF_RADIOACTIVE1);
  CHANGES_TO (PF_RADIOACTIVE3);

 MUTATION(PF_RADIOACTIVE3);
  VALID(true);
  PREREQS (PF_RADIOACTIVE2);

 MUTATION(PF_SLIMY);
  VALID(true);
  LEADS_TO (PF_SLIME_HANDS);

 MUTATION(PF_HERBIVORE);
  VALID(true);
  CANCELS (PF_CARNIVORE, PF_SAPROVORE);
  LEADS_TO (PF_RUMINANT);
  
 MUTATION(PF_CARNIVORE);
  VALID(true);
  CANCELS (PF_HERBIVORE, PF_RUMINANT);
  LEADS_TO (PF_SAPROVORE);

 MUTATION(PF_PONDEROUS1);
  VALID(true);
  CANCELS (PF_FLEET, PF_FLEET2);
  CHANGES_TO (PF_PONDEROUS2);

 MUTATION(PF_PONDEROUS2);
  VALID(true);
  CANCELS (PF_FLEET, PF_FLEET2);
  PREREQS (PF_PONDEROUS1);
  CHANGES_TO (PF_PONDEROUS3);

 MUTATION(PF_PONDEROUS3);
  VALID(true);
  CANCELS (PF_FLEET, PF_FLEET2);
  PREREQS (PF_PONDEROUS2);

 MUTATION(PF_SUNLIGHT_DEPENDENT);
  VALID(true);
  CANCELS (PF_TROGLO, PF_TROGLO2, PF_TROGLO3);

 MUTATION(PF_COLDBLOOD);
  VALID(true);
  CHANGES_TO (PF_COLDBLOOD2);

 MUTATION(PF_COLDBLOOD2);
  VALID(true);
  PREREQS (PF_COLDBLOOD);
  CHANGES_TO (PF_COLDBLOOD3);

 MUTATION(PF_COLDBLOOD3);
  VALID(true);
  PREREQS (PF_COLDBLOOD2);

 MUTATION(PF_GROWL);
  VALID(true);
  CHANGES_TO (PF_SNARL);

 MUTATION(PF_SNARL);
  VALID(true);
  PREREQS (PF_GROWL);

 MUTATION(PF_SHOUT1);
  VALID(true);
  CHANGES_TO (PF_SHOUT2);

 MUTATION(PF_SHOUT2);
  VALID(true);
  PREREQS (PF_SHOUT1);
  CHANGES_TO (PF_SHOUT3);

 MUTATION(PF_SHOUT3);
  VALID(true);
  PREREQS (PF_SHOUT2);

 MUTATION(PF_ARM_TENTACLES);
  VALID(true);
  CHANGES_TO (PF_ARM_TENTACLES_4);

 MUTATION(PF_ARM_TENTACLES_4);
  VALID(true);
  PREREQS (PF_ARM_TENTACLES);
  CHANGES_TO (PF_ARM_TENTACLES_8);

 MUTATION(PF_ARM_TENTACLES_8);
  VALID(true);
  PREREQS (PF_ARM_TENTACLES_4);

 MUTATION(PF_SHELL);
  VALID(true);
  PREREQS (PF_CHITIN);
  CANCELS (PF_CHITIN3);

 MUTATION(PF_LEG_TENTACLES);
  VALID(true);
  CANCELS (PF_PADDED_FEET, PF_HOOVES);
  
 MUTATION(PF_MAX2); //Null trait
  VALID(false);


}



std::vector<pl_flag> mutations_from_category(mutation_category cat)
{
 std::vector<pl_flag> ret;
 switch (cat) {

 case MUTCAT_LIZARD:
  setvector(&ret,
PF_THICKSKIN, PF_INFRARED, PF_FANGS, PF_MEMBRANE, PF_SCALES, PF_TALONS,
PF_SLIT_NOSTRILS, PF_FORKED_TONGUE, PF_TROGLO, PF_WEBBED, PF_CARNIVORE,
PF_COLDBLOOD2, PF_STR_UP_2, PF_DEX_UP_2, NULL);
  break;

 case MUTCAT_BIRD:
  setvector(&ret,
PF_QUICK, PF_LIGHTEATER, PF_DEFT, PF_LIGHTSTEP, PF_BADBACK, PF_GLASSJAW,
PF_NIGHTVISION, PF_HOLLOW_BONES, PF_FEATHERS, PF_TALONS, PF_BEAK, PF_FLEET2,
PF_WINGS_BIRD, PF_COLDBLOOD, PF_DEX_UP_3, NULL);
  break;

 case MUTCAT_FISH:
  setvector(&ret,
PF_SMELLY2, PF_NIGHTVISION2, PF_FANGS, PF_MEMBRANE, PF_GILLS, PF_SLEEK_SCALES,
PF_TAIL_FIN, PF_DEFORMED, PF_THIRST, PF_WEBBED, PF_SLIMY, PF_COLDBLOOD2, NULL);
  break;

 case MUTCAT_BEAST:
  setvector(&ret,
PF_DEFT, PF_ANIMALEMPATH, PF_TERRIFYING, PF_ADRENALINE, PF_MYOPIC, PF_FORGETFUL,
PF_NIGHTVISION2, PF_FANGS, PF_FUR, PF_CLAWS, PF_PHEROMONE_MAMMAL,
PF_PADDED_FEET, PF_TAIL_FLUFFY, PF_SMELLY2, PF_DEFORMED2, PF_HUNGER, PF_TROGLO,
PF_CARNIVORE, PF_SNARL, PF_SHOUT3, PF_CANINE_EARS, PF_STR_UP_4, NULL);
  break;

 case MUTCAT_CATTLE:
  setvector(&ret,
PF_THICKSKIN, PF_DISRESISTANT, PF_NIGHTVISION, PF_FUR, PF_PHEROMONE_MAMMAL,
PF_HOOVES, PF_RUMINANT, PF_HORNS, PF_TAIL_LONG, PF_DEFORMED, PF_PONDEROUS2,
PF_CANINE_EARS, PF_STR_UP_2, NULL);

 case MUTCAT_INSECT:
  setvector(&ret,
PF_QUICK, PF_LIGHTEATER, PF_POISRESIST, PF_NIGHTVISION, PF_TERRIFYING,
PF_HEAVYSLEEPER, PF_NIGHTVISION2, PF_INFRARED, PF_CHITIN2, PF_PHEROMONE_INSECT,
PF_ANTENNAE, PF_WINGS_INSECT, PF_TAIL_STING, PF_MANDIBLES, PF_DEFORMED,
PF_TROGLO, PF_COLDBLOOD3, PF_STR_UP, PF_DEX_UP, NULL);
  break;

 case MUTCAT_PLANT:
  setvector(&ret,
PF_DISIMMUNE, PF_HEAVYSLEEPER, PF_BADHEARING, PF_FASTHEALER2, PF_BARK,
PF_THORNS, PF_LEAVES, PF_DEFORMED2, PF_PONDEROUS3, PF_STR_UP_2, NULL);
  break;

 case MUTCAT_SLIME:
  setvector(&ret,
PF_POISRESIST, PF_ROBUST, PF_CHEMIMBALANCE, PF_REGEN, PF_RADIOGENIC,
PF_DISIMMUNE, PF_POISONOUS, PF_SLIME_HANDS, PF_SMELLY2, PF_DEFORMED3,
PF_HOLLOW_BONES, PF_VOMITOUS, PF_HUNGER, PF_THIRST, PF_SORES, PF_TROGLO,
PF_WEBBED, PF_UNSTABLE, PF_RADIOACTIVE1, PF_SLIMY, PF_DEX_UP, PF_INT_UP, NULL);
  break;

 case MUTCAT_TROGLO:
  setvector(&ret,
PF_QUICK, PF_LIGHTEATER, PF_MYOPIC, PF_NIGHTVISION3, PF_INFRARED, PF_REGEN,
PF_DISIMMUNE, PF_POISONOUS, PF_SAPROVORE, PF_SLIT_NOSTRILS, PF_ALBINO,
PF_TROGLO3, PF_SLIMY, NULL);
  break;

 case MUTCAT_CEPHALOPOD:
  setvector(&ret,
PF_GILLS, PF_MOUTH_TENTACLES, PF_SLIT_NOSTRILS, PF_DEFORMED, PF_THIRST,
PF_BEAK, PF_SLIMY, PF_COLDBLOOD, PF_ARM_TENTACLES_8, PF_SHELL, PF_LEG_TENTACLES,
NULL);
  break;

 case MUTCAT_SPIDER:
  setvector(&ret,
PF_FLEET, PF_POISRESIST, PF_NIGHTVISION3, PF_INFRARED, PF_FUR, PF_CHITIN3,
PF_POISONOUS, PF_COMPOUND_EYES, PF_MANDIBLES, PF_WEB_WEAVER, PF_TROGLO,
PF_CARNIVORE, PF_COLDBLOOD, PF_DEX_UP_2, NULL);
  break;

 case MUTCAT_RAT:
  setvector(&ret,
PF_DISRESISTANT, PF_NIGHTVISION2, PF_FANGS, PF_FUR, PF_CLAWS, PF_TAIL_LONG,
PF_WHISKERS, PF_DEFORMED3, PF_VOMITOUS, PF_HUNGER, PF_TROGLO2, PF_GROWL, NULL);
  break;

 }

 return ret;
}

std::vector<std::string> category_dreams(mutation_category cat, int strength)
{
 std::vector<std::string> message;
 switch (strength)
{

 case 1:
  switch (cat)
  {
   case MUTCAT_LIZARD:
    setvector(&message, "You have a strange dream about lizards.", "Your dreams give you a strange scaly feeling.", NULL);
    break;
  
   case MUTCAT_BIRD:
    setvector(&message, "You have a strange dream about birds.", "Your dreams give you a strange feathered feeling.", NULL);
    break;
  
   case MUTCAT_FISH:
    setvector(&message, "You have a strange dream about fish.", "Your dreams give you a strange scaly feeling.", NULL);
    break;
  
   case MUTCAT_BEAST:
    setvector(&message, "You have a strange dream about animals.", "Your dreams give you a strange furry feeling.", NULL);
    break;
  
   case MUTCAT_CATTLE:
    setvector(&message, "You have a strange dream about cattle.", "Your dreams give you a strange furry feeling.", NULL);
    break;
  
   case MUTCAT_INSECT:
    setvector(&message, "You have a strange dream about insects.", "Your dreams give you a strange chitinous feeling.", NULL);
    break;
  
   case MUTCAT_PLANT:
    setvector(&message, "You have a strange dream about plants.", "Your dreams give you a strange plantlike feeling.", NULL);
    break;
  
   case MUTCAT_SLIME:
    setvector(&message, "You have a strange dream about slime.", "Your dreams give you a strange slimy feeling.", NULL);
    break;
  
   case MUTCAT_TROGLO:
    setvector(&message, "You have a strange dream about living in a cave.", "Your dreams give a strange reclusive feeling.", NULL);
    break;
  
   case MUTCAT_CEPHALOPOD:
    setvector(&message, "You have a strange dream about sea creatures.", "Your dreams give you a strange wet feeling.", NULL);
    break;
  
   case MUTCAT_SPIDER:
    setvector(&message, "You have a strange dream about spiders.", "Your dreams give you a strange chitinous feeling.", NULL);
    break;
  
   case MUTCAT_RAT:
    setvector(&message, "You have a strange dream about rats.", "Your dreams give you a strange furry feeling.", NULL);
    break;
	
   default:
    setvector(&message, "You have a peaceful dream", NULL);
  
  }
  break;

 case 2:
  switch (cat)
  {
   case MUTCAT_LIZARD:
    setvector(&message, "You have a disturbing dream of bathing in the sun.", "While dreaming, you see a distinctively lizard-like reflection of yourself.", NULL);
    break;
  
   case MUTCAT_BIRD:
    setvector(&message, "You have strange dreams of soaring through the sky.", "In a dream, you see a curiously birdlike reflection of yourself.", NULL);
    break;
  
   case MUTCAT_FISH:
    setvector(&message, "You dream of swimming in the open ocean", "In your dream, you see a strangely fishlike image of yourself.", NULL);
    break;
  
   case MUTCAT_BEAST:
    setvector(&message, "You have a vivid dream of hunting in the woods.", "Whilst dreaming, you see a disturbingly bestial version of yourself.", NULL);
    break;
  
   case MUTCAT_CATTLE:
    setvector(&message, "You dream of grazing in an open field.", "In a dream you catch a glimpse of a strangely cattle-like image of yourself.", NULL);
    break;
  
   case MUTCAT_INSECT:
    setvector(&message, "You have a dream of working in a hive.", "While dreaming you look into a mirror and see a frighteningly insectoid reflection.", NULL);
    break;
  
   case MUTCAT_PLANT:
    setvector(&message, "You have a confusing dream of growing in a garden.", "You are confused by a plantlike image of yourself in a dream.", NULL);
    break;
  
   case MUTCAT_SLIME:
    setvector(&message, "You have a strange dream of living in sludge.", "In your dream you see an odd, slimy reflection of yourself.", NULL);
    break;
  
   case MUTCAT_TROGLO:
    setvector(&message, "You dream of living deep inside a dark cave.", "You dream of being a primitive cave dweller.", NULL);
    break;
  
   case MUTCAT_CEPHALOPOD:
    setvector(&message, "You dream of living on the ocean floor.", "While dreaming, a reflection of yourself appears almost like an octopus.", NULL);
    break;
  
   case MUTCAT_SPIDER:
    setvector(&message, "You have a strange dream of spinning webs", "In your dream you see a very spiderlike version of yourself.", NULL);
    break;
  
   case MUTCAT_RAT:
    setvector(&message, "You vividly dream of living in the sewers.", "In your dream you see a group of rats that look almost like yourself.", NULL);
    break;
	
   default:
    setvector(&message, "You have a peaceful dream", NULL);
  
  }
  break;

 case 3:
  switch (cat)
  {
   case MUTCAT_LIZARD:
    setvector(&message, "You are terrified by a dream of becoming a lizard hybrid.", "You have a disturbingly lifelike dream of living as a lizard.", NULL);
    break;
  
   case MUTCAT_BIRD:
    setvector(&message, "You have a realistic dream of flying south with your flock for the winter.", "You are disturbed when your birdlike dreams are more lifelike than reality.", NULL);
    break;
  
   case MUTCAT_FISH:
    setvector(&message, "You have a a disturbing dream of swimming with a school of fish.", "When you wake up you panic about drowning from being out of water.", NULL);
    break;
  
   case MUTCAT_BEAST:
    setvector(&message, "You vividly dream of running with your pack, hunting a wild animal.", "A terrifyingly real dream has you killing game with your bare teeth.", NULL);
    break;
  
   case MUTCAT_CATTLE:
    setvector(&message, "You terrifyingly dream of being led to a slaughterhouse by a farmer.", "You find it hard to wake from a vivid dream of grazing on a farm.", NULL);
    break;
  
   case MUTCAT_INSECT:
    setvector(&message, "You are terrified by a dream of serving the hive queen mindlessly.", "Your disturbingly lifelike dream has you pollinating plants.", NULL);
    break;
  
   case MUTCAT_PLANT:
    setvector(&message, "You have a confusing and scary dream of becoming a plant.", "You are shocked by the transition from your plantlike dreams to reality.", NULL);
    break;
  
   case MUTCAT_SLIME:
    setvector(&message, "Your vivid dream of living as a slime blob frightens you.", "You find it hard to control your limbs after dreaming of amorphous blob life.", NULL);
    break;
  
   case MUTCAT_TROGLO:
    setvector(&message, "Your dream of living in the dark for years is almost real.", "You are frightened of the outside after your vivid dream of cave life.", NULL);
    break;
  
   case MUTCAT_CEPHALOPOD:
    setvector(&message, "Dreaming of living on the ocean floor seems more lifelike than reality.", "You dream of living as a terrifying octopus mutant.", NULL);
    break;
  
   case MUTCAT_SPIDER:
    setvector(&message, "You vividly dream of living in a web and consuming insects.", "Your dreams of fully turning into a spider frighten you.", NULL);
    break;
  
   case MUTCAT_RAT:
    setvector(&message, "You scream in fear while you dream of being chased by a cat", "Your lifelike dreams have you scavenging food with a pack of rats.", NULL);
    break;
	
   default:
    setvector(&message, "You have a peaceful dream", NULL);
  
  }
  break;
	
 default:
  setvector(&message, "You have a peaceful dream", NULL);

 } 
 return message;
}
