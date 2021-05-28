
## player_activities.json:

### activity_level 
 ```
 str value, examples: NO_EXERCISE, MODERATE_EXERCISE
 Required 
```

 ### auto_needs 

 ```
 bool value, example: True
 Optional 
```


 ### based_on 

 ```
 str value, examples: speed, neither
 Required 
```


 ### id 

 ```
 str value, examples: ACT_RELOAD, ACT_FIND_MOUNT
 Required 
```


 ### interruptable 

 ```
 bool value, example: False
 Optional 
```


 ### multi_activity 

 ```
 bool value, example: True
 Optional 
```


 ### no_resume 

 ```
 bool value, example: True
 Optional 
```


 ### refuel_fires 

 ```
 bool value, example: True
 Optional 
```


 ### rooted 

 ```
 bool value, example: True
 Optional 
```


 ### suspendable 

 ```
 bool value, example: False
 Optional 
```


 ### type 

 ```
 str value, example: activity_type
 Required 
```


 ### verb 

 ```
 str value, examples: butchering, constructing 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of verb </summary> 

 ### verb:ctxt 

 ```
 str value, example: training
 Optional 
```



 ### verb:str 

 ```
 str value, example: working out
 Optional 
```


