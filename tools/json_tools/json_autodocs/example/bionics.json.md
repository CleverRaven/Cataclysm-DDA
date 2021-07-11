
## bionics.json:

### act_cost 
 ```
 str value, examples: 5 J, 10 kJ
 Optional 
```

 ### active_flags 

 ```
 list see "active_flags:#"
 Optional 
```


 <details> 
 <summary> children of active_flags </summary> 

 ### active_flags:# 

 ```
 str value, examples: BLIND, DEAF
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### bash_protec 

 ```
 list see "bash_protec:#"
 Optional 
```


 <details> 
 <summary> children of bash_protec </summary> 

 ### bash_protec:# 

 ```
 list see "bash_protec:#:#"
 Required 
```


 <details> 
 <summary> children of bash_protec:# </summary> 

 ### bash_protec:#:# 

 ```
 str value, examples: torso, arm_l
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### bullet_protec 

 ```
 list see "bullet_protec:#"
 Optional 
```


 <details> 
 <summary> children of bullet_protec </summary> 

 ### bullet_protec:# 

 ```
 list see "bullet_protec:#:#"
 Required 
```


 <details> 
 <summary> children of bullet_protec:# </summary> 

 ### bullet_protec:#:# 

 ```
 str value, examples: torso, arm_l
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### canceled_mutations 

 ```
 list see "canceled_mutations:#"
 Optional 
```


 <details> 
 <summary> children of canceled_mutations </summary> 

 ### canceled_mutations:# 

 ```
 str value, example: HYPEROPIC
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### capacity 

 ```
 str value, examples: 100 kJ, 250 kJ
 Optional 
```


 ### cut_protec 

 ```
 list see "cut_protec:#"
 Optional 
```


 <details> 
 <summary> children of cut_protec </summary> 

 ### cut_protec:# 

 ```
 list see "cut_protec:#:#"
 Required 
```


 <details> 
 <summary> children of cut_protec:# </summary> 

 ### cut_protec:#:# 

 ```
 str value, examples: torso, arm_l
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
 str value, examples: A stimulator system has been surgically implanted alongside your adrenal glands, allowing you to trigger your body's adrenaline response at the cost of some bionic power., A thin forcefield surrounds your body, continually draining power.  Anything attempting to penetrate this field has a chance of being deflected at the cost of energy, reducing their ability to deal damage.  Bullets will be deflected more than melee weapons and those in turn more than massive objects.
 Required 
```


 ### enchantments 

 ```
 list see "enchantments:#"
 Optional 
```


 <details> 
 <summary> children of enchantments </summary> 

 ### enchantments:# 

 ```
 str value, examples: ENCH_INVISIBILITY, ENCH_SHADOW_CLOUD
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### encumbrance 

 ```
 list see "encumbrance:#"
 Optional 
```


 <details> 
 <summary> children of encumbrance </summary> 

 ### encumbrance:# 

 ```
 list see "encumbrance:#:#"
 Required 
```


 <details> 
 <summary> children of encumbrance:# </summary> 

 ### encumbrance:#:# 

 ```
 str value, examples: torso, arm_l
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### env_protec 

 ```
 list see "env_protec:#"
 Optional 
```


 <details> 
 <summary> children of env_protec </summary> 

 ### env_protec:# 

 ```
 list see "env_protec:#:#"
 Required 
```


 <details> 
 <summary> children of env_protec:# </summary> 

 ### env_protec:#:# 

 ```
 str value, examples: eyes, mouth
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### exothermic_power_gen 

 ```
 bool value, example: True
 Optional 
```


 ### fake_item 

 ```
 str value, examples: bio_blade_weapon, bio_shotgun_gun
 Optional 
```


 ### flags 

 ```
 list see "flags:#"
 Optional 
```


 <details> 
 <summary> children of flags </summary> 

 ### flags:# 

 ```
 str value, examples: BIONIC_TOGGLED, BIONIC_NPC_USABLE
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### fuel_capacity 

 ```
 int value, examples: 500, 2000
 Optional 
```


 ### fuel_efficiency 

 ```
 int value, examples: 1, 0.25
 Optional 
```


 ### fuel_options 

 ```
 list see "fuel_options:#"
 Optional 
```


 <details> 
 <summary> children of fuel_options </summary> 

 ### fuel_options:# 

 ```
 str value, examples: battery, alcohol
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: bio_adrenaline, bio_ads
 Required 
```


 ### included 

 ```
 bool value, example: True
 Optional 
```


 ### included_bionics 

 ```
 list see "included_bionics:#"
 Optional 
```


 <details> 
 <summary> children of included_bionics </summary> 

 ### included_bionics:# 

 ```
 str value, examples: bio_earplugs, bio_blindfold
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### is_remote_fueled 

 ```
 bool value, example: True
 Optional 
```


 ### name 

 ```
 dict see "name:" values
 Required 
```


 <details> 
 <summary> children of name </summary> 

 ### name:str 

 ```
 str value, examples: Adrenaline Pump, Active Defense System
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### occupied_bodyparts 

 ```
 list see "occupied_bodyparts:#"
 Optional 
```


 <details> 
 <summary> children of occupied_bodyparts </summary> 

 ### occupied_bodyparts:# 

 ```
 list see "occupied_bodyparts:#:#"
 Required 
```


 <details> 
 <summary> children of occupied_bodyparts:# </summary> 

 ### occupied_bodyparts:#:# 

 ```
 str value, examples: torso, head
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### passive_fuel_efficiency 

 ```
 float value, example: 0.00625
 Optional 
```


 ### react_cost 

 ```
 int value, examples: 1, 5 J
 Optional 
```


 ### social_modifiers 

 ```
 dict, see "social_modifiers:" values
 Optional 
```


 <details> 
 <summary> children of social_modifiers </summary> 

 ### social_modifiers:intimidate 

 ```
 int value, examples: 10, 20
 Required 
```



 ### social_modifiers:lie 

 ```
 int value, examples: 20, 10
 Optional 
```



 ### social_modifiers:persuade 

 ```
 int value, examples: -50, 10
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### stat_bonus 

 ```
 list see "stat_bonus:#"
 Optional 
```


 <details> 
 <summary> children of stat_bonus </summary> 

 ### stat_bonus:# 

 ```
 list see "stat_bonus:#:#"
 Required 
```


 <details> 
 <summary> children of stat_bonus:# </summary> 

 ### stat_bonus:#:# 

 ```
 str value, examples: DEX, PER
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### time 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### type 

 ```
 str value, example: bionic
 Required 
```


 ### vitamin_absorb_mod 

 ```
 float value, example: 1.5
 Optional 
```


 ### weight_capacity_bonus 

 ```
 str value, example: 20 kg
 Optional 
```


