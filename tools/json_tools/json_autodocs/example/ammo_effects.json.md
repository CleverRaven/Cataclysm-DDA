
## ammo_effects.json:

### aoe 
 ```
 dict, see "aoe:" values
 Optional 
```

 <details> 
 <summary> children of aoe </summary> 

 ### aoe:chance 

 ```
 int value, examples: 100, 50
 Optional 
```



 ### aoe:check_passable 

 ```
 bool value, examples: False, True
 Optional 
```



 ### aoe:check_sees 

 ```
 bool value, examples: False, True
 Optional 
```



 ### aoe:check_sees_radius 

 ```
 int value, examples: 0, 3
 Optional 
```



 ### aoe:field_type 

 ```
 str value, examples: fd_fire, fd_smoke
 Required 
```



 ### aoe:intensity_max 

 ```
 int value, examples: 3, 1
 Required 
```



 ### aoe:intensity_min 

 ```
 int value, examples: 3, 1
 Required 
```



 ### aoe:radius 

 ```
 int value, examples: 3, 0
 Optional 
```



 ### aoe:radius_z 

 ```
 int value, example: 0
 Optional 
```



 ### aoe:size 

 ```
 int value, examples: 3, 4
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### do_emp_blast 

 ```
 bool value, examples: False, True
 Optional 
```


 ### do_flashbang 

 ```
 bool value, examples: False, True
 Optional 
```


 ### explosion 

 ```
 dict, see "explosion:" values
 Optional 
```


 <details> 
 <summary> children of explosion </summary> 

 ### explosion:distance_factor 

 ```
 float value, examples: 0.8, 0.7
 Optional 
```



 ### explosion:fire 

 ```
 bool value, examples: True, False
 Optional 
```



 ### explosion:power 

 ```
 int value, examples: 360, 60
 Required 
```



 ### explosion:shrapnel 

 ```
 dict, see "explosion:shrapnel:" values
 Optional 
```


 <details> 
 <summary> children of explosion:shrapnel </summary> 

 ### explosion:shrapnel:casing_mass 

 ```
 int value, examples: 1500, 680
 Required 
```



 ### explosion:shrapnel:drop 

 ```
 str value, example: null
 Optional 
```



 ### explosion:shrapnel:fragment_mass 

 ```
 float value, examples: 0.3, 0.15
 Required 
```



 ### explosion:shrapnel:recovery 

 ```
 int value, example: 0
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: AE_NULL, FLAME
 Required 
```


 ### trail 

 ```
 dict, see "trail:" values
 Optional 
```


 <details> 
 <summary> children of trail </summary> 

 ### trail:chance 

 ```
 int value, examples: 66, 75
 Optional 
```



 ### trail:field_type 

 ```
 str value, examples: fd_fire, fd_laser
 Required 
```



 ### trail:intensity_max 

 ```
 int value, example: 2
 Required 
```



 ### trail:intensity_min 

 ```
 int value, examples: 1, 2
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: ammo_effect
 Required 
```


