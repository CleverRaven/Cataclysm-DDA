
## harvest.json:

### entries 
 ```
 list see "entries:#"
 Optional 
```

 <details> 
 <summary> children of entries </summary> 

 ### entries:# 

 ```
 dict see "entries:#:" values
 Required 
```


 <details> 
 <summary> children of entries:# </summary> 

 ### entries:#:base_num 

 ```
 list see "entries:#:base_num:#"
 Optional 
```


 <details> 
 <summary> children of entries:#:base_num </summary> 

 ### entries:#:base_num:# 

 ```
 int value, examples: 2, 1
 Required 
```



 ### entries:#:drop 

 ```
 str value, examples: mutant_meat, meat
 Required 
```



 ### entries:#:faults 

 ```
 list see "entries:#:faults:#"
 Optional 
```


 <details> 
 <summary> children of entries:#:faults </summary> 

 ### entries:#:faults:# 

 ```
 str value, example: fault_bionic_salvaged
 Required 
```



 ### entries:#:flags 

 ```
 list see "entries:#:flags:#"
 Optional 
```


 <details> 
 <summary> children of entries:#:flags </summary> 

 ### entries:#:flags:# 

 ```
 str value, example: FILTHY
 Required 
```



 ### entries:#:mass_ratio 

 ```
 float value, examples: 0.32, 0.25
 Optional 
```



 ### entries:#:max 

 ```
 int value, examples: 1, 8
 Optional 
```



 ### entries:#:scale_num 

 ```
 list see "entries:#:scale_num:#"
 Optional 
```


 <details> 
 <summary> children of entries:#:scale_num </summary> 

 ### entries:#:scale_num:# 

 ```
 int value, examples: 0, 1
 Required 
```



 ### entries:#:type 

 ```
 str value, examples: flesh, bionic
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
 str value, examples: null, exempt
 Required 
```


 ### message 

 ```
 str value, examples: You carefully crack open its exoskeleton to get at the flesh beneath, <arachnid_harvest>
 Optional 
```


 ### type 

 ```
 str value, example: harvest
 Required 
```


