
## monster_factions.json:

### base_faction 
 ```
 str value, examples: insect, mutant
 Optional 
```

 ### by_mood 

 ```
 str value, examples: fish, factionless 

-OR-

 list, see by_mood:#
 Optional 
```


 <details> 
 <summary> children of by_mood </summary> 

 ### by_mood:# 

 ```
 str value, examples: zombie, animal
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### friendly 

 ```
 str value, examples: zombie, science 

-OR-

 list, see friendly:#
 Optional 
```


 <details> 
 <summary> children of friendly </summary> 

 ### friendly:# 

 ```
 str value, example: slime
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### hate 

 ```
 str value, examples: small_animal, fish 

-OR-

 list, see hate:#
 Optional 
```


 <details> 
 <summary> children of hate </summary> 

 ### hate:# 

 ```
 str value, examples: small_animal, insect
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### name 

 ```
 str value, examples: , factionless
 Required 
```


 ### neutral 

 ```
 str value, examples: mutant_with_vortex, nether 

-OR-

 list, see neutral:#
 Optional 
```


 <details> 
 <summary> children of neutral </summary> 

 ### neutral:# 

 ```
 str value, examples: fish, bot
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: MONSTER_FACTION
 Required 
```


