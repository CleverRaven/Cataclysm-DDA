| id                            | name                    | required_proficiencies | description |
| ----------------------------- | ----------------------- | ---------------------- | ----------- |
| `prof_scav_cooking`           | Scavenger Cooking       | None                   | You know the secrets of cooking tasty food using strange substitutions and weird things you found in a cabinet in an abandoned house. |
| `prof_forage_cooking`         | Forager Cooking         | None                   | Cooking with foraged ingredients that you wouldn't find in most cookbooks. |
| `prof_food_prep`              | Food Preparation        | None                   | You've done enough cooking to know the basics about how to quickly and efficiently sort your ingredients. |
| `prof_knife_skills`           | Knife Skills            | prof_food_prep         | You're quite skilled at culinary knife work, making you noticeably faster at preparing meals that involve a lot of chopping. |
| `prof_baking`                 | Principles of Baking    | None                   | You know the basics of baking - proportions, leavening, and things - and are less likely to make a dramatic mistake when working from memory. |
| `prof_baking_bread`           | Breadmaking             | prof_baking            | You've baked enough bread to consider yourself an expert.  May your yeast be ever frothy. |
| `prof_baking_desserts_1`      | Dessert Baking          | prof_baking            | Anyone can make a cookie, but you can make it look amazing.  Cakes and other fluffies are also no longer a barrier. |
| `prof_baking_desserts_2`      | Patisserie Expert       | prof_baking_desserts_1 | You could have been a professional baker before the Cataclysm.  Your profiteroles are the envy of the wastelands. |
| `prof_frying`                 | Principles of Frying    | None                   | Anyone can toss stuff into deep fryer oil.  You know more about selecting oil, keeping the food from burning, and the like. |
| `prof_frying_bread`           | Breading & Battering    | prof_frying            | You know enough about deep frying breads and batters to do it easily without thinking about it. |
| `prof_frying_dessert`         | Fried Desserts          | prof_frying / prof_baking_desserts_1 | You know the vaunted secrets of creating delicious deep fried sweets. |
| `prof_preservation`           | Food Preservation       | None                   | The basic knowledge of ways to keep food safe for long periods of time in a post-refrigeration world. |
| `prof_food_curing`            | Curing and Drying Food  | prof_preservation      | Using salt, smoke, and similar agents to cure or dehydrate food. |
| `prof_food_canning`           | Canning Food            | prof_preservation      | Using pressure canning or tinning to safely store food in sealed containers. |
| `prof_cheesemaking_1`         | Principles of Cheesemaking | None                | The basics of curdling and coagulating milk and similar things without the wrong kinds of bacteria and mold winning out. |
| `prof_cheesemaking_2`         | Cheesemaking Expert     | prof_cheesemaking_1    | Taking the basics of curdled milk and making it delicious. |
| `prof_fermenting`             | Culinary Fermentation   | None                   | The basics of fermenting for human consumption, including sanitary technique, avoiding harmful molds, and identifying common problems. |
| `prof_brewing`                | Brewing                 | prof_fermenting        | The fine art of brewing grains into beers. |
| `prof_winemaking`             | Winemaking              | prof_fermenting        | The knowledge of how to turn sugary fruits, and occasionally vegetables, into fermented beverages. |
| `prof_lactofermenting`        | Lactofermentation       | prof_fermenting        | The microbiology of most non-alcoholic fermented edibles, from sauerkraut to sourdough. |
| `prof_distilling`             | Distillation            | None                   | The process of separating two mixed liquids by their different boiling points. |
| `prof_beverage_distilling`    | Beverage Distillation   | prof_distilling        | Applying the science of distillation to alcoholic beverages to get something tasty. |
| `prof_intro_chemistry`        | Principles of Chemistry | None                   | You are beginning to grasp a general idea of how elements and compounds react with each other. |
| `prof_intro_biology`          | Principles of Biology   | None                   | You are beginning to gain a general idea of how various living beings function. |
| `prof_organic_chemistry`      | Organic Chemistry       | prof_intro_chemistry   | Knowledge of the branch of chemistry that studies and uses carbon-containing compounds. |
| `prof_inorganic_chemistry`    | Inorganic Chemistry     | prof_intro_chemistry   | Knowledge of the branch of chemistry that studies and uses compounds not containing carbon. |
| `prof_biochemistry`           | Biochemistry            | prof_organic_chemistry / prof_intro_biology | The understanding of the chemistry of living things. |
| `prof_physiology`             | Physiology              | prof_intro_biology     | An in-depth understanding of how humans and animals operate. |
| `prof_xenology`               | Xenology                | prof_intro_biology     | You are beginning to grasp a general idea of how alien and post-Cataclysm mutated creatures function and operate. |
| `prof_intro_chem_synth`       | Principles of Chemical Synthesis | prof_intro_chemistry | You're beginning to grasp the basics of chemical production. |
| `prof_chem_synth`             | Complex Chemical Synthesis | prof_intro_chem_synth      | The proper understanding of safe chemical synthesis and how to clean your tools used for the next batch. |
| `prof_pharmaceutical`         | Pharmaceutical Production. | prof_intro_chemistry / prof_intro_biology | An understanding of how to produce various natural and chemical substances pure enough for human consumption. |
| `prof_metallurgy`             | Metallurgy              | None                   | An understanding of the chemistry of metalworking and alloys. |
| `prof_fibers`                 | Fiber Twisting          | None                   | You've got a basic grasp of the theory of how to twist and arrange fibers into a more durable cord. |
| `prof_fibers_rope`            | Ropemaking              | prof_fibers            | Making twisted fibers into durable and useful rope. |
| `prof_spinning`               | Spinning                | None                   | The art of spinning fiber into thread. |
| `prof_weaving`                | Weaving                 | None                   | You've learned how to use a loom to produce sheets of fabric from thread. |
| `prof_knitting`               | Knitting                | None                   | Knitting, one of the oldest ways of tying knots in fiber until it is fabric. |
| `prof_knitting_speed`         | Speed Knitting          | prof_knitting          | You have knitted and knitted and knitted some more, and have the consistent string tension to show for it.  When you get going, the clicking of your needles is like a metronome. |
| `prof_elastics`               | Stretch Fabric          | None                   | Making elastic bands is easy enough, but making a whole garment from stretchy cloth takes a bit of finesse.  You're good at it. |
| `prof_polymerworking`         | Advanced polymer sewing | None                   | You know the tricks for working with Kevlar, Nomex, and other advanced polymer cloth. |
| `prof_basketweaving`          | Basketweaving           | None                   | Forming coarse fibers into shapes. |
| `prof_articulation`           | Joint Articulation      | None                   | Making flexible items out of hard materials is tricky.  You've done enough of it to understand how to make smoothly articulated joints out of rigid or semi-rigid materials. |
| `prof_chain_armour`           | Chain Garments          | None                   | Turning sheets of chain links into effective, wearable clothing that doesn't bind up. |
| `prof_tanning_basic`          | Basic Tanning           | None                   | You're familiar with the theory behind turning raw hides into leather. |
| `prof_tanning`                | Tanning                 | prof_tanning_basic     | You have a lot of practice and experience with processing hides to leather, as well as related techniques like making boiled leather. |
| `prof_leatherworking_basic`   | Principles of Leatherworking | None              | You've got a basic familiarity with how to work with leather, furs, hides, and similar materials. |
| `prof_leatherworking`         | Leatherworking          | prof_leatherworking_basic | Working with leather requires a specific set of skills and tools… a set you are familiar with. |
| `prof_furriery`               | Furriery                | prof_leatherworking_basic | Working with fur and faux fur is a skill all its own. |
| `prof_millinery`              | Millinery               | prof_leatherworking_basic | Like a solitary mouse, you've learned how to construct hats by hand. |
| `prof_chitinworking`          | Chitinworking           | prof_leatherworking_basic | It seems unlikely that anyone prior to the cataclysm knew how to work with the shells of giant bugs as well as you do now. |
| `prof_closures`               | Garment Closures        | None                   | You are familiar with the adjustments and tricks needed to make properly functional buttons, zippers, and other closures on garments. |
| `prof_closures_waterproofing` | Fabric Waterproofing    | None                   | You know how to making garments and fabric containers sealed against water. |
| `prof_cobbling`               | Cobbling                | prof_closures / prof_leatherworking_basic | Like a magical elf, you've learned how to construct footwear by hand. |
| `prof_glassblowing`           | Glassblowing            | None                   | Working with glass and heat without poisoning or perforating yourself. |
| `prof_plumbing`               | Plumbing                | None                   | Working with pipes and pipe fittings to transport fluids without leakage. |
| `prof_plasticworking`         | Plastic Working         | None                   | Working with plastic using your hands, including carving, molding, and gluing it.  You know how to identify thermoplastics that are suitable for mold casting, and how to identify the right plastic for the right job. |
| `prof_carving`                | Carving                 | None                   | Shaping wood, bone, and similar materials with a cutting implement. |
| `prof_carpentry_basic`        | Basic Carpentry         | None                   | Simple projects involving holding planks and panels of wood together with nails or similar fasteners. |
| `prof_pottery`                | Pottery                 | None                   | Basic pottery, from shaping to working with slip to fuse pieces. |
| `prof_pottery_glazing`        | Pottery Glazing         | None                   | Adding glazes to pottery to make them waterproof. |
| `prof_knapping`               | Basic Knapping          | None                   | You know the basic principles of turning stones into more useful tools. |
| `prof_knapping_speed`         | Knapping                | prof_knapping          | You've banged rocks together so much and for so long, you've become extremely fast at it. |
| `prof_bowyery`                | Bowyery                 | None                   | The ability to make durable, effective bows. |
| `prof_fletching`              | Fletching               | None                   | The skill involved in making arrows that fly true. |
| `prof_bow_basic`              | Basic Archer's Form     | None                   | You have grasped some basic understanding on archery, and find handling bows easier. |
| `prof_bow_expert`             | Expert Archer's Form    | prof_bow_basic         | After significant practice, you have become an expert in the movements used in drawing and firing a bow, and have strengthened those muscles considerably. |
| `prof_bow_master`             | Master Archer's Form    | prof_bow_expert        | You have drawn and fired a bow so many times, you probably have a bit of a twist in your spine from it.  You've developed what would once have been a world class understanding of the pose and motion of effective archery. |
| `prof_helicopter_pilot`       | Helicopter Piloting     | None                   | Before the cataclysm, you were a helicopter pilot.  Now, you just hope you can fly again. |
| `prof_aircraft_mechanic`      | Airframe and Powerplant Mechanic | None          | You've been trained and certified to repair aircraft.  You're not really sure how that could help you survive now. |
| `prof_elec_soldering`         | Electronics Soldering   | None                   | An understanding of the skills and tools needed to create durable, effective soldered electrical connections. |
| `prof_electromagnetics`       | Electromagnetics        | None                   | A practical working knowledge of electromagnetic fields and their creation and application. |
| `prof_shock_weapons`          | Electroshock Weapons    | None                   | An understanding of how to use high voltage, low-current devices to cause debilitating muscle spasms.  Includes some theoretical knowledge of how to protect yourself from these. |
| `prof_metalworking`           | Principles of Metalworking | None                | A basic understanding of the properties of metal as a material, and the concepts behind smithing, die casting, and other metalworking techniques. |
| `prof_welding_basic`          | Principles of Welding   | prof_metalworking      | A basic understanding of the different types of welding, welding tools and fuels, how to weld different materials, and more. |
| `prof_welding`                | Welding                 | prof_welding_basic     | You are an experienced welder. |
| `prof_blacksmithing`          | Blacksmithing           | prof_metalworking      | The craft of working metal into tools and other items of use. |
| `prof_redsmithing`            | Redsmithing             | prof_metalworking      | Working copper and bronze shares a lot of techniques with blacksmithing, but the more ductile copper-containing metals have a trick all their own. |
| `prof_fine_metalsmithing`     | Fine Metalsmithing      | None                   | How to make jewelry from precious metals like gold and silver. |
| `prof_gem_setting`            | Gem Setting             | prof_fine_metalsmithing / prof_redsmithing | How to add gemstones to jewelry. |
| `prof_armorsmithing`          | Armorsmithing           | prof_blacksmithing     | How to make articulated armor from pieces of metal. |
| `prof_bladesmith`             | Bladesmithing           | prof_blacksmithing     | How to fabricate sharp and reliable blades from scratch. |
| `prof_toolsmithing`           | Manual Tooling          | prof_blacksmithing     | How to make high-quality tools and parts by hand.  Includes techniques like threading, durable articulation points, and using appropriate metals for appropriate tasks.  Also applies to making blunt instruments that can withstand a severe beating without distortion. |
| `prof_wiremaking`             | Wire Making             | prof_metalworking      | How to turn raw ingots and metals into usable wire.  Includes both drawing and extruding. |
| `prof_handloading`            | Handloading             | None                   | You know how to accurately measure powder and projectile weights for reloading firearm cartridges. |
| `prof_gunsmithing_basic`      | Principles of Gunsmithing | None                 | A basic understanding of how guns are put together and what tools and materials are needed for the job. |
| `prof_gunsmithing_improv`     | Improvised Gunmaking    | None                   | You've become an expert at putting together guns and launchers from makeshift parts. |
| `prof_gunsmithing_antique`    | Antique Gunsmithing     | prof_metalworking / prof_gunsmithing_basic | You're specifically skilled at building and repairing antique guns. |
| `prof_gunsmithing_spring`     | Spring-powered Guns and Crossbows | None         | Similar to bowyery, the art of making guns that are powered by elastic mechanisms. |
| `prof_gunsmithing_revolver`   | Revolver Gunsmithing    | prof_gunsmithing_basic | You've got the know-how to make a classic Western revolver, and its many variants. |
| `prof_lockpicking`            | Lockpicking             | None                   | You know how to pick a lock, at least in theory. |
| `prof_lockpicking_expert`     | Locksmith               | prof_lockpicking       | You can pick a lock in the dark, blindfolded.  Although that doesn't make it all that much harder.  You're good at locks, OK? |
| `prof_safecracking`           | Safecracking            | None                   | Opening safes is an exacting skill, one that you are proficient at. |
| `prof_traps`                  | Principles of Trapping  | None                   | You know the basics of setting and taking down traps safely. |
| `prof_trapsetting`            | Trap Setting            | prof_traps             | You're specifically quite skilled at setting effective traps. |
| `prof_disarming`              | Trap Disarming          | prof_traps             | You know how to take down a trap safely. |
| `prof_spotting`               | Spotting and Awareness  | None                   | You are skilled at spotting things out of the ordinary, like traps or ambushes. |
| `prof_parkour`                | Parkour Expert          | None                   | You're skilled at clearing obstacles; terrain like railings or counters are as easy for you to move on as solid ground. |
| `prof_wound_care`             | Wound Care              | None                   | You know how to bandage wounds and understand basic principles of wound care. |
| `prof_wound_care_expert`      | Wound Care Expert       | prof_wound_care        | Your extensive field experience in bandaging and wound care is on par with that of a paramedic. |

## Magiclysm proficiencies

| id                           | name                   | required_proficiencies           | description |
| ---------------------------- | ---------------------- | -------------------------------- | ----------- |
| `prof_alchemy`               | Alchemy                | None                             | You know the basics of manipulating the mana of objects through application of chemical laws. |
| `prof_almetallurgy`          | Almetallurgy           | prof_alchemy / prof_metalworking | The forging of magical alloys is a complex process requiring an understanding of both mundane metals and alchemy. |
| `prof_leatherworking_dragon` | Dragon leather working | prof_leatherworking              | Working with dragon leather requires a specific set of skills and tools… a set you are familiar with. |
| `prof_scaleworking_dragon`   | Dragon scale working   | prof_leatherworking_dragon       | Working with dragon scales requires a specific set of skills and tools… a set you are familiar with. |
| `prof_golemancy_basic`       | Basic Golemancy        | None                             | Infusing shaped material with your will and the ability to move is hard but you're starting to get it. |

This data was extracted using tools/json_tools/table.py as follows (then formatted):

```bash
tools/json_tools/table.py --type=proficiency id name description required_proficiencies
```
