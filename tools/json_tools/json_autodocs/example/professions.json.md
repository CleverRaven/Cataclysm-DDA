
## professions.json:

### addictions 
 ```
 list see "addictions:#"
 Optional 
```

 <details> 
 <summary> children of addictions </summary> 

 ### addictions:# 

 ```
 dict see "addictions:#:" values
 Required 
```


 <details> 
 <summary> children of addictions:# </summary> 

 ### addictions:#:intensity 

 ```
 int value, examples: 20, 10
 Required 
```



 ### addictions:#:type 

 ```
 str value, examples: nicotine, opiate
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### bonus 

 ```
 dict, see "bonus:" values
 Optional 
```


 <details> 
 <summary> children of bonus </summary> 

 ### bonus:absent 

 ```
 list see "bonus:absent:#"
 Optional 
```


 <details> 
 <summary> children of bonus:absent </summary> 

 ### bonus:absent:# 

 ```
 str value, examples: HYPEROPIC, MYOPIC
 Required 
```



 ### bonus:present 

 ```
 list see "bonus:present:#"
 Required 
```


 <details> 
 <summary> children of bonus:present </summary> 

 ### bonus:present:# 

 ```
 str value, examples: HYPEROPIC, MYOPIC
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### description 

 ```
 str value, examples: Circumstance left you wandering the world, alone.  Now there is nothing to go back to, even if you wanted to.  Perhaps your experience in fending for yourself will prove useful in this new world., Some would say that there's nothing particularly notable about you, but you've survived, and that's more than most could say right now.
 Optional 
```


 ### entries 

 ```
 list see "entries:#"
 Optional 
```


 <details> 
 <summary> children of entries </summary> 

 ### entries:# 

 ```
 dict see "entries:#:" values
 Required 
```


 <details> 
 <summary> children of entries:# </summary> 

 ### entries:#:ammo-item 

 ```
 str value, examples: battery, 9mmfmj
 Optional 
```



 ### entries:#:charges 

 ```
 int value, examples: 12, 30
 Required 
```



 ### entries:#:container-item 

 ```
 str value, examples: cell_phone, laptop
 Optional 
```



 ### entries:#:item 

 ```
 str value, examples: light_plus_battery_cell, light_disposable_cell
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### flags 

 ```
 list see "flags:#"
 Optional 
```


 <details> 
 <summary> children of flags </summary> 

 ### flags:# 

 ```
 str value, examples: SCEN_ONLY, NO_BONUS_ITEMS
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### group 

 ```
 dict, see "group:" values
 Optional 
```


 <details> 
 <summary> children of group </summary> 

 ### group:entries 

 ```
 list see "group:entries:#"
 Optional 
```


 <details> 
 <summary> children of group:entries </summary> 

 ### group:entries:# 

 ```
 dict see "group:entries:#:" values
 Required 
```


 <details> 
 <summary> children of group:entries:# </summary> 

 ### group:entries:#:ammo-item 

 ```
 str value, example: albuterol
 Required 
```



 ### group:entries:#:charges 

 ```
 int value, example: 100
 Required 
```



 ### group:entries:#:item 

 ```
 str value, example: inhaler
 Required 
```



 ### group:items 

 ```
 list see "group:items:#"
 Optional 
```


 <details> 
 <summary> children of group:items </summary> 

 ### group:items:# 

 ```
 str value, examples: glasses_eye, glasses_bifocal
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: army_mags_m4, army_mags_mp5
 Optional 
```


 ### item 

 ```
 str value, examples: sunglasses, fancy_sunglasses
 Optional 
```


 ### items 

 ```
 dict, see "items:" values
 Optional 
```


 <details> 
 <summary> children of items </summary> 

 ### items:both 

 ```
 list see "items:both:#"
 Required 
```


 <details> 
 <summary> children of items:both </summary> 

 ### items:both:# 

 ```
 str value, examples: striped_shirt, karate_gi
 Optional 
```



 ### items:both:ammo 

 ```
 int value, example: 100
 Optional 
```



 ### items:both:entries 

 ```
 list see "items:both:entries:#"
 Optional 
```


 <details> 
 <summary> children of items:both:entries </summary> 

 ### items:both:entries:# 

 ```
 dict see "items:both:entries:#:" values
 Required 
```


 <details> 
 <summary> children of items:both:entries:# </summary> 

 ### items:both:entries:#:ammo-item 

 ```
 str value, examples: battery, 9mm
 Optional 
```



 ### items:both:entries:#:charges 

 ```
 int value, examples: 100, 30
 Optional 
```



 ### items:both:entries:#:container-item 

 ```
 str value, examples: sheath, holster
 Optional 
```



 ### items:both:entries:#:contents-group 

 ```
 str value, examples: army_mags_m4, army_mags_usp45 

-OR-

 list, see items:both:entries:#:contents-group:#
 Optional 
```


 <details> 
 <summary> children of items:both:entries:#:contents-group </summary> 

 ### items:both:entries:#:contents-group:# 

 ```
 str value, example: army_mags_mp5
 Optional 
```



 ### items:both:entries:#:contents-item 

 ```
 str value, examples: shoulder_strap, machete 

-OR-

 list, see items:both:entries:#:contents-item:#
 Optional 
```


 <details> 
 <summary> children of items:both:entries:#:contents-item </summary> 

 ### items:both:entries:#:contents-item:# 

 ```
 str value, examples: shoulder_strap, flashbang
 Optional 
```



 ### items:both:entries:#:count 

 ```
 int value, examples: 2, 3
 Optional 
```



 ### items:both:entries:#:custom-flags 

 ```
 list see "items:both:entries:#:custom-flags:#"
 Optional 
```


 <details> 
 <summary> children of items:both:entries:#:custom-flags </summary> 

 ### items:both:entries:#:custom-flags:# 

 ```
 str value, examples: no_auto_equip, auto_wield
 Required 
```



 ### items:both:entries:#:group 

 ```
 str value, examples: charged_cell_phone, charged_two_way_radio
 Optional 
```



 ### items:both:entries:#:item 

 ```
 str value, examples: ear_plugs, lighter
 Optional 
```



 ### items:both:entries:#:snippets 

 ```
 list see "items:both:entries:#:snippets:#"
 Optional 
```


 <details> 
 <summary> children of items:both:entries:#:snippets </summary> 

 ### items:both:entries:#:snippets:# 

 ```
 str value, example: basin
 Required 
```



 ### items:both:items 

 ```
 list see "items:both:items:#"
 Optional 
```


 <details> 
 <summary> children of items:both:items </summary> 

 ### items:both:items:# 

 ```
 str value, examples: pants, jeans
 Required 
```



 ### items:both:magazine 

 ```
 int value, example: 100
 Optional 
```



 ### items:female 

 ```
 list see "items:female:#"
 Optional 
```


 <details> 
 <summary> children of items:female </summary> 

 ### items:female:# 

 ```
 str value, examples: bra, sports_bra
 Required 
```



 ### items:male 

 ```
 list see "items:male:#"
 Optional 
```


 <details> 
 <summary> children of items:male </summary> 

 ### items:male:# 

 ```
 str value, examples: boxer_shorts, briefs
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### name 

 ```
 str value, examples: Vagabond, Survivor 

-OR-

 dict, see name:
 Optional 
```


 <details> 
 <summary> children of name </summary> 

 ### name:female 

 ```
 str value, examples: Maid, Captive
 Optional 
```



 ### name:male 

 ```
 str value, examples: Butler, Captive
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### pets 

 ```
 list see "pets:#"
 Optional 
```


 <details> 
 <summary> children of pets </summary> 

 ### pets:# 

 ```
 dict see "pets:#:" values
 Required 
```


 <details> 
 <summary> children of pets:# </summary> 

 ### pets:#:amount 

 ```
 int value, examples: 1, 5
 Required 
```



 ### pets:#:name 

 ```
 str value, examples: mon_dog_gshepherd, mon_cat
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### points 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### proficiencies 

 ```
 list see "proficiencies:#"
 Optional 
```


 <details> 
 <summary> children of proficiencies </summary> 

 ### proficiencies:# 

 ```
 str value, examples: prof_gunsmithing_basic, prof_fibers
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### skills 

 ```
 list see "skills:#"
 Optional 
```


 <details> 
 <summary> children of skills </summary> 

 ### skills:# 

 ```
 dict, see "skills:#:" values
 Optional 
```


 <details> 
 <summary> children of skills:# </summary> 

 ### skills:#:level 

 ```
 int value, examples: 2, 1
 Required 
```



 ### skills:#:name 

 ```
 str value, examples: gun, speech
 Required 
```



 ### skills:level 

 ```
 int value, example: 2
 Optional 
```



 ### skills:name 

 ```
 str value, examples: melee, survival
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### sub 

 ```
 list see "sub:#"
 Optional 
```


 <details> 
 <summary> children of sub </summary> 

 ### sub:# 

 ```
 dict see "sub:#:" values
 Required 
```


 <details> 
 <summary> children of sub:# </summary> 

 ### sub:#:absent 

 ```
 list see "sub:#:absent:#"
 Optional 
```


 <details> 
 <summary> children of sub:#:absent </summary> 

 ### sub:#:absent:# 

 ```
 str value, examples: MEATARIAN, ANTIWHEAT
 Required 
```



 ### sub:#:item 

 ```
 str value, example: blazer
 Optional 
```



 ### sub:#:new 

 ```
 list see "sub:#:new:#"
 Required 
```


 <details> 
 <summary> children of sub:#:new </summary> 

 ### sub:#:new:# 

 ```
 str value, examples: fitover_sunglasses, veggy_salad 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of sub:#:new:# </summary> 

 ### sub:#:new:#:item 

 ```
 str value, examples: fchicken, hat_cotton
 Optional 
```



 ### sub:#:new:#:ratio 

 ```
 int value, examples: 1, 2
 Optional 
```



 ### sub:#:present 

 ```
 list see "sub:#:present:#"
 Optional 
```


 <details> 
 <summary> children of sub:#:present </summary> 

 ### sub:#:present:# 

 ```
 str value, examples: ANTIWHEAT, ANTIFRUIT
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### subtype 

 ```
 str value, example: collection
 Optional 
```


 ### trait 

 ```
 str value, example: WOOLALLERGY
 Optional 
```


 ### traits 

 ```
 list see "traits:#"
 Optional 
```


 <details> 
 <summary> children of traits </summary> 

 ### traits:# 

 ```
 str value, examples: PROF_MED, PROF_POLICE
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, examples: profession, item_group
 Required 
```


 ### vehicle 

 ```
 str value, examples: semi_truck, motorcycle
 Optional 
```


