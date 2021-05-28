
## conducts.json:

### hidden_by 
 ```
 list see "hidden_by:#"
 Optional 
```

 <details> 
 <summary> children of hidden_by </summary> 

 ### hidden_by:# 

 ```
 str value, examples: conduct_pure_human, conduct_zero_kills
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: conduct_no_smash, conduct_no_attacks
 Required 
```


 ### name 

 ```
 str value, examples: Mouse in a china shop, Nonviolence
 Required 
```


 ### requirements 

 ```
 list see "requirements:#"
 Required 
```


 <details> 
 <summary> children of requirements </summary> 

 ### requirements:# 

 ```
 dict see "requirements:#:" values
 Required 
```


 <details> 
 <summary> children of requirements:# </summary> 

 ### requirements:#:description 

 ```
 str value, examples: Install no bionic implants, Smash no tiles
 Required 
```



 ### requirements:#:event_statistic 

 ```
 str value, examples: num_installs_cbm, num_avatar_smash
 Required 
```



 ### requirements:#:is 

 ```
 str value, example: <=
 Required 
```



 ### requirements:#:target 

 ```
 int value, example: 0
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: conduct
 Required 
```


