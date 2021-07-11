
## martialarts.json:

### allow_melee 
 ```
 bool value, example: True
 Optional 
```

 ### arm_block 

 ```
 int value, examples: 2, 1
 Optional 
```


 ### autolearn 

 ```
 list see "autolearn:#"
 Optional 
```


 <details> 
 <summary> children of autolearn </summary> 

 ### autolearn:# 

 ```
 list see "autolearn:#:#"
 Required 
```


 <details> 
 <summary> children of autolearn:# </summary> 

 ### autolearn:#:# 

 ```
 str value, example: melee
 Required 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### description 

 ```
 str value, examples: Not a martial art, this is just plain old punching and kicking., Not a martial art, setting this style will prevent weapon attacks and force punches (with free hand) or kicks.
 Required 
```


 ### force_unarmed 

 ```
 bool value, example: True
 Optional 
```


 ### id 

 ```
 str value, examples: style_none, style_kicks
 Required 
```


 ### initiate 

 ```
 list see "initiate:#"
 Required 
```


 <details> 
 <summary> children of initiate </summary> 

 ### initiate:# 

 ```
 str value, examples: You decide to not use any martial arts., You force yourself to fight unarmed.
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### learn_difficulty 

 ```
 int value, examples: 5, 10
 Optional 
```


 ### leg_block 

 ```
 int value, examples: 99, 4
 Optional 
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
 str value, examples: No style, Force unarmed
 Required 
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

 ### onattack_buffs:#:buff_duration 

 ```
 int value, examples: 3, 1
 Required 
```



 ### onattack_buffs:#:description 

 ```
 str value, examples: You intentions are known!  It will take you a few moments to sneak attack again.

-50% all damage.
Last 3 turns., A sharp sword cuts true.
Although, all things fade with time.
Restraint hones your skills.

-1.0 Dodge skill, -1 bash damage, -1 cut damage.
Lasts 1 turn.  Stacks 5 times.
 Required 
```



 ### onattack_buffs:#:flat_bonuses 

 ```
 list see "onattack_buffs:#:flat_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onattack_buffs:#:flat_bonuses </summary> 

 ### onattack_buffs:#:flat_bonuses:# 

 ```
 dict see "onattack_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onattack_buffs:#:flat_bonuses:# </summary> 

 ### onattack_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: -1.0
 Required 
```



 ### onattack_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: dodge
 Required 
```



 ### onattack_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: cut
 Required 
```



 ### onattack_buffs:#:id 

 ```
 str value, examples: buff_ninjutsu_onattack, buff_niten_onattack
 Required 
```



 ### onattack_buffs:#:max_stacks 

 ```
 int value, example: 5
 Optional 
```



 ### onattack_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Required 
```



 ### onattack_buffs:#:mult_bonuses 

 ```
 list see "onattack_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onattack_buffs:#:mult_bonuses </summary> 

 ### onattack_buffs:#:mult_bonuses:# 

 ```
 dict see "onattack_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onattack_buffs:#:mult_bonuses:# </summary> 

 ### onattack_buffs:#:mult_bonuses:#:scale 

 ```
 float value, example: 0.5
 Required 
```



 ### onattack_buffs:#:mult_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### onattack_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### onattack_buffs:#:name 

 ```
 str value, examples: Loss of Surprise, Falling Leaf
 Required 
```



 ### onattack_buffs:#:unarmed_allowed 

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

 ### onblock_buffs 

 ```
 list see "onblock_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onblock_buffs </summary> 

 ### onblock_buffs:# 

 ```
 dict see "onblock_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onblock_buffs:# </summary> 

 ### onblock_buffs:#:buff_duration 

 ```
 int value, example: 1
 Required 
```



 ### onblock_buffs:#:description 

 ```
 str value, examples: Your next strike will find its mark much easier from your parry.

+1 Accuracy.
Lasts 1 turn., Each successful block reveals an opening in your opponent's guard.

+1 Accuracy.
Lasts 1 turn.  Stacks 3 times.
 Required 
```



 ### onblock_buffs:#:flat_bonuses 

 ```
 list see "onblock_buffs:#:flat_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onblock_buffs:#:flat_bonuses </summary> 

 ### onblock_buffs:#:flat_bonuses:# 

 ```
 dict see "onblock_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onblock_buffs:#:flat_bonuses:# </summary> 

 ### onblock_buffs:#:flat_bonuses:#:scale 

 ```
 float value, example: 1.0
 Required 
```



 ### onblock_buffs:#:flat_bonuses:#:stat 

 ```
 str value, example: hit
 Required 
```



 ### onblock_buffs:#:id 

 ```
 str value, examples: buff_fencing_onblock, buff_medievalpole_onblock
 Required 
```



 ### onblock_buffs:#:max_stacks 

 ```
 int value, example: 3
 Optional 
```



 ### onblock_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Required 
```



 ### onblock_buffs:#:name 

 ```
 str value, examples: Parry, Defense Break
 Required 
```



 ### onblock_buffs:#:skill_requirements 

 ```
 list see "onblock_buffs:#:skill_requirements:#"
 Required 
```


 <details> 
 <summary> children of onblock_buffs:#:skill_requirements </summary> 

 ### onblock_buffs:#:skill_requirements:# 

 ```
 dict see "onblock_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onblock_buffs:#:skill_requirements:# </summary> 

 ### onblock_buffs:#:skill_requirements:#:level 

 ```
 int value, example: 1
 Required 
```



 ### onblock_buffs:#:skill_requirements:#:name 

 ```
 str value, example: melee
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
 int value, examples: 3, 1
 Required 
```



 ### oncrit_buffs:#:description 

 ```
 str value, examples: You can follow up a critical hit with a stronger attack if the opportunity presents itself.

+15% bonus to all damage.
Enables "Combination Strike" technique.
Lasts 3 turns.  Stacks 3 times., Your powerful attack has given you the chance to end this fight right now!
Enables "Vicious Strike" techniques.
Lasts 1 turn.
 Required 
```



 ### oncrit_buffs:#:flat_bonuses 

 ```
 list see "oncrit_buffs:#:flat_bonuses:#"
 Optional 
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
 float value, examples: 0.5, 1.0
 Required 
```



 ### oncrit_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, examples: str, per
 Required 
```



 ### oncrit_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: arpen, damage
 Required 
```



 ### oncrit_buffs:#:flat_bonuses:#:type 

 ```
 str value, examples: bash, electric
 Required 
```



 ### oncrit_buffs:#:id 

 ```
 str value, examples: buff_eskrima_oncrit, buff_swordsmanship_oncrit
 Required 
```



 ### oncrit_buffs:#:max_stacks 

 ```
 int value, examples: 2, 3
 Optional 
```



 ### oncrit_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### oncrit_buffs:#:mult_bonuses 

 ```
 list see "oncrit_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of oncrit_buffs:#:mult_bonuses </summary> 

 ### oncrit_buffs:#:mult_bonuses:# 

 ```
 dict see "oncrit_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of oncrit_buffs:#:mult_bonuses:# </summary> 

 ### oncrit_buffs:#:mult_bonuses:#:scale 

 ```
 float value, example: 1.15
 Required 
```



 ### oncrit_buffs:#:mult_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### oncrit_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### oncrit_buffs:#:name 

 ```
 str value, examples: Eskrima Combination, Manslayer
 Required 
```



 ### oncrit_buffs:#:skill_requirements 

 ```
 list see "oncrit_buffs:#:skill_requirements:#"
 Optional 
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
 int value, examples: 2, 5
 Required 
```



 ### oncrit_buffs:#:skill_requirements:#:name 

 ```
 str value, examples: melee, unarmed
 Required 
```



 ### oncrit_buffs:#:unarmed_allowed 

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

 ### ondodge_buffs:#:bonus_dodges 

 ```
 int value, example: 1
 Optional 
```



 ### ondodge_buffs:#:buff_duration 

 ```
 int value, examples: 1, 2
 Required 
```



 ### ondodge_buffs:#:description 

 ```
 str value, examples: You've seen your chance.  Now strike back!

+25% Bash damage.
Lasts for 1 turn., Much like the crane, you a quick to avoid danger.

+1 Dodge attempts, +1.0 Dodge skill.
Lasts 2 turns.
 Required 
```



 ### ondodge_buffs:#:flat_bonuses 

 ```
 list see "ondodge_buffs:#:flat_bonuses:#"
 Optional 
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
 float value, examples: 1.0, 0.15
 Required 
```



 ### ondodge_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, examples: dex, per
 Optional 
```



 ### ondodge_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: arpen, dodge
 Required 
```



 ### ondodge_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### ondodge_buffs:#:id 

 ```
 str value, examples: buff_boxing_ondodge, buff_crane_ondodge
 Required 
```



 ### ondodge_buffs:#:max_stacks 

 ```
 int value, examples: 3, 4
 Optional 
```



 ### ondodge_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### ondodge_buffs:#:mult_bonuses 

 ```
 list see "ondodge_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of ondodge_buffs:#:mult_bonuses </summary> 

 ### ondodge_buffs:#:mult_bonuses:# 

 ```
 dict see "ondodge_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of ondodge_buffs:#:mult_bonuses:# </summary> 

 ### ondodge_buffs:#:mult_bonuses:#:scale 

 ```
 float value, examples: 1.25, 1.1
 Required 
```



 ### ondodge_buffs:#:mult_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### ondodge_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### ondodge_buffs:#:name 

 ```
 str value, examples: Counter Chance, Crane's Grace
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
 int value, examples: 1, 5
 Required 
```



 ### ondodge_buffs:#:skill_requirements:#:name 

 ```
 str value, examples: unarmed, melee
 Required 
```



 ### ondodge_buffs:#:unarmed_allowed 

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
 str value, example: Taking a hit will not slow you down.  You will outlast your opponent and win this fight.

+Bash damage increased by 25% of Strength, blocked damage decreased by 50% of Strength.
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
 float value, example: 0.25
 Required 
```



 ### ongethit_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, example: str
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
 str value, example: buff_muay_thai_ongethit
 Required 
```



 ### ongethit_buffs:#:name 

 ```
 str value, example: Determination
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
 int value, example: 3
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

 ### onhit_buffs:#:bonus_blocks 

 ```
 int value, example: 2
 Optional 
```



 ### onhit_buffs:#:bonus_dodges 

 ```
 int value, example: 1
 Optional 
```



 ### onhit_buffs:#:buff_duration 

 ```
 int value, examples: 1, 2
 Required 
```



 ### onhit_buffs:#:description 

 ```
 str value, examples: Life and combat are a circle.  An attack leads to a counter and to an attack once again.  Seek to complete this loop.

+1 Accuracy, +2 bash Damage.
Enables "Dragon Vortex Block" and "Dragon Wing Dodge"
Lasts 1 turn., Landing a hit allows you to perfectly position yourself for maximum defense against multiple opponents.

+2 Block attempts, +1 Dodges attempts, blocked damge reduced by 50% of Strength.
Lasts 2 turns.
 Required 
```



 ### onhit_buffs:#:flat_bonuses 

 ```
 list see "onhit_buffs:#:flat_bonuses:#"
 Optional 
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
 float value, examples: 1.0, 0.5
 Required 
```



 ### onhit_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, example: str
 Optional 
```



 ### onhit_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: hit, block
 Required 
```



 ### onhit_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### onhit_buffs:#:id 

 ```
 str value, examples: buff_dragon_onhit, buff_karate_onhit
 Required 
```



 ### onhit_buffs:#:max_stacks 

 ```
 int value, examples: 4, 3
 Optional 
```



 ### onhit_buffs:#:mult_bonuses 

 ```
 list see "onhit_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onhit_buffs:#:mult_bonuses </summary> 

 ### onhit_buffs:#:mult_bonuses:# 

 ```
 dict see "onhit_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onhit_buffs:#:mult_bonuses:# </summary> 

 ### onhit_buffs:#:mult_bonuses:#:scale 

 ```
 float value, examples: 1.2, 1.1
 Required 
```



 ### onhit_buffs:#:mult_bonuses:#:stat 

 ```
 str value, examples: damage, movecost
 Required 
```



 ### onhit_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### onhit_buffs:#:name 

 ```
 str value, examples: Dragon's Flight, Karate Strike
 Required 
```



 ### onhit_buffs:#:req_buffs 

 ```
 list see "onhit_buffs:#:req_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onhit_buffs:#:req_buffs </summary> 

 ### onhit_buffs:#:req_buffs:# 

 ```
 str value, example: buff_pankration_ondodge
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
 int value, examples: 1, 3
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

 ### onkill_buffs:#:bonus_dodges 

 ```
 int value, example: 2
 Optional 
```



 ### onkill_buffs:#:buff_duration 

 ```
 int value, examples: 3, 5
 Required 
```



 ### onkill_buffs:#:description 

 ```
 str value, examples: Your target has perished.  It is time to leave and plan your next attack.

+2 Dodge attempts, +10 movement speed.
Last 3 turns., YOU ARE ON FIRE!  +5 fire damage for 5 turns.
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
 float value, examples: 10.0, 5.0
 Required 
```



 ### onkill_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: speed, damage
 Required 
```



 ### onkill_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: heat
 Optional 
```



 ### onkill_buffs:#:id 

 ```
 str value, examples: buff_ninjutsu_onkill, debug_kill_buff
 Required 
```



 ### onkill_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### onkill_buffs:#:name 

 ```
 str value, examples: Escape Plan, On Fire
 Required 
```



 ### onkill_buffs:#:skill_requirements 

 ```
 list see "onkill_buffs:#:skill_requirements:#"
 Optional 
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
 int value, example: 3
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

 ### onmiss_buffs 

 ```
 list see "onmiss_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onmiss_buffs </summary> 

 ### onmiss_buffs:# 

 ```
 dict see "onmiss_buffs:#:" values
 Required 
```


 <details> 
 <summary> children of onmiss_buffs:# </summary> 

 ### onmiss_buffs:#:buff_duration 

 ```
 int value, examples: 2, 1
 Required 
```



 ### onmiss_buffs:#:description 

 ```
 str value, examples: You didn't miss, it's just part of the dance and the best part is about to start!

+15% Bash damage.
Lasts 2 turns.  Stacks 3 times., Your feint is the perfect setup for a devastating followup attack!

+1 Accuracy.
Enables "Compound Attack" technique.
Lasts 1 turn.
 Required 
```



 ### onmiss_buffs:#:flat_bonuses 

 ```
 list see "onmiss_buffs:#:flat_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onmiss_buffs:#:flat_bonuses </summary> 

 ### onmiss_buffs:#:flat_bonuses:# 

 ```
 dict see "onmiss_buffs:#:flat_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onmiss_buffs:#:flat_bonuses:# </summary> 

 ### onmiss_buffs:#:flat_bonuses:#:scale 

 ```
 float value, examples: 1.0, 2.0
 Required 
```



 ### onmiss_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: hit, damage
 Required 
```



 ### onmiss_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### onmiss_buffs:#:id 

 ```
 str value, examples: buff_capoeira_onmiss, buff_fencing_onmiss
 Required 
```



 ### onmiss_buffs:#:max_stacks 

 ```
 int value, examples: 3, 5
 Optional 
```



 ### onmiss_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### onmiss_buffs:#:mult_bonuses 

 ```
 list see "onmiss_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of onmiss_buffs:#:mult_bonuses </summary> 

 ### onmiss_buffs:#:mult_bonuses:# 

 ```
 dict see "onmiss_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of onmiss_buffs:#:mult_bonuses:# </summary> 

 ### onmiss_buffs:#:mult_bonuses:#:scale 

 ```
 float value, example: 1.15
 Required 
```



 ### onmiss_buffs:#:mult_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### onmiss_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### onmiss_buffs:#:name 

 ```
 str value, examples: Capoeira Tempo, Remise
 Required 
```



 ### onmiss_buffs:#:skill_requirements 

 ```
 list see "onmiss_buffs:#:skill_requirements:#"
 Optional 
```


 <details> 
 <summary> children of onmiss_buffs:#:skill_requirements </summary> 

 ### onmiss_buffs:#:skill_requirements:# 

 ```
 dict see "onmiss_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of onmiss_buffs:#:skill_requirements:# </summary> 

 ### onmiss_buffs:#:skill_requirements:#:level 

 ```
 int value, examples: 3, 1
 Required 
```



 ### onmiss_buffs:#:skill_requirements:#:name 

 ```
 str value, examples: melee, unarmed
 Required 
```



 ### onmiss_buffs:#:unarmed_allowed 

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

 ### onmove_buffs:#:bonus_blocks 

 ```
 int value, examples: -2, -1
 Optional 
```



 ### onmove_buffs:#:bonus_dodges 

 ```
 int value, examples: 1, 2
 Optional 
```



 ### onmove_buffs:#:buff_duration 

 ```
 int value, examples: 1, 3
 Required 
```



 ### onmove_buffs:#:description 

 ```
 str value, examples: You are make yourself harder to hit by bobbing and weaving as you move.

+1.0 Dodge skill.
Lasts for 1 turns.  Stacks 2 times., You can feel the rhythm as you move.  Not only are you harder to hit, but your kicks are even more amazing!

+1.0 Dodge skill.
Enables "Spin Kick" and "Sweep Kick" techniques.
Lasts 3 turns.
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
 float value, examples: 1.0, -0.5
 Required 
```



 ### onmove_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, examples: str, dex
 Optional 
```



 ### onmove_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: dodge, block
 Required 
```



 ### onmove_buffs:#:id 

 ```
 str value, examples: buff_boxing_onmove, buff_capoeira_onmove
 Required 
```



 ### onmove_buffs:#:max_stacks 

 ```
 int value, example: 2
 Optional 
```



 ### onmove_buffs:#:melee_allowed 

 ```
 bool value, example: True
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
 float value, examples: 1.25, 1.1
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
 str value, examples: Footwork, Capoeira Momentum
 Required 
```



 ### onmove_buffs:#:req_buffs 

 ```
 list see "onmove_buffs:#:req_buffs:#"
 Optional 
```


 <details> 
 <summary> children of onmove_buffs:#:req_buffs </summary> 

 ### onmove_buffs:#:req_buffs:# 

 ```
 str value, example: buff_leopard_onmove1
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
 int value, examples: 2, 1
 Required 
```



 ### onmove_buffs:#:skill_requirements:#:name 

 ```
 str value, examples: unarmed, melee
 Required 
```



 ### onmove_buffs:#:unarmed_allowed 

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
 int value, examples: 2, 1
 Required 
```



 ### onpause_buffs:#:description 

 ```
 str value, examples: The eye of the storm,
a fleeting moment of peace,
gone without a trace.

+2 Accuracy, Dodge skill increased by 50% of Perception.
Lasts 2 turns., Every snake wait for the perfect moment to strike.  Aim as your opponents approve and attack their weakness without mercy!

+1 Accuracy, gain armor penetration equal to 50% of Perceptions.
Lasts 1 turn.  Stacks 3 times.
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
 float value, examples: 2.0, 1.0
 Required 
```



 ### onpause_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, example: per
 Required 
```



 ### onpause_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: hit, block
 Required 
```



 ### onpause_buffs:#:flat_bonuses:#:type 

 ```
 str value, example: bash
 Optional 
```



 ### onpause_buffs:#:id 

 ```
 str value, examples: buff_niten_onpause, buff_snake_onpause
 Required 
```



 ### onpause_buffs:#:max_stacks 

 ```
 int value, example: 3
 Optional 
```



 ### onpause_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### onpause_buffs:#:name 

 ```
 str value, examples: Stillness, Snake's Coil
 Required 
```



 ### onpause_buffs:#:skill_requirements 

 ```
 list see "onpause_buffs:#:skill_requirements:#"
 Optional 
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
 int value, examples: 1, 4
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
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### primary_skill 

 ```
 str value, examples: cutting, bashing
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

 ### static_buffs:#:bonus_blocks 

 ```
 int value, examples: 1, 2
 Optional 
```



 ### static_buffs:#:bonus_dodges 

 ```
 int value, example: 1
 Optional 
```



 ### static_buffs:#:description 

 ```
 str value, examples: By disregarding offensive in favor of self-defense, you are better at protecting.

Blocked damage reduced by 100% of Dexterity., A solid stance allows you block more damage than normal and deliver better punches.

+2 Bash damage, Blocked damge reduced by 50% of Strength.
 Required 
```



 ### static_buffs:#:flat_bonuses 

 ```
 list see "static_buffs:#:flat_bonuses:#"
 Optional 
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
 float value, examples: 0.5, 1.0
 Required 
```



 ### static_buffs:#:flat_bonuses:#:scaling-stat 

 ```
 str value, examples: str, dex
 Optional 
```



 ### static_buffs:#:flat_bonuses:#:stat 

 ```
 str value, examples: block, hit
 Required 
```



 ### static_buffs:#:flat_bonuses:#:type 

 ```
 str value, examples: bash, cut
 Optional 
```



 ### static_buffs:#:id 

 ```
 str value, examples: buff_aikido_static1, buff_boxing_static
 Required 
```



 ### static_buffs:#:melee_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### static_buffs:#:mult_bonuses 

 ```
 list see "static_buffs:#:mult_bonuses:#"
 Optional 
```


 <details> 
 <summary> children of static_buffs:#:mult_bonuses </summary> 

 ### static_buffs:#:mult_bonuses:# 

 ```
 dict see "static_buffs:#:mult_bonuses:#:" values
 Required 
```


 <details> 
 <summary> children of static_buffs:#:mult_bonuses:# </summary> 

 ### static_buffs:#:mult_bonuses:#:scale 

 ```
 float value, examples: 1.5, 1.33
 Required 
```



 ### static_buffs:#:mult_bonuses:#:stat 

 ```
 str value, example: damage
 Required 
```



 ### static_buffs:#:mult_bonuses:#:type 

 ```
 str value, example: bash
 Required 
```



 ### static_buffs:#:name 

 ```
 str value, examples: Aikido Stance, Boxing Stance
 Required 
```



 ### static_buffs:#:quiet 

 ```
 bool value, example: True
 Optional 
```



 ### static_buffs:#:skill_requirements 

 ```
 list see "static_buffs:#:skill_requirements:#"
 Optional 
```


 <details> 
 <summary> children of static_buffs:#:skill_requirements </summary> 

 ### static_buffs:#:skill_requirements:# 

 ```
 dict see "static_buffs:#:skill_requirements:#:" values
 Required 
```


 <details> 
 <summary> children of static_buffs:#:skill_requirements:# </summary> 

 ### static_buffs:#:skill_requirements:#:level 

 ```
 int value, examples: 3, 7
 Required 
```



 ### static_buffs:#:skill_requirements:#:name 

 ```
 str value, examples: unarmed, melee
 Required 
```



 ### static_buffs:#:stealthy 

 ```
 bool value, example: True
 Optional 
```



 ### static_buffs:#:throw_immune 

 ```
 bool value, example: True
 Optional 
```



 ### static_buffs:#:unarmed_allowed 

 ```
 bool value, example: True
 Optional 
```



 ### static_buffs:#:unarmed_weapons_allowed 

 ```
 bool value, example: False
 Optional 
```



 </details>
</summary>


 </details>
</summary>


 </details>
</summary>

 ### strictly_melee 

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
 str value, examples: tec_aikido_break, tec_boxing_rapid
 Required 
```



 </details>
</summary>


 </details>
</summary>

 ### type 

 ```
 str value, example: martial_art
 Required 
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
 str value, examples: baton-extended, bagh_nakha
 Required 
```


