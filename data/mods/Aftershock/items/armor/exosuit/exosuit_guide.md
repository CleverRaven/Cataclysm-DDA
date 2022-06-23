# Exosuit Guide


### Background
### Frame
The frame of the exosuit is a relatively simple design, easy to manufacture.  A titanium frame lined with artificial muscles, studded with connection points allowing quick installation and swapping of plating and modules without specialized equipment. A helmet can be attached to the frame, and the weight is supported by the frame without restricting the pilot's field of view.
### Underlayer
These are comfortable jumpsuits specially designed to integrate with the frame.  Underlayers are not required for operation, but prevent accidental injury as the frame moves.  Specialized undersuits are built for various environments or combat.
### PSU
Exosuits typically use a single large power supply unit, attached to the back.  Auxiliary batteries can be attached as well across the frame, extending the runtime. 

| Name        | Capacity | Fuel    | Weight | Volume
| ---         | ---      | ---     | ---    | ---
| Battery PSU | 40000    | Battery | 15 kg  | 20 L
| ICE PSU     | 10000    | Gasoline| 20 kg  | 25 L
| Large Aux   | 4000     | Battery | 5 kg   |  5 L
| Small Aux   | 1000     | Battery | 2 kg   | 1 L


### Plating
Each plating has a minimum weight of 500 g to represent the connection points.  The steel and plastic are based on 6mm thickness, while the VCC is 4mm thick to keep weight managable.  Exosuits can accept a wide range of plating.  Plastic, steel, and vaccum-cast carbide are common.  Improvised armor has been found on frontiers and in the underworld.  The plates are rounded, using a ball-and-joint system somewhat resembling a hard diving or EVA suit.

| Plating              | Weight   
| ---                  | ---
| Plastic              | 11.01 kg
| Tempered Steel       | 44.05 kg
| Vaccum-cast Carbide  | 65.41 kg 
   
### Modules
A variety of modules are available, ranging from sensor packages to batteries to load-bearing modules to jetpacks.  These are built in a swappable black box configuration, requiring only that the module be mounted on the suit's connection points. 

Modules come in Small, Medium, Large, Helmet, and PSU. The helmet modules can only be mounted in the exosuit helmet, and the PSU has a unique slot off of the torso.  Modules cannot exceed the volume or weight of the module class. Small and medium modules can fit into larger module slots.


| Module Class | Volume | Weight
| ---          | ---    | ---   
| Large        |   10 L |  10 kg
| Medium       |  2.5 L |   5 kg
| Small        |  0.5 L | 0.5 kg
| Helmet       | 0.25 L | 0.5 kg
| PSU          |   75 L |  75 kg

| Location | Weight Cap | Volume Cap | Large | Medium | Small | Helmet | PSU
| ---      | ---        | ---        | ---   | ---    | ---   | ---    | ---
| Helmet   |  2 L       |  5 kg      | No    | No     | No    | Yes    | No
| Torso    |  4 L       | 15 kg      | Yes   | Yes    | Yes   | No     | No 
| Arm      | 10 L       | 10 kg      | No    | Yes    | Yes   | No     | No
| Leg      | 10 L       | 10 kg      | No    | Yes    | Yes   | No     | No
| PSU      | 75 L       | 75 kg      | Yes   | Yes    | Yes   | No     | Yes

| Module         | Class  | Power Draw | Weight | Description
| ---            | ---    | ---        | ---    | ---
| Large Support  | Large  | 1500       | 10 kg  | Adds an additional 30 kg of carry weight while active.
| Small Support  | Small  | 1000       |  2 kg  | Adds an additional 20 kg of carry weight while active.
| Zoom Lenses    | Helmet | ---        | 500 g  | Corrects vision and provides optical zoom.
| Mounted Lamp   | Small  | 1000/2000  | 500 g  | A sturdy lamp with a low and high setting.
| Recoil Control | Small  | 2000       | 500 g  | Reduces weapon dispersion while active.
| Air Supply     | Medium | ---        |  2 kg  | A regenerating oxygen supply, activates in emergencies.
| Imager         | Helmet | 50 per use | 500 g  | As the handheld tool, sees through walls.
| Climb Assist   | Medium | ---        |  5 kg  | Allows the user to climb walls.
| Jetpack        | Large  | ---        | 10 kg  | An ammonia-fueled jetpack provides a dash across terrain.
| Forcefield     | Medium | 5000       |  5 kg  | Toggles between 25 bash/cut or 40 ballistic/pierce protection.
| Med Injector   | Small  | ---        | 500 g  | Injects RA21E to treat injuries.

| Storage Module   | Volume Cap | Weight Cap | Class  | Weight | Description
| ---              | ---        | ---        | ---    | ---    | ---
| Rifle Storage    | 9 L        | 10 kg      | Large  | 500 g  | As item Back Holster
| Melee Storage    | 3750 ml    |  5 kg      | Medium | 500 g  | As item Back Scabbard
| Storage Pack     | Multiple   | Multiple   | Large  | 2250 g | As item Large Tactical Backpack
| Medium Pouch     | 2500 ml    | 5 kg       | Medium | 275 g  |As item Fanny Pack
| Small Pouch      | 1500 ml    | 5 kg       | Small  | 125 g  | As item Leather Pouch
| Large Ammo Pouch | Multiple   | Multiple   | Large  | 1750 g | As item Tac Vest
| Small Ammo Pouch | 2 L        | 4 kg       | Medium | 1050 g | As item Drop Leg Pouches

### Operation
The basic operation is based on the frame mirroring the pilot's motions.  This works well for movement, though fine manipulation takes practice as pilots must adapt to the suit's feedback.  Once certified, a pilot receives additional training in their field, learning to use their specialized equipemnt.  The exosuit is easily outfitted for hazardous environments ranging from frozen tundra to burning deserts, from underwater to orbit.

### Repairs
The frame itself is somewhat difficulty to repair due to the complex components and the titanium used.
Plating (aside from carbide) can be field repaired as long as the plate has not been completely destroyed.  Destroyed plates can have their connectors salvaged and used to create new plates with the right equipment.  Modules are complicated electronics packed into very tight containers, making repairs extremely difficult.  Specialized toolkits are issued for each frame allowing field repairs, and powered gantries are available to support the suit for more intensive repairs while recharging.
See Exosuit Repair Kit, Exosuit Maintenance Gantry


# Balance
### Power Consumption
The inital power draw for the frame is based on the Aftershock suit operation time, 45 hours for the frame alone.  Adding plating and modules increases the weight, requiring power hungry support modules for all but the strongest pilots.  With the frame, helmet, battery PSU and carbide plating, the entire suit weighs just under 126 kg, before any additonal equipment is added.  For a default character, this means they would need several support modules just to move, and the minimum number would cut the runtime down to 21 hours.  Further modules add additional weight and potentially power draw.

### Inspirations
BattleTech, Beam Saber, Jovian Chronicles, Silent Storm, Fallout, Patlabor.