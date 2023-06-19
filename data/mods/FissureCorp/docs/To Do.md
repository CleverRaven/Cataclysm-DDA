## To Do

### MVP
Melee system + 1 enemy

### Needs Infrastructure
Needs support in C++, need to add EOC infrastructure for it to be possible at all.
* For [[The Idea#Melee]] I need EOC selectors. Need to be able to pop up a menu with N options, have the player select one, and trigger an EOC based on that selection.
* For [[The Idea#Melee]] I need EOC to trigger a melee attack, that you select.
* For [[The Idea#Melee]] I need EOC to get all valid melee attacks / get one of your valid melee attacks (based on weight). 
* For [[The Idea#The enemies]] I need templates for monsters. So that monsters can stay unique.
* For [[The Idea#The enemies]] I probably need some general infra, to support monsters only attacking when the player does. I also probably will need it for the player. Ideally standard attacking can be disabled and it will just be attacking through the new system.
* In general I'd like EOC keybinds, so that mods can add specific keybinds that can trigger EOCs of your choice. 

### Needs Implementing
Needs development time just on the JSON side (mod only)
* [[The Idea#Guns]] just need to add them.
* [[The Idea#Research]] just need to add all the resource types to spend and internal tracking vars.
* [[The Idea#Subsystems]].
* Create [[The Idea#The enemies]].
* Create [[The Idea#Melee]] system. 
* Creating the spawning system for enemies, anomalies, and quests.
* Sprite work. Ultica? or Neodays. 

### Needs Investigating
* [[The Idea#The world]] How is this going to work? Can I remove roads in JSON from cities? What do I need to do to make that happen? Should I just use nested mapgen for it.



### Melee
* Generalize martial arts to weapons.
* Check for non wielded weapons techniques.