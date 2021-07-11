
## martialarts_fictional.json:

### aoe 
 ```
 str value, example: wide
 Optional 
```

 ### arm_block 

 ```
 int value, examples: 3, 2
 Optional 
```


 ### arm_block_with_bio_armor_arms 

 ```
 bool value, example: True
 Optional 
```


 ### block_counter 

 ```
 bool value, example: True
 Optional 
```


 ### crit_ok 

 ```
 bool value, example: True
 Optional 
```


 ### crit_tec 

 ```
 bool value, example: True
 Optional 
```


 ### defensive 

 ```
 bool value, example: True
 Optional 
```


 ### description 

 ```
 str value, examples: A modern combat style for the post-modern human.  Nicknamed "Biojutsu", Bionic Combatives combines integrated weaponry, armor and augments into an consolidated fighting discipline., One of the Five Deadly Venoms, used by Zhang Yiaotian.  Centipede Style uses an onslaught of rapid strikes.  Each attack you land increases your attack speed.  Critical hits increase your damage further.
 Optional 
```


 ### disarms 

 ```
 bool value, example: True
 Optional 
```


 ### dodge_counter 

 ```
 bool value, example: True
 Optional 
```


 ### down_dur 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### grab_break 

 ```
 bool value, example: True
 Optional 
```


 ### id 

 ```
 str value, examples: style_biojutsu, style_centipede
 Required 
```


 ### initial_ma_styles 

 ```
 list see "initial_ma_styles:#"
 Optional 
```


 <details> 
 <summary> children of initial_ma_styles </summary> 

 ### initial_ma_styles:# 

 ```
 str value, example: style_centipede
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### initiate 

 ```
 list see "initiate:#"
 Optional 
```


 <details> 
 <summary> children of initiate </summary> 

 ### initiate:# 

 ```
 str value, examples: BEGINNING BIONIC COMBATIVES PROGRAM.  INITIATING COMBAT PROTOCOLS., You ready yourself to attack as fast as possible.
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### knockback_dist 

 ```
 int value, examples: 1, 3
 Optional 
```


 ### knockback_spread 

 ```
 int value, example: 1
 Optional 
```


 ### learn_difficulty 

 ```
 int value, example: 10
 Optional 
```


 ### leg_block 

 ```
 int value, example: 4
 Optional 
```


 ### leg_block_with_bio_armor_legs 

 ```
 bool value, example: True
 Optional 
```


 ### melee_allowed 

 ```
 bool value, example: True
 Optional 
```


 ### messages 

 ```
 list see "messages:#"
 Optional 
```


 <details> 
 <summary> children of messages </summary> 

 ### messages:# 

 ```
 str value, examples: You make an efficient strike against %s, You block and counter-attack %s
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### miss_recovery 

 ```
 bool value, example: True
 Optional 
```


 ### mult_bonuses 

 ```
 list see "mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of mult_bonuses </summary> 

 ### mult_bonuses:# 

 ```
 dict see "mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of mult_bonuses:# </summary> 

 ### mult_bonuses:#:scale 

 ```
 float value, examples: 2.0, 0.5
 Required 
```



 ### mult_bonuses:#:stat 

 ```
 str value, examples: damage, movecost
 Required 
```



 ### mult_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### name 

 ```
 str value, examples: Disarm, Grab Break 

-OR-

 dict
 Required 
```


 <details> 
 <summary> children of name </summary> 

 ### name:str 

 ```
 str value, examples: Bionic Combatives, Centipede Kung Fu
 Optional 
```



 </details>
</summary>


 </details>
</summary>

 ### onattack_buffs 

 ```
 list see "onattack_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onattack_buffs </summary> 

 ### onattack_buffs:# 

 ```
 dict see "onattack_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onattack_buffs:# </summary> 

 ### onattack_buffs:#:bonus_dodges 

 ```
 int value, example: 1
 Required 
```



 ### onattack_buffs:#:buff_duration 

 ```
 int value, example: 1
 Required 
```



 ### onattack_buffs:#:description 

 ```
 str value, example: Nothing is scarier than an angry scorpion.  Your attacks can keep others at bay.

+1 Dodge attempts.
Lasts 1 turn.
 Required 
```



 ### onattack_buffs:#:id 

 ```
 str value, example: buff_scorpion_onattack
 Required 
```



 ### onattack_buffs:#:name 

 ```
 str value, example: Scorpion's Intimidation
 Required 
```



 ### onattack_buffs:#:skill_requirements 

 ```
 list see "onattack_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of onattack_buffs:#:skill_requirements </summary> 

 ### onattack_buffs:#:skill_requirements:# 

 ```
 dict see "onattack_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onattack_buffs:#:skill_requirements:# </summary> 

 ### onattack_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 1
 Required 
```



 ### onattack_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### onattack_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### oncrit_buffs 

 ```
 list see "oncrit_buffs:#"
 Optional 
```


 <details> 
 <summary> children of oncrit_buffs </summary> 

 ### oncrit_buffs:# 

 ```
 dict see "oncrit_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of oncrit_buffs:# </summary> 

 ### oncrit_buffs:#:buff_duration 

 ```
 int value, example: 2
 Required 
```



 ### oncrit_buffs:#:description 

 ```
 str value, example: Your venom burns your opponents at the worst of times.

+2 bashing damage.
Lasts 2 turns.
 Required 
```



 ### oncrit_buffs:#:flat_bonuses 

 ```
 list see "oncrit_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of oncrit_buffs:#:flat_bonuses </summary> 

 ### oncrit_buffs:#:flat_bonuses:# 

 ```
 dict see "oncrit_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of oncrit_buffs:#:flat_bonuses:# </summary> 

 ### oncrit_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: 2.0
 Required 
```



 ### oncrit_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### oncrit_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### oncrit_buffs:#:id 

 ```
 str value, example: buff_centipede_oncrit
 Required 
```



 ### oncrit_buffs:#:name 

 ```
 str value, example: Centipede's Venom
 Required 
```



 ### oncrit_buffs:#:skill_requirements 

 ```
 list see "oncrit_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of oncrit_buffs:#:skill_requirements </summary> 

 ### oncrit_buffs:#:skill_requirements:# 

 ```
 dict see "oncrit_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of oncrit_buffs:#:skill_requirements:# </summary> 

 ### oncrit_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 1
 Required 
```



 ### oncrit_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### oncrit_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### ondodge_buffs 

 ```
 list see "ondodge_buffs:#"
 Optional 
```


 <details> 
 <summary> children of ondodge_buffs </summary> 

 ### ondodge_buffs:# 

 ```
 dict see "ondodge_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of ondodge_buffs:# </summary> 

 ### ondodge_buffs:#:buff_duration 

 ```
 int value, example: 1
 Required 
```



 ### ondodge_buffs:#:description 

 ```
 str value, example: Your evasiveness has left your opponent wide open to painful bite.

Enables "Viper Bite" technique.
Lasts 1 turn.
 Required 
```



 ### ondodge_buffs:#:flat_bonuses 

 ```
 list see "ondodge_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of ondodge_buffs:#:flat_bonuses </summary> 

 ### ondodge_buffs:#:flat_bonuses:# 

 ```
 dict see "ondodge_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of ondodge_buffs:#:flat_bonuses:# </summary> 

 ### ondodge_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: 2.0
 Required 
```



 ### ondodge_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### ondodge_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### ondodge_buffs:#:id 

 ```
 str value, example: buff_venom_snake_ondodge1
 Required 
```



 ### ondodge_buffs:#:name 

 ```
 str value, example: Viper's Ambush
 Required 
```



 ### ondodge_buffs:#:skill_requirements 

 ```
 list see "ondodge_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of ondodge_buffs:#:skill_requirements </summary> 

 ### ondodge_buffs:#:skill_requirements:# 

 ```
 dict see "ondodge_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of ondodge_buffs:#:skill_requirements:# </summary> 

 ### ondodge_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 2
 Required 
```



 ### ondodge_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### ondodge_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### ongethit_buffs 

 ```
 list see "ongethit_buffs:#"
 Optional 
```


 <details> 
 <summary> children of ongethit_buffs </summary> 

 ### ongethit_buffs:# 

 ```
 dict see "ongethit_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of ongethit_buffs:# </summary> 

 ### ongethit_buffs:#:buff_duration 

 ```
 int value, example: 5
 Required 
```



 ### ongethit_buffs:#:description 

 ```
 str value, example: Your venom is just another lesson about the strength of your iron body.

+2 bash damage.
Lasts 5 turns.
 Required 
```



 ### ongethit_buffs:#:flat_bonuses 

 ```
 list see "ongethit_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of ongethit_buffs:#:flat_bonuses </summary> 

 ### ongethit_buffs:#:flat_bonuses:# 

 ```
 dict see "ongethit_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of ongethit_buffs:#:flat_bonuses:# </summary> 

 ### ongethit_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: 2.0
 Required 
```



 ### ongethit_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### ongethit_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### ongethit_buffs:#:id 

 ```
 str value, example: toad_ongethit
 Required 
```



 ### ongethit_buffs:#:name 

 ```
 str value, example: Toad's Venom
 Required 
```



 ### ongethit_buffs:#:skill_requirements 

 ```
 list see "ongethit_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of ongethit_buffs:#:skill_requirements </summary> 

 ### ongethit_buffs:#:skill_requirements:# 

 ```
 dict see "ongethit_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of ongethit_buffs:#:skill_requirements:# </summary> 

 ### ongethit_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 2
 Required 
```



 ### ongethit_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### ongethit_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### onhit_buffs 

 ```
 list see "onhit_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onhit_buffs </summary> 

 ### onhit_buffs:# 

 ```
 dict see "onhit_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onhit_buffs:# </summary> 

 ### onhit_buffs:#:buff_duration 

 ```
 int value, examples: 3, 4
 Required 
```



 ### onhit_buffs:#:description 

 ```
 str value, examples: Your attacks are a blur of hands and legs that become faster as your strike your opponents without rest.

-4 move cost.
Lasts 3 turns.  Stacks 4 times., Your venom is a long lasting pain that your opponents will never forget.

+2 bash damage.
Lasts 4 turns.
 Required 
```



 ### onhit_buffs:#:flat_bonuses 

 ```
 list see "onhit_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of onhit_buffs:#:flat_bonuses </summary> 

 ### onhit_buffs:#:flat_bonuses:# 

 ```
 dict see "onhit_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onhit_buffs:#:flat_bonuses:# </summary> 

 ### onhit_buffs:#:flat_bonuses:#:scale 

 ```
 float value, examples: -4.0, 2.0
 Required 
```



 ### onhit_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: movecost, damage
 Required 
```



 ### onhit_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### onhit_buffs:#:id 

 ```
 str value, examples: buff_centipede_onhit, buff_lizard_onhit
 Required 
```



 ### onhit_buffs:#:max_stacks 

 ```
 int value, example: 4
 Optional 
```



 ### onhit_buffs:#:name 

 ```
 str value, examples: Centipede's Frenzy, Lizard's Venom
 Required 
```



 ### onhit_buffs:#:skill_requirements 

 ```
 list see "onhit_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of onhit_buffs:#:skill_requirements </summary> 

 ### onhit_buffs:#:skill_requirements:# 

 ```
 dict see "onhit_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onhit_buffs:#:skill_requirements:# </summary> 

 ### onhit_buffs:#:skill_requirements:#:level 

 ```
 int value, examples: 5, 2
 Required 
```



 ### onhit_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### onhit_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### onkill_buffs 

 ```
 list see "onkill_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onkill_buffs </summary> 

 ### onkill_buffs:# 

 ```
 dict see "onkill_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onkill_buffs:# </summary> 

 ### onkill_buffs:#:buff_duration 

 ```
 int value, example: 3
 Required 
```



 ### onkill_buffs:#:description 

 ```
 str value, example: >10 LOCATE TARGET
>20 EXECUTE TARGET
>30 GOTO 10

+1 Accuracy, +2 all damage.
Lasts 3 turns.  Stacks 3 times.
 Required 
```



 ### onkill_buffs:#:flat_bonuses 

 ```
 list see "onkill_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of onkill_buffs:#:flat_bonuses </summary> 

 ### onkill_buffs:#:flat_bonuses:# 

 ```
 dict see "onkill_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onkill_buffs:#:flat_bonuses:# </summary> 

 ### onkill_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: 2.0
 Required 
```



 ### onkill_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### onkill_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### onkill_buffs:#:id 

 ```
 str value, example: buff_biojutsu_onkill
 Required 
```



 ### onkill_buffs:#:max_stacks 

 ```
 int value, example: 3
 Required 
```



 ### onkill_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Required 
```



 ### onkill_buffs:#:name 

 ```
 str value, example: Optimization
 Required 
```



 ### onkill_buffs:#:skill_requirements 

 ```
 list see "onkill_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of onkill_buffs:#:skill_requirements </summary> 

 ### onkill_buffs:#:skill_requirements:# 

 ```
 dict see "onkill_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onkill_buffs:#:skill_requirements:# </summary> 

 ### onkill_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 4
 Required 
```



 ### onkill_buffs:#:skill_requirements:#:name 

 ```
 str value, example: melee
 Required 
```



 ### onkill_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### onmove_buffs 

 ```
 list see "onmove_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onmove_buffs </summary> 

 ### onmove_buffs:# 

 ```
 dict see "onmove_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onmove_buffs:# </summary> 

 ### onmove_buffs:#:buff_duration 

 ```
 int value, examples: 3, 2
 Required 
```



 ### onmove_buffs:#:description 

 ```
 str value, examples: By briefly scaling, leaping, or pushing off a nearby wall, can attack downward at unsuspecting opponents.

+3 Accuracy when near a wall.
Lasts 3 turns., Rush forward and catch your prey!

+10% damage.
Enables "Pincer Strike" technique.
Stacks 2 times.  Lasts 2 turns.
 Required 
```



 ### onmove_buffs:#:flat_bonuses 

 ```
 list see "onmove_buffs:#:flat_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onmove_buffs:#:flat_bonuses </summary> 

 ### onmove_buffs:#:flat_bonuses:# 

 ```
 dict see "onmove_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onmove_buffs:#:flat_bonuses:# </summary> 

 ### onmove_buffs:#:flat_bonuses:#:scale 

 ```
 float value, examples: 3.0, -1.0
 Required 
```



 ### onmove_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: hit, armor
 Required 
```



 ### onmove_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### onmove_buffs:#:id 

 ```
 str value, examples: buff_lizard_onmove, buff_scorpion_onmove
 Required 
```



 ### onmove_buffs:#:max_stacks 

 ```
 int value, examples: 2, 6
 Optional 
```



 ### onmove_buffs:#:mult_bonuses 

 ```
 list see "onmove_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onmove_buffs:#:mult_bonuses </summary> 

 ### onmove_buffs:#:mult_bonuses:# 

 ```
 dict see "onmove_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onmove_buffs:#:mult_bonuses:# </summary> 

 ### onmove_buffs:#:mult_bonuses:#:scale 

 ```
 float value, example: 1.1
 Required 
```



 ### onmove_buffs:#:mult_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### onmove_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### onmove_buffs:#:name 

 ```
 str value, examples: Lizard's Leap, Scorpion's Charge
 Required 
```



 ### onmove_buffs:#:skill_requirements 

 ```
 list see "onmove_buffs:#:skill_requirements:#"
 Optional 
```


 <details> 
 <summary> children of onmove_buffs:#:skill_requirements </summary> 

 ### onmove_buffs:#:skill_requirements:# 

 ```
 dict see "onmove_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onmove_buffs:#:skill_requirements:# </summary> 

 ### onmove_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 2
 Required 
```



 ### onmove_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### onmove_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 ### onmove_buffs:#:wall_adjacent 

 ```
 bool value, example: True
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### onpause_buffs 

 ```
 list see "onpause_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onpause_buffs </summary> 

 ### onpause_buffs:# 

 ```
 dict see "onpause_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onpause_buffs:# </summary> 

 ### onpause_buffs:#:buff_duration 

 ```
 int value, example: 2
 Required 
```



 ### onpause_buffs:#:description 

 ```
 str value, example: By concentrating for a moment, you can bolster the strength of your iron skin.

+3 bash, cut, and stab armor.
Lasts 2 turns.
 Required 
```



 ### onpause_buffs:#:flat_bonuses 

 ```
 list see "onpause_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of onpause_buffs:#:flat_bonuses </summary> 

 ### onpause_buffs:#:flat_bonuses:# 

 ```
 dict see "onpause_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onpause_buffs:#:flat_bonuses:# </summary> 

 ### onpause_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: 3.0
 Required 
```



 ### onpause_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: armor
 Required 
```



 ### onpause_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### onpause_buffs:#:id 

 ```
 str value, example: buff_toad_onpause
 Required 
```



 ### onpause_buffs:#:name 

 ```
 str value, example: Toad's Meditation
 Required 
```



 ### onpause_buffs:#:skill_requirements 

 ```
 list see "onpause_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of onpause_buffs:#:skill_requirements </summary> 

 ### onpause_buffs:#:skill_requirements:# 

 ```
 dict see "onpause_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onpause_buffs:#:skill_requirements:# </summary> 

 ### onpause_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 5
 Required 
```



 ### onpause_buffs:#:skill_requirements:#:name 

 ```
 str value, example: unarmed
 Required 
```



 ### onpause_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### points 

 ```
 int value, example: 2
 Optional 
```


 ### powerful_knockback 

 ```
 bool value, example: True
 Optional 
```


 ### req_buffs 

 ```
 list see "req_buffs:#"
 Optional 
```


 <details> 
 <summary> children of req_buffs </summary> 

 ### req_buffs:# 

 ```
 str value, examples: buff_venom_snake_ondodge1, buff_scorpion_onmove
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### skill_requirements 

 ```
 list see "skill_requirements:#"
 Optional 
```


 <details> 
 <summary> children of skill_requirements </summary> 

 ### skill_requirements:# 

 ```
 dict see "skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of skill_requirements:# </summary> 

 ### skill_requirements:#:level 

 ```
 int value, examples: 4, 5
 Required 
```



 ### skill_requirements:#:name 

 ```
 str value, examples: unarmed, melee
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### starting_trait 

 ```
 bool value, example: False
 Optional 
```


 ### static_buffs 

 ```
 list see "static_buffs:#"
 Optional 
```


 <details> 
 <summary> children of static_buffs </summary> 

 ### static_buffs:# 

 ```
 dict see "static_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of static_buffs:# </summary> 

 ### static_buffs:#:// 

 ```
 str value, example: FWIW, this is twice the amount of armor provided by bionic plating.
 Optional 
```



 ### static_buffs:#:bonus_blocks 

 ```
 int value, example: 2
 Optional 
```



 ### static_buffs:#:description 

 ```
 str value, examples: void player::ApplyBiojutsuStatic() {
    blocks_left += 2;
    set_hit_bonus( get_hit_bonus() + 1 );
}

+2 Blocks attempts, +1 Accuracy., By briefly scaling, leaping, or pushing off a nearby wall, you can avoid the worst of your opponents attacks.

+3.0 Dodge skill when near a wall.
Enables "Lizard Tail" and "Lizard Wall Counter" techniques when near a wall.
 Required 
```



 ### static_buffs:#:flat_bonuses 

 ```
 list see "static_buffs:#:flat_bonuses:#"
 Required 
```


 <details> 
 <summary> children of static_buffs:#:flat_bonuses </summary> 

 ### static_buffs:#:flat_bonuses:# 

 ```
 dict see "static_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of static_buffs:#:flat_bonuses:# </summary> 

 ### static_buffs:#:flat_bonuses:#:scale 

 ```
 float value, examples: 1.0, 3.0
 Required 
```



 ### static_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: dodge, hit
 Required 
```



 ### static_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### static_buffs:#:id 

 ```
 str value, examples: buff_biojutsu_static, buff_lizard_static
 Required 
```



 ### static_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### static_buffs:#:name 

 ```
 str value, examples: Biojutsu Stance, Lizard's Cunning
 Required 
```



 ### static_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Required 
```



 ### static_buffs:#:wall_adjacent 

 ```
 bool value, example: True
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### stun_dur 

 ```
 int value, examples: 1, 2
 Optional 
```


 ### stunned_target 

 ```
 bool value, example: True
 Optional 
```


 ### techniques 

 ```
 list see "techniques:#"
 Optional 
```


 <details> 
 <summary> children of techniques </summary> 

 ### techniques:# 

 ```
 str value, examples: tec_biojutsu_counter, tec_centipede_rapid
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, examples: technique, martial_art
 Required 
```


 ### unarmed_allowed 

 ```
 bool value, example: True
 Optional 
```


 ### unarmed_weapons_allowed 

 ```
 bool value, example: False
 Optional 
```


 ### valid 

 ```
 bool value, example: False
 Optional 
```


 ### wall_adjacent 

 ```
 bool value, example: True
 Optional 
```


 ### weapons 

 ```
 list see "weapons:#"
 Optional 
```


 <details> 
 <summary> children of weapons </summary> 

 ### weapons:# 

 ```
 str value, example: bio_claws_weapon
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### weighting 

 ```
 int value, example: 2
 Optional 
```


