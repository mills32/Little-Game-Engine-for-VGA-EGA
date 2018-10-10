# Little-Game-Engine-for-VGA-MCGA
Little Game Engine for MCGA/VGA and 8086/286

	A lot of code from David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	This is a 16-bit program, it will only work on MS-DOS and windows 98.                     
	Remember to compile in the LARGE memory model!                        

	This program is intended to work on MS-DOS, 8086/286, and MCGA (or VGA) video.     
	Yes you read well, an 8086 with an MCGA is capable of:
	- Load and draw 256 colours bmp.
	- Load and draw sprites (8x8, 16x16 and 32x32),with transparency and animations.
	- Load and draw TMX maps, created with tiled: https://www.mapeditor.org/
	- Hardware scroll (70 fps).
	- Palette animations.
	- Load and play IMF music for ym3812, OPL2, Adlib, or Sound Blaster compatible. 

	The "BAD" things:
	- An 8088 will run the engine, but it won't be able to draw more than one sprite without slowdowns and flickering.
	- An 8086 can only draw 4 or 5 sprites without flickering.
	- The 304x176 resolution hides the borders being updated, but some monitors may not work.	
	
	Please feel free to copy this source code.
	
"Little Game Engine" para MCGA/VGA y 8086/286

	Un montón de código copiado de David Brackeen                                   
	http://www.brackeen.com/home/vga/                                     

	Este es un programa de 16-bit, funcionará en MS-DOS y Windows 98.                     
	Recuerda que hay que compilarlo usando el "LARGE memory model".                        

	Este programa está optimizado para funcionar en MS-DOS, con un procesador 8086, y una tarjeta MCGA (o VGA).     
	Si, has leído bien, un 8086 con una MCGA is capaz de:
	- Cargar archivos bmp de 256 colores y dibujarlos.
	- Cargar sprites de 8x8, 16x16 y 32x32, con transparencia y con animaciones.
	- Cargar mapas TMX, creados con el programa tiled: https://www.mapeditor.org/
	- Scroll de los mapas a 70 fps.
	- Animaciones de paleta, puedes cambiar 8 primeros colores de la paleta para hacer animaciones.
	- Cargar y reproducir música en formato IMF para el chip ym3812, OPL2, o Adlib, o cualquier Sound Blaster conpatible. 

	Las cosas "MALAS":
	- Un 8088 con VGA cargará el motor, pero no podrá dibujar mas de un sprite sin que haya ralentizaciones y paradeos.
	- Un 8086 solo podrá dibujar 3 o 4 sprites de 16x16 sin parpadear. Si añades más, pues pardadearán bastante.
	- La resolución de 304x176 oculta los bordes que son actualizados para el scroll, pero algunos monitores antiguos no funcionarán.	
	
	Este código es complétamente libre, cópialo o haz lo que quieras con el.
	
	
NOTES
-----
Use only indexed bmp files with 256 colours.
Little Engine uses 16x16 pixels tiles, and will split the tilesets in chunks of 16x16 pixels (from left to right / top to bottom).
Sprites are split in chunks of 8x8,16x16 or 32x32. 
TMX maps must be in XML format and orthogonal.
IMF files max size is 64 Kb.

NOTAS
-----
Usa solamente archivos bmp indexados con 256 colores.
Little Engine usa tiles de 16x16, y corta los tilesets en trozos de 16x16 (de izquierda a derecha y de arriba a abajo).
Los sprites serán divididos en trozos de 8x8,16x16 or 32x32. 
Los mapas TMX deben estar en formato XML y ortogonal.
Los archivos IMF no pueden superar los 64 Kb.