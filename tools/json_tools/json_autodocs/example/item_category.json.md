
## item_category.json:

### id 
 ```
 str value, examples: guns, magazines
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
 str value, examples: GUNS, MAGAZINES
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### priority_zones 

 ```
 list see "priority_zones:#"
 Optional 
```


 <details> 
 <summary> children of priority_zones </summary> 

 ### priority_zones:# 

 ```
 dict see "priority_zones:#:" values
 Required 
```


 <details> 
 <summary> children of priority_zones:# </summary> 

 ### priority_zones:#:filthy 

 ```
 bool value, example: True
 Required 
```



 ### priority_zones:#:id 

 ```
 str value, examples: LOOT_FCLOTHING, LOOT_FARMOR
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### sort_rank 

 ```
 int value, examples: -12, -23
 Required 
```


 ### type 

 ```
 str value, example: ITEM_CATEGORY
 Required 
```


 ### zone 

 ```
 str value, examples: LOOT_GUNS, LOOT_MAGAZINES
 Optional 
```


