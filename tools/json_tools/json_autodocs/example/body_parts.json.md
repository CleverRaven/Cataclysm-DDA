
## body_parts.json:

### accusative 
 ```
 dict see "accusative:" values
 Required 
```

 <details> 
 <summary> children of accusative </summary> 

 ### accusative:ctxt 

 ```
 str value, example: bodypart_accusative
 Required 
```



 ### accusative:str 

 ```
 str value, examples: torso, head
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### accusative_multiple 

 ```
 dict, see "accusative_multiple:" values
 Optional 
```


 <details> 
 <summary> children of accusative_multiple </summary> 

 ### accusative_multiple:ctxt 

 ```
 str value, example: bodypart_accusative
 Required 
```



 ### accusative_multiple:str 

 ```
 str value, examples: arms, hands
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### base_hp 

 ```
 int value, example: 60
 Required 
```


 ### bionic_slots 

 ```
 int value, examples: 4, 20
 Required 
```


 ### cold_morale_mod 

 ```
 float value, examples: 0.5, 2
 Optional 
```


 ### connected_to 

 ```
 str value, examples: torso, head
 Optional 
```


 ### drench_capacity 

 ```
 int value, examples: 3, 1
 Required 
```


 ### encumbrance_text 

 ```
 str value, examples: Running is slowed., Melee and ranged combat is hampered.
 Required 
```


 ### fire_warmth_bonus 

 ```
 int value, examples: 150, 600
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
 str value, example: IGNORE_TEMP
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### heading 

 ```
 str value, examples: Torso, Head
 Required 
```


 ### heading_multiple 

 ```
 str value, examples: Arms, Hands
 Required 
```


 ### hit_difficulty 

 ```
 float value, examples: 1.15, 0.95
 Required 
```


 ### hit_size 

 ```
 int value, examples: 9, 3
 Required 
```


 ### hit_size_relative 

 ```
 list see "hit_size_relative:#"
 Required 
```


 <details> 
 <summary> children of hit_size_relative </summary> 

 ### hit_size_relative:# 

 ```
 int value, examples: 0, 15
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### hot_morale_mod 

 ```
 float value, examples: 0.5, 2
 Optional 
```


 ### hp_bar_ui_text 

 ```
 str value, examples: TORSO, HEAD
 Optional 
```


 ### id 

 ```
 str value, examples: torso, head
 Required 
```


 ### is_limb 

 ```
 bool value, example: True
 Optional 
```


 ### legacy_id 

 ```
 str value, examples: TORSO, HEAD
 Required 
```


 ### main_part 

 ```
 str value, examples: head, arm_l
 Required 
```


 ### name 

 ```
 str value, examples: torso, head
 Required 
```


 ### name_multiple 

 ```
 str value, examples: arms, hands
 Optional 
```


 ### opposite_part 

 ```
 str value, examples: torso, head
 Required 
```


 ### side 

 ```
 str value, examples: both, left
 Required 
```


 ### smash_efficiency 

 ```
 float value, example: 0.25
 Optional 
```


 ### smash_message 

 ```
 str value, examples: You use your flippin' face to smash the %s.  EXTREME., You elbow-smash the %s.
 Required 
```


 ### squeamish_penalty 

 ```
 int value, examples: 5, 3
 Required 
```


 ### stylish_bonus 

 ```
 float value, examples: 0.5, 2
 Optional 
```


 ### type 

 ```
 str value, example: body_part
 Required 
```


