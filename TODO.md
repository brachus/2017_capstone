
TODO's for prototype:

-  Implement basic script animation system:
	<overall, command based.  everything is separated by semicolons>
	load "path" - load image file as sprite.  centerxy is center of image.
			image is refered to by indexes generated from 0.
	post idx x y   - using index from load commands, x and y.
	unpost idx    - remove images by index. 
	next          - next frame.  images posted remain visible until unposted.

-  Use animations for title menu.
-  Implement weapon select
-  Implement clipping
-  destructable props, bomb props, character attack,
-  2nd player (AI ? )
