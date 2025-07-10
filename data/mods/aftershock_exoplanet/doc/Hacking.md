# Hacking
### Hackable Furniture
To define a Hackable Furniture in Aftershock you need to give it an examine action that triggers an EOC. This EOC must call ```EOC_start_lock_hack``` and must set up several essential variables. Refer to the following chart below for what variables are available to you and which ones are required to be defined. When running the EOC be sure to set the Alpha talker to the Avatar so the EOCs can run correctly.


| Variable Name | Description | Context | Required |
|-|-|-|-|
|furn_pos   |The location of the furniture. Supply pos variable from the examine action.<br/>```"furn_pos": { "context_val": "pos" }``` | EOC Context | ✓ |
|t_delay    |The base amount of time it should take to hack this furniture. It is best defined before calling the EOC.<br/>```{ "math": [ "t_delay = time('20min')" ] }``` | EOC Context | ✓ |
|difficulty |The difficulty of hacking this furniture. To hack this furniture the player must beat this number by rolling a d4 and adding their hack skill | EOC Context | ✓ |
|t_radius   |The radius around this furniture in which the ter-fur transform is applied. | EOC Context | ✓ |
|power_cost_mult | Multiplier applied to the total power cost of the hack if a tool or bionic is used. This can be used to increase the total power requirement of a hack without making it take longer to complete.| EOC Context |
|hack_minor_failure_eoc |The ID of the EOC to run when the player minorly fails at hacking this furniture. Leaving this blank will trigger the generic hack minor failure EOC to run. | User |
|hack_critical_failure |The ID of the EOC to run when the player critically fails at hacking this furniture. Leaving this blank will trigger the generic critical failure EOC to run. However, you will still need to add this furniture to the multi-lockdown ter-fur transform.| User
|hack_sucess_eoc | The ID of the EOC to run when the player succeeds at hacking the furniture. If this is left blank the generic success EOC will run. However, you will still need to add the furniture to the multi-unlock ter-fur transform.| User

```
    "examine_action": {
      "type": "effect_on_condition",
      "effect_on_conditions": [
        {
          "id": "EOC_unlock_afs_display_case",
          "effect": [
            { "math": [ "_t_delay = time('20 m')" ] },
            {
              "run_eocs": "EOC_start_lock_hack",
              "variables": { "furn_pos": { "context_val": "pos" }, "t_delay": { "context_val": "t_delay" }, "difficulty": "10", "t_radius": "0" },
              "alpha_talker": "avatar"
            }
          ]
        }
      ]
    },
```

```
    "examine_action": {
      "type": "effect_on_condition",
      "effect_on_conditions": [
        {
          "id": "EOC_unlock_afs_security_panel",
          "effect": [
            { "math": [ "_t_delay = time('20 m')" ] },
            { "u_add_var": "hack_minor_failure_eoc", value: "EOC_Hack_Custom_Minor_Failure" },
            { "u_add_var": "hack_critical_fail_eoc", value: "EOC_Hack_Custom_Critical_Failure" },
            { "u_add_var": "hack_success_eoc", value: "EOC_Hack_Custom_Success" },
            {
              "run_eocs": "EOC_start_lock_hack",
              "variables": { "furn_pos": { "context_val": "pos" }, "t_delay": { "context_val": "t_delay" }, "difficulty": "10", "t_radius": "6", "power_cost_mult": 2 },
              "alpha_talker": "avatar"
            }
          ]
        }
      ]
    },
```

### Hacking tools
Hacking tools require relatively little setup to work within our system. Simply make sure your item has the hack tool quality. Electronic tools require no adjustment and work right out of the gate. Bionic hacking tools also work straight away provided they are flagged with ```USE_BIONIC_POWER```. To make an item that doesn't consume charges at all simply flag it with ```HACK_NO_CHARGE```.

| Quality Level | Description |
|-|-|
| 1 | Electronics that could technically be used for hacking but offer virtually no benefits for it whatsoever. |
| 2 | Improvised Hacking tools that provide some benefit for hacking but fall short when compared to well-manufactured equipment.
| 3 | Small Corporate Infiltrator hacking gear. Providing a suite of hacking utilities. |
| 4 | Military Grade Hacking Gear. The stuff that UICA and the big Corpos use. |
| 5 | Elite Infiltrator Gear. The top-class equipment for that is still technically manufacturable. |
| 6 | Glittertech Hacking Equipment. Tools of this level aren't manufacturable anymore and can only be found in pre-discontinuity ruins.