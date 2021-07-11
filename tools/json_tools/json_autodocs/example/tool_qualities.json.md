
## tool_qualities.json:

### id 
 ```
 str value, examples: CUT, CUT_FINE
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
 str value, examples: cutting, fine cutting
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: tool_quality
 Required 
```


 ### usages 

 ```
 list see "usages:#"
 Optional 
```


 <details> 
 <summary> children of usages </summary> 

 ### usages:# 

 ```
 list see "usages:#:#"
 Required 
```


 <details> 
 <summary> children of usages:# </summary> 

 ### usages:#:# 

 ```
 int value, examples: 1, 2 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of usages:#:# </summary> 

 ### usages:#:#:# 

 ```
 str value, examples: salvage, LUMBER
 Required 
```


