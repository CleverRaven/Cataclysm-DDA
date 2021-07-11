
## achievements.json:

### description 
 ```
 str value, examples: Survive for a day and find a safe place to sleep, Survive for a week
 Optional 
```

 ### hidden_by 

 ```
 list see "hidden_by:#"
 Optional 
```


 <details> 
 <summary> children of hidden_by </summary> 

 ### hidden_by:# 

 ```
 str value, examples: achievement_survive_91_days, achievement_cut_1_tree
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: achievement_kill_zombie, achievement_kill_cyborg
 Required 
```


 ### name 

 ```
 str value, examples: One down, billions to go…, Resistance is not futile 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of name </summary> 

 ### name://~ 

 ```
 str value, examples: Pun on 'deca-' prefix for 10 and 'decimate', with it's meaning for killing, Pun on 'cent-' prefix for 100 and 'sentinel', as in its connection to guarding or protection that may involve killing
 Optional 
```



 ### name:str 

 ```
 str value, examples: Decamate, Centinel
 Optional 
```



 </details>
</summary>


 </details>
</summary>

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
 str value, examples: Wield a crowbar, Wear a tank suit
 Optional 
```



 ### requirements:#:event_statistic 

 ```
 str value, examples: avatar_last_crosses_mutation_threshold, num_avatar_monster_kills
 Required 
```



 ### requirements:#:is 

 ```
 str value, examples: >=, ==
 Required 
```



 ### requirements:#:target 

 ```
 int value, examples: 1, 10 

-OR-

 list, see requirements:#:target:#
 Optional 
```


 <details> 
 <summary> children of requirements:#:target </summary> 

 ### requirements:#:target:# 

 ```
 str value, examples: mutation_category_id, oter_type_str_id
 Optional 
```



 ### requirements:#:visible 

 ```
 str value, examples: when_achievement_completed, when_requirement_completed
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### time_constraint 

 ```
 dict, see "time_constraint:" values
 Optional 
```


 <details> 
 <summary> children of time_constraint </summary> 

 ### time_constraint:is 

 ```
 str value, examples: >=, <=
 Required 
```



 ### time_constraint:since 

 ```
 str value, example: game_start
 Required 
```



 ### time_constraint:target 

 ```
 str value, examples: 91 days, 1 minute
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: achievement
 Required 
```


