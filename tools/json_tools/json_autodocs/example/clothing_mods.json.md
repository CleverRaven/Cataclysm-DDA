
## clothing_mods.json:

### destroy_prompt 
 ```
 str value, examples: Destroy leather padding, Destroy steel padding
 Required 
```

 ### flag 

 ```
 str value, examples: leather_padded, steel_padded
 Required 
```


 ### id 

 ```
 str value, examples: leather_padded, steel_padded
 Required 
```


 ### implement_prompt 

 ```
 str value, examples: Pad with leather, Pad with steel
 Required 
```


 ### item 

 ```
 str value, examples: leather, steel_armor
 Required 
```


 ### mod_value 

 ```
 list see "mod_value:#"
 Required 
```


 <details> 
 <summary> children of mod_value </summary> 

 ### mod_value:# 

 ```
 dict see "mod_value:#:" values
 Required 
```


 <details> 
 <summary> children of mod_value:# </summary> 

 ### mod_value:#:proportion 

 ```
 list see "mod_value:#:proportion:#"
 Required 
```


 <details> 
 <summary> children of mod_value:#:proportion </summary> 

 ### mod_value:#:proportion:# 

 ```
 str value, examples: thickness, coverage
 Required 
```



 ### mod_value:#:round_up 

 ```
 bool value, example: True
 Optional 
```



 ### mod_value:#:type 

 ```
 str value, examples: bash, encumbrance
 Required 
```



 ### mod_value:#:value 

 ```
 int value, examples: 1, 3
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### restricted 

 ```
 bool value, example: True
 Optional 
```


 ### type 

 ```
 str value, example: clothing_mod
 Required 
```


