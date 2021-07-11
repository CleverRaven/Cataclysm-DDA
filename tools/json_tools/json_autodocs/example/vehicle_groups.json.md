
## vehicle_groups.json:

### id 
 ```
 str value, examples: city_vehicles, boatrent
 Required 
```

 ### type 

 ```
 str value, example: vehicle_group
 Required 
```


 ### vehicles 

 ```
 list see "vehicles:#"
 Required 
```


 <details> 
 <summary> children of vehicles </summary> 

 ### vehicles:# 

 ```
 list see "vehicles:#:#"
 Required 
```


 <details> 
 <summary> children of vehicles:# </summary> 

 ### vehicles:#:# 

 ```
 str value, examples: car, canoe
 Required 
```


