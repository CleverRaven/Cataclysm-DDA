
## item_spell_effects.json:

### aoe_increment 
 ```
 float value, example: 1.4
 Optional 
```

 ### base_casting_time 

 ```
 int value, example: 10
 Optional 
```


 ### damage_type 

 ```
 str value, example: cold
 Optional 
```


 ### description 

 ```
 str value, examples: Creates a short-lived inflatable., You see fog bubbling out of the device.
 Required 
```


 ### effect 

 ```
 str value, examples: summon, attack
 Required 
```


 ### effect_str 

 ```
 str value, examples: mon_halloween_dragon, mon_halloween_ghost
 Optional 
```


 ### field_id 

 ```
 str value, example: fd_fog
 Optional 
```


 ### field_intensity_increment 

 ```
 float value, example: 0.5
 Optional 
```


 ### flags 

 ```
 list see "flags:#"
 Required 
```


 <details> 
 <summary> children of flags </summary> 

 ### flags:# 

 ```
 str value, example: SILENT
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: dragon_inflatable, ghost_inflatable
 Required 
```


 ### max_aoe 

 ```
 int value, examples: 1, 60
 Required 
```


 ### max_damage 

 ```
 int value, example: 1
 Required 
```


 ### max_duration 

 ```
 int value, example: 18000
 Optional 
```


 ### max_field_intensity 

 ```
 int value, example: 4
 Optional 
```


 ### max_level 

 ```
 int value, example: 1
 Required 
```


 ### max_range 

 ```
 int value, examples: 10, 6
 Required 
```


 ### min_aoe 

 ```
 int value, examples: 1, 30
 Required 
```


 ### min_damage 

 ```
 int value, example: 1
 Required 
```


 ### min_duration 

 ```
 int value, examples: 40000, 18000
 Required 
```


 ### min_field_intensity 

 ```
 int value, example: 0
 Optional 
```


 ### min_range 

 ```
 int value, examples: 10, 6
 Required 
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
 str value, examples: inflatable dragon, inflatable ghost
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### shape 

 ```
 str value, examples: blast, cone
 Required 
```


 ### type 

 ```
 str value, example: SPELL
 Required 
```


 ### valid_targets 

 ```
 list see "valid_targets:#"
 Required 
```


 <details> 
 <summary> children of valid_targets </summary> 

 ### valid_targets:# 

 ```
 str value, examples: ground, ally
 Required 
```


