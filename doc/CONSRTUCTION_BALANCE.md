# Construction Balance

	# Digging:
    
    When we dig, we're generally digging out either a deep or shallow pit.
    
    The dimensions are a little hand-wavey... based on our exactly
    as-big-as-they-need-to-be tile sizes, we could assume that the width and height
    are 1 meter each. Our pit could be a square as well, or we could assume it's
    circular and 1 meter in diameter.
    
    Depth is even less rigidly defined, both in terms of "what is a z-level", and in
    terms of how the deep and shallow pits are used in game.
    
    A shallow pit gets used to do things like build an improvised shelter or start
    the foundation for a wall, and is ostensibly a relatively quick effort: a
    survivor might get a crude digging implement (e.g. digging stick) and attempt to
    dig out the start of an improvised shelter, or they might have a proper shovel
    and be digging a foundation footing. Referencing the 2018 IBC
    https://codes.iccsafe.org/content/IRC2018/chapter-4-foundations we can see in
    R403.1.4 the requirement that "exterior footings shall be placed not less than
    12 inches (305 mm) below the undisturbed ground surface." You'd need even more
    space for the actual footing, but that's close enough. I don't think survivors
    care about building to code, but let's call it 12 inches (0.3048 meters) at the
    maximum for a shallow pit depth.
    
    The deep pit is a little more complicated because it gets used for more complex
    constructions like traps, reinforced concrete wall footings, palisades, wells,
    root cellars, and the like. The depth requirements for those are quite varied,
    but let's just throw some number around to get a feel for it: say at least 2
    meters deep to be an effective pit trap. The USGS maintains
    depth-to-ground-water-level records and provides them at
    https://waterdata.usgs.gov/nwis/current/?type=gw. A cursory review (and
    remembering it's seasonal) shows values in Massachusetts ranging from 122 feet
    and 1 foot, and values between 3 to 9 feet aren't uncommon. Root cellars are
    ideally constructed where the ground temperature has stabilized, below the frost
    line. This depth varies by location, but is somewhere in the 1 to 3 meter range.
    
    With all of that in hand, let's make some estimates.
    
    A shallow pit is a circular pit 1 meter in diameter and 0.3048 meters deep,
    which works out to... ~0.239 m^3. That's so close, let's just call it 0.25 m^3
    or 250 liters.
    
    A deep pit is a rectangular pit 1m x 1m x 2m, or 2 m^3, or 2000 liters.
    
    Now, for how long that takes, things are going to get even more subjective and
    couched in assumptions. Let's assume a single individual, digging in optimally
    diggable soil(rather than something requiring a pickaxe to break the soil or
    requiring saws to remove tree roots), using an appropriate (but manual)
    implement designed for the task (e.g. a shovel). The Canadian Centre for
    Occupational Health and Safety has some interesting recommendations on the rate
    of shoveling, weight of the load, and throw distance at
    https://www.ccohs.ca/oshanswers/ergonomics/shovel.html. Of particular interest
    is the table of "recommended workload for continuous shoveling", which gives a
    weight per minute and a total weight per 15 minutes, as well as a description of
    the conditions. It also discusses the need to take breaks, for example
    alternating 15 minutes of shoveling and 15 minutes of rest in extreme
    conditions. Taking all that into consideration, I'm going to call it 10
    scoops/min * 5 kg/scoop for 15 minutes followed by 15 minutes of rest, or
    effectively 25 kg/min.
    
    Now we need to bring the weights and volumes together. Again, more hand waving
    as the soil composition is going to have a big influence on this. The
    engineering toolbox https://www.engineeringtoolbox.com/dirt-mud-densities-d_1727.html
    lists the density of wet and dry versions of many materials. I'm going with the
    assumption that this is moist/wet soil that includes clay, silt, load, as well as
    some rock, so I'll just call it 1700 kg/m^3 on average.
    
    Shallow pit is 0.25 m^3 * 1700 kg/m^3, or 425 kg.
    Deep pit is 2 m^3 * 1700 kg/m^3, or 3400 kg.
    
    We'll do some variables below, but for reference:
    Shallow pit: 425 kg / 25 kg/min = 17 minutes
    Deep pit: 3400 kg / 25 kg/min = 136 minutes
    
    Now, one addendum: we're digging our deep pit in the location that we've already
    dug the shallow pit, so really we should exclude the shallow pit volume from the
    deep when counting the amount of work we need to do.
    
    Adjusted deep pit: ( 3400 kg - 425 kg ) / 25 kg/min = 119 minutes
