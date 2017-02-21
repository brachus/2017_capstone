
- (exits + items are now charas, BUT...) implement 'placers' for world/rooms.
	2 types so far:
		player placer: any human/cpu player may be placed here.
		npc placer: any helper npc may be placed here.  possibly a weight to indicate strength of npc??

- Integrate room structure into main loop:
		have all active characters be declared so from room structs.
		
		before battle mode:
			which room?
			which difficulty? 
		
		starting battle mode:
			select world, ## LEAVE THIS UNDONE select world ver.
				(different placement based on difficulty/game type/# players )
			
			make active charas from all templates placed in world.
			
			how many players?
			
			which player are we focusing on?
			
			select world
			
			
			
			
			
		
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
