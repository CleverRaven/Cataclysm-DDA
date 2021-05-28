
## scenarios.json:

### add_professions 
 ```
 bool value, example: True
 Optional 
```

 ### allowed_locs 

 ```
 list see "allowed_locs:#"
 Required 
```


 <details> 
 <summary> children of allowed_locs </summary> 

 ### allowed_locs:# 

 ```
 str value, examples: sloc_house, sloc_field
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### custom_initial_date 

 ```
 dict, see "custom_initial_date:" values
 Optional 
```


 <details> 
 <summary> children of custom_initial_date </summary> 

 ### custom_initial_date:season 

 ```
 str value, examples: winter, summer
 Required 
```



 ### custom_initial_date:year 

 ```
 int value, example: 2
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### description 

 ```
 str value, examples: You have survived the initial wave of panic, and have achieved (relative) safety in one of the many government evac shelters., You have survived the initial wave of panic, and have achieved (relative) safety in an abandoned building.  Unfortunately, while you're the only one here now, you are definitely not the first person to be here, and in its current state this building no longer seems nearly as secure as one might have hoped.
 Required 
```


 ### flags 

 ```
 list see "flags:#"
 Required 
```


 <details> 
 <summary> children of flags </summary> 

 ### flags:# 

 ```
 str value, examples: LONE_START, CITY_START
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### forbidden_traits 

 ```
 list see "forbidden_traits:#"
 Optional 
```


 <details> 
 <summary> children of forbidden_traits </summary> 

 ### forbidden_traits:# 

 ```
 str value, examples: FASTREADER, TOUGH
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### forced_traits 

 ```
 list see "forced_traits:#"
 Optional 
```


 <details> 
 <summary> children of forced_traits </summary> 

 ### forced_traits:# 

 ```
 str value, examples: ILLITERATE, FLIMSY2
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: evacuee, squatter
 Required 
```


 ### map_extra 

 ```
 str value, example: mx_helicopter
 Optional 
```


 ### missions 

 ```
 list see "missions:#"
 Optional 
```


 <details> 
 <summary> children of missions </summary> 

 ### missions:# 

 ```
 str value, examples: MISSION_INFECTED_START_FIND_ANTIBIOTICS, MISSION_LAST_DELIVERY
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### name 

 ```
 str value, examples: Evacuee, Squatter
 Required 
```


 ### points 

 ```
 int value, examples: -8, 0
 Required 
```


 ### professions 

 ```
 list see "professions:#"
 Optional 
```


 <details> 
 <summary> children of professions </summary> 

 ### professions:# 

 ```
 str value, examples: unemployed, sheltered_survivor
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### start_name 

 ```
 str value, examples: In Town, Wilderness
 Required 
```


 ### traits 

 ```
 list see "traits:#"
 Optional 
```


 <details> 
 <summary> children of traits </summary> 

 ### traits:# 

 ```
 str value, example: ELFAEYES
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: scenario
 Required 
```


 ### vehicle 

 ```
 str value, examples: food_truck_delivery_mission, 2seater2_scenario
 Optional 
```


