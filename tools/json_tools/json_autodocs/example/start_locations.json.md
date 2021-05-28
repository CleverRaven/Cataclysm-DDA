
## start_locations.json:

### extend 
 ```
 dict, see "extend:" values
 Optional 
```

 <details> 
 <summary> children of extend </summary> 

 ### extend:flags 

 ```
 list see "extend:flags:#"
 Required 
```


 <details> 
 <summary> children of extend:flags </summary> 

 ### extend:flags:# 

 ```
 str value, example: BOARDED
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### flags 

 ```
 list see "flags:#"
 Optional 
```


 <details> 
 <summary> children of flags </summary> 

 ### flags:# 

 ```
 str value, example: ALLOW_OUTSIDE
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: sloc_shelter_a, sloc_shelter_b
 Required 
```


 ### name 

 ```
 str value, examples: Shelter - Open Floorplan, Shelter - Compartmentalized
 Required 
```


 ### terrain 

 ```
 list see "terrain:#"
 Optional 
```


 <details> 
 <summary> children of terrain </summary> 

 ### terrain:# 

 ```
 str value, examples: shelter, shelter_1 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of terrain:# </summary> 

 ### terrain:#:om_terrain 

 ```
 str value, examples: hospital, fire_station
 Optional 
```



 ### terrain:#:om_terrain_match_type 

 ```
 str value, examples: PREFIX, TYPE
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: start_location
 Required 
```


