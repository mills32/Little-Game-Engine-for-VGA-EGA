# Little-Game-Engine-for-VGA-MCGA
Little Game Engine for MCGA/VGA and 8086/286

	Image loader from bitmap.c by David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program, to compile in the LARGE memory model                                                                      
	                  
	This program is intended to work on MS-DOS, 8086, MCGA (or VGA) video.     
	Yes you read well, an 8086 with an MCGA is capable of:
		- Store and display tiles to fill the entire screen (64 Kb of VRAM)
		- Hardware scrolling (yes, just 3 or 4 games used that).
		- Copy tiles from ram to the borders of the VRAM (very fast) to scroll around big maps.
		- Draw sprites (16x16 pixels) on top of the map.
		- Do all this keeping 60 FPS!!
	
	The "BAD" things
		- An 8086 can only draw 4 or 5 sprites without flickering.
		- It uses 304x184 resolution, some monitors will fail, windows 95, 98 will fail(use only in pure DOS)	
	                          
