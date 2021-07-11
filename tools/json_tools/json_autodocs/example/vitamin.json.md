
## vitamin.json:

### deficiency 
 ```
 str value, examples: hypocalcemia, anemia
 Optional 
```

 ### disease 

 ```
 list see "disease:#"
 Optional 
```


 <details> 
 <summary> children of disease </summary> 

 ### disease:# 

 ```
 list see "disease:#:#"
 Required 
```


 <details> 
 <summary> children of disease:# </summary> 

 ### disease:#:# 

 ```
 int value, examples: -4800, -2800
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### disease_excess 

 ```
 list see "disease_excess:#"
 Optional 
```


 <details> 
 <summary> children of disease_excess </summary> 

 ### disease_excess:# 

 ```
 list see "disease_excess:#:#"
 Required 
```


 <details> 
 <summary> children of disease_excess:# </summary> 

 ### disease_excess:#:# 

 ```
 int value, examples: 500, 10
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### excess 

 ```
 str value, examples: hypervitaminosis, toxin_buildup
 Optional 
```


 ### flags 

 ```
 list see "flags:#"
 Optional 
```


 <details> 
 <summary> children of flags </summary> 

 ### flags:# 

 ```
 str value, example: NO_DISPLAY
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### id 

 ```
 str value, examples: calcium, iron
 Required 
```


 ### max 

 ```
 int value, examples: 3600, 0
 Optional 
```


 ### min 

 ```
 int value, examples: -12000, -5600
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
 str value, examples: Calcium, Iron
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### rate 

 ```
 str value, examples: 15 m, 4 h
 Required 
```


 ### type 

 ```
 str value, example: vitamin
 Required 
```


 ### vit_type 

 ```
 str value, examples: vitamin, counter
 Required 
```


