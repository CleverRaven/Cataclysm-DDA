
## statistics.json:

### description 
 ```
 dict, see "description:" values
 Optional 
```

 <details> 
 <summary> children of description </summary> 

 ### description:str 

 ```
 str value, examples: wake up, monster killed
 Optional 
```



 ### description:str_pl 

 ```
 str value, examples: times woken up, monsters killed
 Optional 
```



 ### description:str_sp 

 ```
 str value, examples: damage taken, damage healed
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### drop_fields 

 ```
 list see "drop_fields:#"
 Optional 
```


 <details> 
 <summary> children of drop_fields </summary> 

 ### drop_fields:# 

 ```
 str value, examples: character, attacker
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### event_transformation 

 ```
 str value, examples: avatar_species_kills, moves_expand_pre_move_mode
 Optional 
```


 ### event_type 

 ```
 str value, examples: gains_skill_level, avatar_moves
 Optional 
```


 ### field 

 ```
 str value, examples: z, damage
 Optional 
```


 ### id 

 ```
 str value, examples: avatar_id, avatar_wakes_up
 Required 
```


 ### new_fields 

 ```
 dict, see "new_fields:" values
 Optional 
```


 <details> 
 <summary> children of new_fields </summary> 

 ### new_fields:flag 

 ```
 dict, see "new_fields:flag:" values
 Optional 
```


 <details> 
 <summary> children of new_fields:flag </summary> 

 ### new_fields:flag:flags_of_itype 

 ```
 str value, example: itype
 Required 
```



 ### new_fields:mounted 

 ```
 dict, see "new_fields:mounted:" values
 Optional 
```


 <details> 
 <summary> children of new_fields:mounted </summary> 

 ### new_fields:mounted:is_mounted 

 ```
 str value, example: mount
 Required 
```



 ### new_fields:oter_type_id 

 ```
 dict, see "new_fields:oter_type_id:" values
 Optional 
```


 <details> 
 <summary> children of new_fields:oter_type_id </summary> 

 ### new_fields:oter_type_id:oter_type_of_oter 

 ```
 str value, example: oter_id
 Required 
```



 ### new_fields:species 

 ```
 dict, see "new_fields:species:" values
 Optional 
```


 <details> 
 <summary> children of new_fields:species </summary> 

 ### new_fields:species:species_of_monster 

 ```
 str value, example: victim_type
 Required 
```



 ### new_fields:swimming 

 ```
 dict, see "new_fields:swimming:" values
 Optional 
```


 <details> 
 <summary> children of new_fields:swimming </summary> 

 ### new_fields:swimming:is_swimming_terrain 

 ```
 str value, example: terrain
 Required 
```



 ### new_fields:terrain_flag 

 ```
 dict, see "new_fields:terrain_flag:" values
 Optional 
```


 <details> 
 <summary> children of new_fields:terrain_flag </summary> 

 ### new_fields:terrain_flag:flags_of_terrain 

 ```
 str value, example: terrain
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### stat_type 

 ```
 str value, examples: count, last_value
 Optional 
```


 ### type 

 ```
 str value, examples: event_statistic, event_transformation
 Required 
```


 ### value_constraints 

 ```
 dict, see "value_constraints:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints </summary> 

 ### value_constraints:attacker 

 ```
 dict, see "value_constraints:attacker:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:attacker </summary> 

 ### value_constraints:attacker:equals_statistic 

 ```
 str value, example: avatar_id
 Required 
```



 ### value_constraints:character 

 ```
 dict, see "value_constraints:character:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:character </summary> 

 ### value_constraints:character:equals_statistic 

 ```
 str value, example: avatar_id
 Required 
```



 ### value_constraints:flag 

 ```
 dict, see "value_constraints:flag:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:flag </summary> 

 ### value_constraints:flag:equals 

 ```
 list see "value_constraints:flag:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:flag:equals </summary> 

 ### value_constraints:flag:equals:# 

 ```
 str value, example: flag_id
 Required 
```



 ### value_constraints:itype 

 ```
 dict, see "value_constraints:itype:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:itype </summary> 

 ### value_constraints:itype:equals 

 ```
 list see "value_constraints:itype:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:itype:equals </summary> 

 ### value_constraints:itype:equals:# 

 ```
 str value, example: itype_id
 Required 
```



 ### value_constraints:killer 

 ```
 dict, see "value_constraints:killer:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:killer </summary> 

 ### value_constraints:killer:equals_statistic 

 ```
 str value, example: avatar_id
 Required 
```



 ### value_constraints:mounted 

 ```
 dict, see "value_constraints:mounted:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:mounted </summary> 

 ### value_constraints:mounted:equals 

 ```
 bool value, examples: False, True
 Required 
```



 ### value_constraints:movement_mode 

 ```
 dict, see "value_constraints:movement_mode:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:movement_mode </summary> 

 ### value_constraints:movement_mode:equals 

 ```
 list see "value_constraints:movement_mode:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:movement_mode:equals </summary> 

 ### value_constraints:movement_mode:equals:# 

 ```
 str value, example: character_movemode
 Required 
```



 ### value_constraints:new_level 

 ```
 dict, see "value_constraints:new_level:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:new_level </summary> 

 ### value_constraints:new_level:equals 

 ```
 list see "value_constraints:new_level:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:new_level:equals </summary> 

 ### value_constraints:new_level:equals:# 

 ```
 str value, example: int
 Required 
```



 ### value_constraints:part 

 ```
 dict, see "value_constraints:part:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:part </summary> 

 ### value_constraints:part:equals 

 ```
 list see "value_constraints:part:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:part:equals </summary> 

 ### value_constraints:part:equals:# 

 ```
 str value, example: body_part
 Required 
```



 ### value_constraints:pos 

 ```
 dict, see "value_constraints:pos:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:pos </summary> 

 ### value_constraints:pos:equals_statistic 

 ```
 str value, example: first_omt
 Required 
```



 ### value_constraints:skill 

 ```
 dict, see "value_constraints:skill:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:skill </summary> 

 ### value_constraints:skill:equals 

 ```
 list see "value_constraints:skill:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:skill:equals </summary> 

 ### value_constraints:skill:equals:# 

 ```
 str value, example: skill_id
 Required 
```



 ### value_constraints:species 

 ```
 dict, see "value_constraints:species:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:species </summary> 

 ### value_constraints:species:equals 

 ```
 list see "value_constraints:species:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:species:equals </summary> 

 ### value_constraints:species:equals:# 

 ```
 str value, example: species_id
 Required 
```



 ### value_constraints:swimming 

 ```
 dict, see "value_constraints:swimming:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:swimming </summary> 

 ### value_constraints:swimming:equals 

 ```
 bool value, examples: False, True
 Required 
```



 ### value_constraints:terrain_flag 

 ```
 dict, see "value_constraints:terrain_flag:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:terrain_flag </summary> 

 ### value_constraints:terrain_flag:equals 

 ```
 list see "value_constraints:terrain_flag:equals:#"
 Required 
```


 <details> 
 <summary> children of value_constraints:terrain_flag:equals </summary> 

 ### value_constraints:terrain_flag:equals:# 

 ```
 str value, example: string
 Required 
```



 ### value_constraints:underwater 

 ```
 dict, see "value_constraints:underwater:" values
 Optional 
```


 <details> 
 <summary> children of value_constraints:underwater </summary> 

 ### value_constraints:underwater:equals 

 ```
 bool value, example: True
 Required 
```


