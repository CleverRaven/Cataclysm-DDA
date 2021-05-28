
## weather_type.json:

### acidic 
 ```
 bool value, examples: False, True
 Required 
```

 ### color 

 ```
 str value, examples: white, light_blue
 Required 
```


 ### dangerous 

 ```
 bool value, examples: False, True
 Required 
```


 ### effects 

 ```
 list see "effects:#"
 Optional 
```


 <details> 
 <summary> children of effects </summary> 

 ### effects:# 

 ```
 dict see "effects:#:" values
 Required 
```


 <details> 
 <summary> children of effects:# </summary> 

 ### effects:#:lightning 

 ```
 bool value, example: True
 Optional 
```



 ### effects:#:message 

 ```
 str value, examples: A flash of lightning illuminates your surroundings!., The acid rain stings, but is mostly harmless for now…
 Optional 
```



 ### effects:#:must_be_outside 

 ```
 bool value, examples: True, False
 Required 
```



 ### effects:#:one_in_chance 

 ```
 int value, example: 50
 Optional 
```



 ### effects:#:pain 

 ```
 int value, examples: 1, 3
 Optional 
```



 ### effects:#:pain_max 

 ```
 int value, examples: 10, 100
 Optional 
```



 ### effects:#:rain_proof 

 ```
 bool value, example: True
 Optional 
```



 ### effects:#:sound_effect 

 ```
 str value, example: thunder_far
 Optional 
```



 ### effects:#:sound_message 

 ```
 str value, example: You hear a distant rumble of thunder.
 Optional 
```



 ### effects:#:time_between 

 ```
 str value, examples: 3 minutes, 2 seconds
 Optional 
```



 ### effects:#:wet 

 ```
 int value, examples: 10, 40
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: sunny, cloudy
 Required 
```


 ### light_modifier 

 ```
 int value, examples: -20, -30
 Required 
```


 ### map_color 

 ```
 str value, examples: h_light_blue, yellow_green
 Required 
```


 ### name 

 ```
 str value, examples: Sunny, Cloudy
 Required 
```


 ### precip 

 ```
 str value, examples: heavy, light
 Required 
```


 ### rains 

 ```
 bool value, examples: True, False
 Required 
```


 ### ranged_penalty 

 ```
 int value, examples: 4, 0
 Required 
```


 ### requirements 

 ```
 dict see "requirements:" values
 Required 
```


 <details> 
 <summary> children of requirements </summary> 

 ### requirements:humidity_and_pressure 

 ```
 bool value, example: False
 Optional 
```



 ### requirements:humidity_max 

 ```
 int value, example: 70
 Optional 
```



 ### requirements:humidity_min 

 ```
 int value, examples: 40, 96
 Optional 
```



 ### requirements:pressure_max 

 ```
 int value, examples: 1010, 1003
 Optional 
```



 ### requirements:pressure_min 

 ```
 int value, example: 1020
 Optional 
```



 ### requirements:required_weathers 

 ```
 list see "requirements:required_weathers:#"
 Optional 
```


 <details> 
 <summary> children of requirements:required_weathers </summary> 

 ### requirements:required_weathers:# 

 ```
 str value, examples: rain, light_drizzle
 Required 
```



 ### requirements:temperature_max 

 ```
 int value, example: 33
 Optional 
```



 ### requirements:time 

 ```
 str value, example: day
 Optional 
```



 ### requirements:windpower_min 

 ```
 int value, example: 15
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### sight_penalty 

 ```
 float value, examples: 1.0, 1.03
 Required 
```


 ### sound_attn 

 ```
 int value, examples: 0, 4
 Required 
```


 ### sound_category 

 ```
 str value, examples: drizzle, rainy
 Optional 
```


 ### sun_intensity 

 ```
 str value, examples: light, none
 Required 
```


 ### sym 

 ```
 str value, examples: ., %
 Required 
```


 ### tiles_animation 

 ```
 str value, examples: weather_rain_drop, weather_snowflake
 Optional 
```


 ### type 

 ```
 str value, example: weather_type
 Required 
```


 ### weather_animation 

 ```
 dict, see "weather_animation:" values
 Optional 
```


 <details> 
 <summary> children of weather_animation </summary> 

 ### weather_animation:color 

 ```
 str value, examples: light_blue, white
 Required 
```



 ### weather_animation:factor 

 ```
 float value, examples: 0.01, 0.02
 Required 
```



 ### weather_animation:sym 

 ```
 str value, examples: ,, .
 Required 
```


