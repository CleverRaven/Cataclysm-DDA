
## gates.json:

### alias 
 ```
 list see "alias:#"
 Optional 
```

 <details> 
 <summary> children of alias </summary> 

 ### alias:# 

 ```
 str value, examples: t_gates_control_concrete, t_gates_control_concrete_lab
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### bashing_damage 

 ```
 int value, examples: 40, 30
 Required 
```


 ### door 

 ```
 str value, examples: t_door_metal_locked, t_palisade_gate
 Required 
```


 ### floor 

 ```
 str value, examples: t_floor, t_thconc_floor
 Required 
```


 ### id 

 ```
 str value, examples: t_gates_mech_control, t_gates_mech_control_lab
 Required 
```


 ### messages 

 ```
 dict see "messages:" values
 Required 
```


 <details> 
 <summary> children of messages </summary> 

 ### messages:close 

 ```
 str value, examples: The gate is closed!, The barn doors closed!
 Required 
```



 ### messages:fail 

 ```
 str value, examples: The gate can't be closed!, The barn doors can't be closed!
 Required 
```



 ### messages:open 

 ```
 str value, examples: The gate is opened!, The barn doors opened!
 Required 
```



 ### messages:pull 

 ```
 str value, examples: You turn the handle…, You pull the rope…
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### moves 

 ```
 int value, examples: 1800, 1200
 Required 
```


 ### type 

 ```
 str value, example: gate
 Required 
```


 ### walls 

 ```
 str value, examples: t_wall_wood, t_palisade 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of walls </summary> 

 ### walls:# 

 ```
 str value, example: t_wall
 Optional 
```


