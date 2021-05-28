
## construction.json:

### byproducts 
 ```
 list see "byproducts:#"
 Optional 
```

 <details> 
 <summary> children of byproducts </summary> 

 ### byproducts:# 

 ```
 dict see "byproducts:#:" values
 Required 
```


 <details> 
 <summary> children of byproducts:# </summary> 

 ### byproducts:#:charges 

 ```
 list see "byproducts:#:charges:#"
 Optional 
```


 <details> 
 <summary> children of byproducts:#:charges </summary> 

 ### byproducts:#:charges:# 

 ```
 int value, examples: 15, 300
 Required 
```



 ### byproducts:#:count 

 ```
 list see "byproducts:#:count:#"
 Optional 
```


 <details> 
 <summary> children of byproducts:#:count </summary> 

 ### byproducts:#:count:# 

 ```
 int value, examples: 1, 0
 Required 
```



 ### byproducts:#:item 

 ```
 str value, examples: straw_pile, duct_tape
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### category 

 ```
 str value, examples: CONSTRUCT, FURN
 Required 
```


 ### components 

 ```
 list see "components:#"
 Optional 
```


 <details> 
 <summary> children of components </summary> 

 ### components:# 

 ```
 list see "components:#:#"
 Required 
```


 <details> 
 <summary> children of components:# </summary> 

 ### components:#:# 

 ```
 list see "components:#:#:#"
 Required 
```


 <details> 
 <summary> children of components:#:# </summary> 

 ### components:#:#:# 

 ```
 str value, examples: 2x4, nail
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

 ### dark_craftable 

 ```
 bool value, example: True
 Optional 
```


 ### difficulty 

 ```
 int value, examples: 1, 3
 Optional 
```


 ### group 

 ```
 str value, examples: build_sky_light_frame, board_up_wood_door
 Required 
```


 ### id 

 ```
 str value, examples: constr_deconstruct, constr_deconstruct_simple
 Required 
```


 ### on_display 

 ```
 bool value, example: False
 Optional 
```


 ### post_flags 

 ```
 list see "post_flags:#"
 Optional 
```


 <details> 
 <summary> children of post_flags </summary> 

 ### post_flags:# 

 ```
 str value, example: keep_items
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### post_special 

 ```
 str value, examples: done_extract_maybe_revert_to_dirt, done_deconstruct
 Optional 
```


 ### post_terrain 

 ```
 str value, examples: t_dirt, t_skylight_frame
 Optional 
```


 ### pre_flags 

 ```
 str value, examples: DIGGABLE, BARRICADABLE_DOOR 

-OR-

 list, see pre_flags:#
 Optional 
```


 <details> 
 <summary> children of pre_flags </summary> 

 ### pre_flags:# 

 ```
 str value, examples: FLAT, DIGGABLE
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### pre_note 

 ```
 str value, examples: Can be deconstructed without tools., Must be supported on at least two sides.
 Optional 
```


 ### pre_special 

 ```
 str value, examples: check_empty, check_support
 Optional 
```


 ### pre_terrain 

 ```
 str value, examples: t_window_empty, t_pit
 Optional 
```


 ### qualities 

 ```
 list see "qualities:#"
 Optional 
```


 <details> 
 <summary> children of qualities </summary> 

 ### qualities:# 

 ```
 list see "qualities:#:#"
 Required 
```


 <details> 
 <summary> children of qualities:# </summary> 

 ### qualities:#:# 

 ```
 dict, see "qualities:#:#:" values
 Optional 
```


 <details> 
 <summary> children of qualities:#:# </summary> 

 ### qualities:#:#:id 

 ```
 str value, examples: HAMMER, DIG
 Required 
```



 ### qualities:#:#:level 

 ```
 int value, examples: 2, 1
 Required 
```



 ### qualities:#:id 

 ```
 str value, examples: CUT, HAMMER
 Optional 
```



 ### qualities:#:level 

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

 ### required_skills 

 ```
 list see "required_skills:#"
 Optional 
```


 <details> 
 <summary> children of required_skills </summary> 

 ### required_skills:# 

 ```
 list see "required_skills:#:#"
 Required 
```


 <details> 
 <summary> children of required_skills:# </summary> 

 ### required_skills:#:# 

 ```
 str value, examples: fabrication, survival
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### skill 

 ```
 str value, examples: fabrication, survival
 Optional 
```


 ### time 

 ```
 str value, examples: 30 m, 120 m
 Required 
```


 ### tools 

 ```
 list see "tools:#"
 Optional 
```


 <details> 
 <summary> children of tools </summary> 

 ### tools:# 

 ```
 list see "tools:#:#"
 Required 
```


 <details> 
 <summary> children of tools:# </summary> 

 ### tools:#:# 

 ```
 str value, examples: paint_brush, chipper 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of tools:#:# </summary> 

 ### tools:#:#:# 

 ```
 str value, examples: con_mix, oxy_torch
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

 ### type 

 ```
 str value, example: construction
 Required 
```


 ### using 

 ```
 list see "using:#"
 Optional 
```


 <details> 
 <summary> children of using </summary> 

 ### using:# 

 ```
 list see "using:#:#"
 Required 
```


 <details> 
 <summary> children of using:# </summary> 

 ### using:#:# 

 ```
 str value, example: welding_standard
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### vehicle_start 

 ```
 bool value, example: True
 Optional 
```


