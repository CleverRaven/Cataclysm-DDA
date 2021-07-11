
## techniques.json:

### aoe 
 ```
 str value, examples: spin, wide
 Optional 
```

 ### block_counter 

 ```
 bool value, example: True
 Optional 
```


 ### crit_ok 

 ```
 bool value, example: True
 Optional 
```


 ### crit_tec 

 ```
 bool value, example: True
 Optional 
```


 ### defensive 

 ```
 bool value, example: True
 Optional 
```


 ### description 

 ```
 str value, examples: Unwield target's weapon, Medium blocking ability
 Optional 
```


 ### disarms 

 ```
 bool value, example: True
 Optional 
```


 ### dodge_counter 

 ```
 bool value, example: True
 Optional 
```


 ### down_dur 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### downed_target 

 ```
 bool value, example: True
 Optional 
```


 ### dummy 

 ```
 bool value, example: True
 Optional 
```


 ### flat_bonuses 

 ```
 list see "flat_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of flat_bonuses </summary> 

 ### flat_bonuses:# 

 ```
 dict see "flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of flat_bonuses:# </summary> 

 ### flat_bonuses:#:scale 

 ```
 float value, examples: 1.0, 100
 Required 
```



 ### flat_bonuses:#:scaling-stat 

 ```
 str value, examples: str, per
 Required 
```



 ### flat_bonuses:#:stat 

 ```
 str value, examples: arpen, movecost
 Required 
```



 ### flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### grab_break 

 ```
 bool value, example: True
 Optional 
```


 ### human_target 

 ```
 bool value, example: True
 Optional 
```


 ### id 

 ```
 str value, examples: tec_none, WBLOCK_1
 Required 
```


 ### knockback_dist 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### knockback_follow 

 ```
 bool value, example: True
 Optional 
```


 ### knockback_spread 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### melee_allowed 

 ```
 bool value, example: True
 Optional 
```


 ### messages 

 ```
 list see "messages:#"
 Optional 
```


 <details> 
 <summary> children of messages </summary> 

 ### messages:# 

 ```
 str value, examples: You send %s reeling, You throw a heavy cross at %s
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### miss_recovery 

 ```
 bool value, example: True
 Optional 
```


 ### mult_bonuses 

 ```
 list see "mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of mult_bonuses </summary> 

 ### mult_bonuses:# 

 ```
 dict see "mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of mult_bonuses:# </summary> 

 ### mult_bonuses:#:scale 

 ```
 float value, examples: 0.5, 1.2
 Required 
```



 ### mult_bonuses:#:scaling-stat 

 ```
 str value, example: str
 Optional 
```



 ### mult_bonuses:#:stat 

 ```
 str value, examples: damage, movecost
 Required 
```



 ### mult_bonuses:#:type 

 ```
 str value, examples: bash, cut
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### name 

 ```
 str value, examples: Grab Break, Feint
 Required 
```


 ### req_buffs 

 ```
 str value, example: buff_medievalpole_onmiss 

-OR-

 list, see req_buffs:#
 Optional 
```


 <details> 
 <summary> children of req_buffs </summary> 

 ### req_buffs:# 

 ```
 str value, examples: buff_capoeira_onmove, buff_dragon_onhit
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### side_switch 

 ```
 bool value, example: True
 Optional 
```


 ### skill_requirements 

 ```
 list see "skill_requirements:#"
 Optional 
```


 <details> 
 <summary> children of skill_requirements </summary> 

 ### skill_requirements:# 

 ```
 dict see "skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of skill_requirements:# </summary> 

 ### skill_requirements:#:level 

 ```
 int value, examples: 4, 3
 Required 
```



 ### skill_requirements:#:name 

 ```
 str value, examples: unarmed, melee
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### stun_dur 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### stunned_target 

 ```
 bool value, example: True
 Optional 
```


 ### take_weapon 

 ```
 bool value, example: True
 Optional 
```


 ### type 

 ```
 str value, example: technique
 Required 
```


 ### unarmed_allowed 

 ```
 bool value, example: True
 Optional 
```


 ### unarmed_weapons_allowed 

 ```
 bool value, example: False
 Optional 
```


 ### weapon_damage_requirements 

 ```
 list see "weapon_damage_requirements:#"
 Optional 
```


 <details> 
 <summary> children of weapon_damage_requirements </summary> 

 ### weapon_damage_requirements:# 

 ```
 dict see "weapon_damage_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of weapon_damage_requirements:# </summary> 

 ### weapon_damage_requirements:#:min 

 ```
 int value, example: 2
 Required 
```



 ### weapon_damage_requirements:#:type 

 ```
 str value, example: bash
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### weighting 

 ```
 int value, examples: 2, 3
 Optional 
```


