
## materials.json:

### acid_resist 
 ```
 int value, examples: 0, 1
 Optional 
```

 ### bash_dmg_verb 

 ```
 str value, examples: dented, ripped
 Optional 
```


 ### bash_resist 

 ```
 int value, examples: 1, 0
 Optional 
```


 ### bullet_resist 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### burn_data 

 ```
 list see "burn_data:#"
 Optional 
```


 <details> 
 <summary> children of burn_data </summary> 

 ### burn_data:# 

 ```
 dict see "burn_data:#:" values
 Required 
```


 <details> 
 <summary> children of burn_data:# </summary> 

 ### burn_data:#:// 

 ```
 str value, example: More like shattering than melting
 Optional 
```



 ### burn_data:#:burn 

 ```
 int value, examples: 1, 10
 Optional 
```



 ### burn_data:#:fuel 

 ```
 int value, examples: 1, 0
 Optional 
```



 ### burn_data:#:immune 

 ```
 bool value, example: True
 Optional 
```



 ### burn_data:#:smoke 

 ```
 int value, examples: 1, 2
 Optional 
```



 ### burn_data:#:volume_per_turn 

 ```
 str value, examples: 1250 ml, 2500 ml
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### burn_products 

 ```
 list see "burn_products:#"
 Optional 
```


 <details> 
 <summary> children of burn_products </summary> 

 ### burn_products:# 

 ```
 list see "burn_products:#:#"
 Required 
```


 <details> 
 <summary> children of burn_products:# </summary> 

 ### burn_products:#:# 

 ```
 str value, examples: scrap, ash
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### chip_resist 

 ```
 int value, examples: 0, 2
 Optional 
```


 ### compact_accepts 

 ```
 list see "compact_accepts:#"
 Optional 
```


 <details> 
 <summary> children of compact_accepts </summary> 

 ### compact_accepts:# 

 ```
 str value, example: budget_steel
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### compacts_into 

 ```
 list see "compacts_into:#"
 Optional 
```


 <details> 
 <summary> children of compacts_into </summary> 

 ### compacts_into:# 

 ```
 str value, examples: material_aluminium_ingot, scrap_bronze
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### cut_dmg_verb 

 ```
 str value, examples: cut, scratched
 Optional 
```


 ### cut_resist 

 ```
 int value, examples: 1, 0
 Optional 
```


 ### density 

 ```
 int value, examples: 4, 1
 Optional 
```


 ### dmg_adj 

 ```
 list see "dmg_adj:#"
 Optional 
```


 <details> 
 <summary> children of dmg_adj </summary> 

 ### dmg_adj:# 

 ```
 str value, examples: lightly damaged, marked
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### edible 

 ```
 bool value, example: True
 Optional 
```


 ### elec_resist 

 ```
 int value, examples: 0, 2
 Optional 
```


 ### fire_resist 

 ```
 int value, examples: 1, 3
 Optional 
```


 ### fuel_data 

 ```
 dict, see "fuel_data:" values
 Optional 
```


 <details> 
 <summary> children of fuel_data </summary> 

 ### fuel_data:// 

 ```
 str value, examples: Roughly equivalent to LPG, Compressed gaseous hydrogen has 9.2 MJ/L.  This stuff has a very high energy density but is also bulky and explosive.
 Optional 
```



 ### fuel_data:energy 

 ```
 int value, examples: 1, 26
 Required 
```



 ### fuel_data:explosion_data 

 ```
 dict, see "fuel_data:explosion_data:" values
 Optional 
```


 <details> 
 <summary> children of fuel_data:explosion_data </summary> 

 ### fuel_data:explosion_data:chance_cold 

 ```
 int value, examples: 1000, 10
 Required 
```



 ### fuel_data:explosion_data:chance_hot 

 ```
 int value, examples: 20, 5
 Required 
```



 ### fuel_data:explosion_data:factor 

 ```
 float value, examples: 0.2, 1
 Required 
```



 ### fuel_data:explosion_data:fiery 

 ```
 bool value, examples: True, False
 Required 
```



 ### fuel_data:explosion_data:size_factor 

 ```
 float value, examples: 0.1, 0.15
 Required 
```



 ### fuel_data:perpetual 

 ```
 bool value, example: True
 Optional 
```



 ### fuel_data:pump_terrain 

 ```
 str value, examples: t_diesel_pump, t_jp8_pump
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: pseudo_fuel, battery
 Required 
```


 ### latent_heat 

 ```
 int value, examples: 273, 333
 Optional 
```


 ### name 

 ```
 str value, examples: Powder, Pseudo fuel
 Required 
```


 ### reinforces 

 ```
 bool value, example: True
 Optional 
```


 ### repaired_with 

 ```
 str value, examples: scrap, leather
 Optional 
```


 ### rotting 

 ```
 bool value, example: True
 Optional 
```


 ### salvaged_into 

 ```
 str value, examples: resin_chunk, acidchitin_piece
 Optional 
```


 ### soft 

 ```
 bool value, examples: True, False
 Optional 
```


 ### specific_heat_liquid 

 ```
 float value, examples: 0.82, 3.9
 Optional 
```


 ### specific_heat_solid 

 ```
 float value, examples: 0.45, 1.9
 Optional 
```


 ### type 

 ```
 str value, example: material
 Required 
```


 ### vitamins 

 ```
 list see "vitamins:#"
 Optional 
```


 <details> 
 <summary> children of vitamins </summary> 

 ### vitamins:# 

 ```
 list see "vitamins:#:#"
 Required 
```


 <details> 
 <summary> children of vitamins:# </summary> 

 ### vitamins:#:# 

 ```
 str value, examples: calcium, vitB
 Required 
```


