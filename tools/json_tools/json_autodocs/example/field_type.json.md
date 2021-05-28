
## field_type.json:

### accelerated_decay 
 ```
 bool value, example: True
 Optional 
```

 ### apply_slime_factor 

 ```
 int value, example: 10
 Optional 
```


 ### bash 

 ```
 dict, see "bash:" values
 Optional 
```


 <details> 
 <summary> children of bash </summary> 

 ### bash:msg_success 

 ```
 str value, example: You brush aside some webs.
 Required 
```



 ### bash:sound 

 ```
 str value, example: hsh!
 Required 
```



 ### bash:sound_fail_vol 

 ```
 int value, example: 2
 Required 
```



 ### bash:sound_vol 

 ```
 int value, example: 2
 Required 
```



 ### bash:str_max 

 ```
 int value, example: 3
 Required 
```



 ### bash:str_min 

 ```
 int value, example: 1
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### decay_amount_factor 

 ```
 int value, examples: 5, 2
 Optional 
```


 ### decrease_intensity_on_contact 

 ```
 bool value, example: True
 Optional 
```


 ### description_affix 

 ```
 str value, examples: covered_in, under
 Optional 
```


 ### dirty_transparency_cache 

 ```
 bool value, example: True
 Optional 
```


 ### display_field 

 ```
 bool value, example: True
 Optional 
```


 ### display_items 

 ```
 bool value, example: False
 Optional 
```


 ### gas_absorption_factor 

 ```
 int value, examples: 15, 12
 Optional 
```


 ### half_life 

 ```
 str value, examples: 50 minutes, 2 days
 Optional 
```


 ### has_acid 

 ```
 bool value, example: True
 Optional 
```


 ### has_elec 

 ```
 bool value, example: True
 Optional 
```


 ### has_fire 

 ```
 bool value, example: True
 Optional 
```


 ### has_fume 

 ```
 bool value, example: True
 Optional 
```


 ### id 

 ```
 str value, examples: fd_blood, fd_bile
 Required 
```


 ### immune_mtypes 

 ```
 list see "immune_mtypes:#"
 Optional 
```


 <details> 
 <summary> children of immune_mtypes </summary> 

 ### immune_mtypes:# 

 ```
 str value, example: mon_spider_web
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### immunity_data 

 ```
 dict, see "immunity_data:" values
 Optional 
```


 <details> 
 <summary> children of immunity_data </summary> 

 ### immunity_data:body_part_env_resistance 

 ```
 list see "immunity_data:body_part_env_resistance:#"
 Optional 
```


 <details> 
 <summary> children of immunity_data:body_part_env_resistance </summary> 

 ### immunity_data:body_part_env_resistance:# 

 ```
 list see "immunity_data:body_part_env_resistance:#:#"
 Required 
```


 <details> 
 <summary> children of immunity_data:body_part_env_resistance:# </summary> 

 ### immunity_data:body_part_env_resistance:#:# 

 ```
 str value, example: mouth
 Required 
```



 ### immunity_data:traits 

 ```
 list see "immunity_data:traits:#"
 Optional 
```


 <details> 
 <summary> children of immunity_data:traits </summary> 

 ### immunity_data:traits:# 

 ```
 str value, examples: WEB_WALKER, ACIDPROOF
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### intensity_levels 

 ```
 list see "intensity_levels:#"
 Required 
```


 <details> 
 <summary> children of intensity_levels </summary> 

 ### intensity_levels:# 

 ```
 dict see "intensity_levels:#:" values
 Required 
```


 <details> 
 <summary> children of intensity_levels:# </summary> 

 ### intensity_levels:#:// 

 ```
 str value, example: repeat last entry
 Optional 
```



 ### intensity_levels:#:color 

 ```
 str value, examples: light_gray, light_green
 Optional 
```



 ### intensity_levels:#:convection_temperature_mod 

 ```
 int value, examples: 300, -10
 Optional 
```



 ### intensity_levels:#:dangerous 

 ```
 bool value, examples: True, False
 Optional 
```



 ### intensity_levels:#:effects 

 ```
 list see "intensity_levels:#:effects:#"
 Optional 
```


 <details> 
 <summary> children of intensity_levels:#:effects </summary> 

 ### intensity_levels:#:effects:# 

 ```
 dict see "intensity_levels:#:effects:#:" values
 Required 
```


 <details> 
 <summary> children of intensity_levels:#:effects:# </summary> 

 ### intensity_levels:#:effects:#:// 

 ```
 str value, example: won't be applied outside of vehicles, so it could apply harsher effect
 Optional 
```



 ### intensity_levels:#:effects:#:body_part 

 ```
 str value, examples: mouth, eyes
 Optional 
```



 ### intensity_levels:#:effects:#:chance_in_vehicle 

 ```
 int value, example: 1
 Optional 
```



 ### intensity_levels:#:effects:#:chance_inside_vehicle 

 ```
 int value, examples: 1, 3
 Optional 
```



 ### intensity_levels:#:effects:#:chance_outside_vehicle 

 ```
 int value, examples: 1, 3
 Optional 
```



 ### intensity_levels:#:effects:#:effect_id 

 ```
 str value, examples: poison, webbed
 Required 
```



 ### intensity_levels:#:effects:#:immune_in_vehicle 

 ```
 bool value, example: True
 Optional 
```



 ### intensity_levels:#:effects:#:immune_inside_vehicle 

 ```
 bool value, example: True
 Optional 
```



 ### intensity_levels:#:effects:#:immune_outside_vehicle 

 ```
 bool value, example: True
 Optional 
```



 ### intensity_levels:#:effects:#:intensity 

 ```
 int value, examples: 1, 2
 Required 
```



 ### intensity_levels:#:effects:#:is_environmental 

 ```
 bool value, example: False
 Optional 
```



 ### intensity_levels:#:effects:#:max_duration 

 ```
 str value, examples: 2 seconds, 2 minutes
 Optional 
```



 ### intensity_levels:#:effects:#:message 

 ```
 str value, examples: The sap sticks to you!, You feel sick from inhaling the extinguisher mist.
 Optional 
```



 ### intensity_levels:#:effects:#:message_npc 

 ```
 str value, example: The sap sticks to <npcname>!
 Optional 
```



 ### intensity_levels:#:effects:#:message_type 

 ```
 str value, example: bad
 Optional 
```



 ### intensity_levels:#:effects:#:min_duration 

 ```
 str value, examples: 2 seconds, 2 minutes
 Optional 
```



 ### intensity_levels:#:extra_radiation_max 

 ```
 int value, example: 1
 Optional 
```



 ### intensity_levels:#:intensity_upgrade_chance 

 ```
 int value, example: 10
 Optional 
```



 ### intensity_levels:#:intensity_upgrade_duration 

 ```
 str value, example: 6 hours
 Optional 
```



 ### intensity_levels:#:light_emitted 

 ```
 int value, examples: 20, 0.01
 Optional 
```



 ### intensity_levels:#:light_override 

 ```
 float value, example: 3.7
 Optional 
```



 ### intensity_levels:#:monster_spawn_chance 

 ```
 int value, examples: 15, 600
 Optional 
```



 ### intensity_levels:#:monster_spawn_count 

 ```
 int value, example: 1
 Optional 
```



 ### intensity_levels:#:monster_spawn_group 

 ```
 str value, examples: GROUP_TINDALOS, GROUP_NETHER_FATIGUE_FIELD
 Optional 
```



 ### intensity_levels:#:monster_spawn_radius 

 ```
 int value, example: 1
 Optional 
```



 ### intensity_levels:#:name 

 ```
 str value, examples: hazy cloud, blood splatter 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of intensity_levels:#:name </summary> 

 ### intensity_levels:#:name:ctxt 

 ```
 str value, example: noun
 Optional 
```



 ### intensity_levels:#:name:str 

 ```
 str value, example: fire
 Optional 
```



 ### intensity_levels:#:radiation_hurt_damage_max 

 ```
 int value, example: 3
 Optional 
```



 ### intensity_levels:#:radiation_hurt_damage_min 

 ```
 int value, example: 1
 Optional 
```



 ### intensity_levels:#:radiation_hurt_message 

 ```
 str value, example: This radioactive gas burns!
 Optional 
```



 ### intensity_levels:#:scent_neutralization 

 ```
 int value, example: 1
 Optional 
```



 ### intensity_levels:#:sym 

 ```
 str value, examples: &, 8
 Optional 
```



 ### intensity_levels:#:translucency 

 ```
 int value, examples: 5, 1
 Optional 
```



 ### intensity_levels:#:transparent 

 ```
 bool value, example: False
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### is_splattering 

 ```
 bool value, example: True
 Optional 
```


 ### legacy_enum_id 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### legacy_make_rubble 

 ```
 bool value, example: True
 Optional 
```


 ### looks_like 

 ```
 str value, examples: fd_smoke, fd_blood
 Optional 
```


 ### npc_complain 

 ```
 dict, see "npc_complain:" values
 Optional 
```


 <details> 
 <summary> children of npc_complain </summary> 

 ### npc_complain:chance 

 ```
 int value, example: 20
 Required 
```



 ### npc_complain:duration 

 ```
 str value, examples: 30 minutes, 10 minutes
 Required 
```



 ### npc_complain:issue 

 ```
 str value, examples: weed_smoke, crack_smoke
 Required 
```



 ### npc_complain:speech 

 ```
 str value, examples: <weed_smoke>, <crack_smoke>
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### outdoor_age_speedup 

 ```
 str value, examples: 100 minutes, 1 minutes
 Optional 
```


 ### percent_spread 

 ```
 int value, examples: 10, 30
 Optional 
```


 ### phase 

 ```
 str value, examples: gas, liquid
 Optional 
```


 ### priority 

 ```
 int value, examples: 8, -1
 Optional 
```


 ### type 

 ```
 str value, example: field_type
 Required 
```


 ### underwater_age_speedup 

 ```
 str value, examples: 25 minutes, 2 minutes
 Optional 
```


 ### wandering_field 

 ```
 str value, examples: fd_toxic_gas, fd_tindalos_gas
 Optional 
```


