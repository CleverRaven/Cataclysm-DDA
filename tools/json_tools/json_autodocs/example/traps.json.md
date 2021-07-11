
## traps.json:

### action 
 ```
 str value, examples: none, shotgun
 Required 
```

 ### always_invisible 

 ```
 bool value, example: True
 Optional 
```


 ### avoidance 

 ```
 int value, examples: 0, 99
 Required 
```


 ### benign 

 ```
 bool value, examples: True, False
 Optional 
```


 ### color 

 ```
 str value, examples: white, brown
 Required 
```


 ### comfort 

 ```
 int value, example: 4
 Optional 
```


 ### difficulty 

 ```
 int value, examples: 0, 99
 Required 
```


 ### drops 

 ```
 list see "drops:#"
 Optional 
```


 <details> 
 <summary> children of drops </summary> 

 ### drops:# 

 ```
 str value, examples: string_36, beartrap 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of drops:# </summary> 

 ### drops:#:charges 

 ```
 int value, example: 1
 Optional 
```



 ### drops:#:item 

 ```
 str value, example: shot_00
 Optional 
```



 ### drops:#:quantity 

 ```
 int value, examples: 1, 2
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### floor_bedding_warmth 

 ```
 int value, examples: -1000, -500
 Optional 
```


 ### funnel_radius 

 ```
 int value, examples: 200, 300
 Optional 
```


 ### id 

 ```
 str value, examples: tr_bubblewrap, tr_glass
 Required 
```


 ### map_regen 

 ```
 str value, examples: microlab_shifting_hall, microlab_shifting_hall_2
 Optional 
```


 ### name 

 ```
 str value, examples: , shotgun trap
 Required 
```


 ### symbol 

 ```
 str value, examples: ^, _
 Required 
```


 ### trap_radius 

 ```
 int value, example: 1
 Optional 
```


 ### trigger_weight 

 ```
 str value, example: 200 g
 Optional 
```


 ### type 

 ```
 str value, example: trap
 Required 
```


 ### vehicle_data 

 ```
 dict, see "vehicle_data:" values
 Optional 
```


 <details> 
 <summary> children of vehicle_data </summary> 

 ### vehicle_data:chance 

 ```
 int value, examples: 70, 30
 Optional 
```



 ### vehicle_data:damage 

 ```
 int value, examples: 300, 500
 Optional 
```



 ### vehicle_data:do_explosion 

 ```
 bool value, example: True
 Optional 
```



 ### vehicle_data:is_falling 

 ```
 bool value, example: True
 Optional 
```



 ### vehicle_data:remove_trap 

 ```
 bool value, example: True
 Optional 
```



 ### vehicle_data:set_trap 

 ```
 str value, example: tr_shotgun_2_1
 Optional 
```



 ### vehicle_data:shrapnel 

 ```
 int value, examples: 8, 12
 Optional 
```



 ### vehicle_data:sound 

 ```
 str value, examples: Bang!, Boom!
 Optional 
```



 ### vehicle_data:sound_type 

 ```
 str value, examples: fire_gun, trap
 Optional 
```



 ### vehicle_data:sound_variant 

 ```
 str value, examples: default, bear_trap
 Optional 
```



 ### vehicle_data:sound_volume 

 ```
 int value, examples: 10, 60
 Optional 
```



 ### vehicle_data:spawn_items 

 ```
 list see "vehicle_data:spawn_items:#"
 Optional 
```


 <details> 
 <summary> children of vehicle_data:spawn_items </summary> 

 ### vehicle_data:spawn_items:# 

 ```
 str value, examples: beartrap, crossbow 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of vehicle_data:spawn_items:# </summary> 

 ### vehicle_data:spawn_items:#:chance 

 ```
 float value, example: 0.9
 Optional 
```



 ### vehicle_data:spawn_items:#:id 

 ```
 str value, example: bolt_steel
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### visibility 

 ```
 int value, examples: -1, 0
 Required 
```


