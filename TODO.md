
## COMPLETE ## - implement, and create pick-up items (health, elemental prototype)

- implement loading sprites/chara templates from external file all together.

- implement room switching, start with castle and underground passage.

- implement fire ball
	requires:
	implement paths.  path node: # frames, mode;  path head: dat, 

- with finished passage way tiles, create an example using multiple layers.

- have rooms be able to select 1 layer out of multiple layers in json tilemaps,
	so two different rooms may use different layers from same json tilemap.
	
- allow boxes in tilemap editor be called "castle", which would have same effect as "wall",
	but would help with drawing the map.

- implement inventory, mockups first for designing the window.  should for now be able to "USE" or "COMBINE" items.  items
	should have descriptions.  possibly have item library?  some items may just sit there, as they might be weapons or weapon upgrades.
	




- wall objects in tile maps.  any tiles that are within or touching this object, this becomes clip area for player.
-  OK, ensure this is as bug free as can be.



- implement function for checking if an active character is hit  by one  more 
    action frames.  this should also effectively also calculate a net force for 
    impact.
    
- implement function for taking damage. 

- add "interact" flag for charas.  this may be set to null, "collide", or "active".


- implement function for checking if any given character interacts with some other character
    by colliding, or in case of being a player, actively interacting with another character.
    this should populate a Tag List.  
    
    Each tag should consist of:
		Type flag - for some type of interaction.
			e.g.:
				

			
			
			
			
		
-  + Exits;  implement transitions into loop.
	Exits can specify specific transition (e.g. fall effect, fade effect. )

- Add glint effect, flicker effect, etc. to render sprite structure.

-  Implement item roster, which would keep track of all active items.

-  Implement weapon select
-  + HUD weapon select.

-  Implement inventory.

-  Implement clipping / collision detection for charas

-  destructable props, bomb props, character attack,

-  Camera shake.

-  2nd player (AI ? )
