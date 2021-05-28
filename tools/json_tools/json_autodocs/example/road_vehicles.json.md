
## road_vehicles.json:

### id 
 ```
 str value, examples: road_straight_wrecks, road_end_wrecks
 Required 
```

 ### locations 

 ```
 list see "locations:#"
 Optional 
```


 <details> 
 <summary> children of locations </summary> 

 ### locations:# 

 ```
 dict see "locations:#:" values
 Required 
```


 <details> 
 <summary> children of locations:# </summary> 

 ### locations:#:facing 

 ```
 int value, examples: 90, 0 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of locations:#:facing </summary> 

 ### locations:#:facing:# 

 ```
 int value, examples: 90, 0
 Optional 
```



 ### locations:#:x 

 ```
 int value, examples: 6, 16 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of locations:#:x </summary> 

 ### locations:#:x:# 

 ```
 int value, examples: 0, 4
 Optional 
```



 ### locations:#:y 

 ```
 int value, examples: 6, 17 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of locations:#:y </summary> 

 ### locations:#:y:# 

 ```
 int value, examples: 4, 0
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### spawn_types 

 ```
 list see "spawn_types:#"
 Optional 
```


 <details> 
 <summary> children of spawn_types </summary> 

 ### spawn_types:# 

 ```
 dict see "spawn_types:#:" values
 Required 
```


 <details> 
 <summary> children of spawn_types:# </summary> 

 ### spawn_types:#:// 

 ```
 str value, examples: Clear section of road, Clear section of bridge
 Required 
```



 ### spawn_types:#:vehicle_function 

 ```
 str value, examples: no_vehicles, parkinglot
 Required 
```



 ### spawn_types:#:vehicle_json 

 ```
 dict, see "spawn_types:#:vehicle_json:" values
 Optional 
```


 <details> 
 <summary> children of spawn_types:#:vehicle_json </summary> 

 ### spawn_types:#:vehicle_json:fuel 

 ```
 int value, examples: -1, 0
 Required 
```



 ### spawn_types:#:vehicle_json:number 

 ```
 int value, example: 1 

-OR-

 list
 Required 
```


 <details> 
 <summary> children of spawn_types:#:vehicle_json:number </summary> 

 ### spawn_types:#:vehicle_json:number:# 

 ```
 int value, example: 1
 Optional 
```



 ### spawn_types:#:vehicle_json:placement 

 ```
 str value, examples: highway, %t_wrecks
 Required 
```



 ### spawn_types:#:vehicle_json:status 

 ```
 int value, examples: -1, 1
 Required 
```



 ### spawn_types:#:vehicle_json:vehicle 

 ```
 str value, examples: highway, city_vehicles
 Required 
```



 ### spawn_types:#:weight 

 ```
 int value, examples: 50, 100
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
 str value, examples: vehicle_placement, vehicle_spawn
 Required 
```


