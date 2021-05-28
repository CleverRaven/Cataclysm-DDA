
## skills.json:

### companion_combat_rank_factor 
 ```
 int value, example: 1
 Optional 
```

 ### companion_industry_rank_factor 

 ```
 int value, example: 1
 Optional 
```


 ### companion_skill_practice 

 ```
 list see "companion_skill_practice:#"
 Optional 
```


 <details> 
 <summary> children of companion_skill_practice </summary> 

 ### companion_skill_practice:# 

 ```
 dict see "companion_skill_practice:#:" values
 Required 
```


 <details> 
 <summary> children of companion_skill_practice:# </summary> 

 ### companion_skill_practice:#:skill 

 ```
 str value, examples: hunting, 
 Required 
```



 ### companion_skill_practice:#:weight 

 ```
 int value, examples: 10, 15
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### companion_survival_rank_factor 

 ```
 int value, example: 1
 Optional 
```


 ### description 

 ```
 str value, examples: Your skill in speaking to other people.  Covers ability in boasting, flattery, threats, persuasion, lies, bartering, and other facets of interpersonal communication.  Works best in conjunction with a high level of intelligence., Your skill in accessing and manipulating computers.  Higher levels can allow a user to navigate complex software systems and even bypass their security.
 Required 
```


 ### display_category 

 ```
 str value, examples: display_ranged, display_interaction
 Required 
```


 ### id 

 ```
 str value, examples: speech, computer
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
 str value, examples: social, computers
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### tags 

 ```
 list see "tags:#"
 Optional 
```


 <details> 
 <summary> children of tags </summary> 

 ### tags:# 

 ```
 str value, examples: combat_skill, contextual_skill
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### time_to_attack 

 ```
 dict, see "time_to_attack:" values
 Optional 
```


 <details> 
 <summary> children of time_to_attack </summary> 

 ### time_to_attack:base_time 

 ```
 int value, examples: 80, 75
 Required 
```



 ### time_to_attack:min_time 

 ```
 int value, examples: 20, 15
 Required 
```



 ### time_to_attack:time_reduction_per_level 

 ```
 int value, examples: 6, 7
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: skill
 Required 
```


