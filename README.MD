# Little-Game-Engine-for-VGA-MCGA
Little Game Engine for MCGA/VGA and 8086/286
--------------------------------------------

A lot of code from David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

This is a 16-bit MS-DOS program, it will only work on MS-DOS, FreeDos and windows 98.                     
Remember to compile in the LARGE memory model!                        

Minimum requirements:
- CPU 8088 4.77 Mhz
- RAM 512 Kb.
- GRAPHICS MCGA/VGA

Recommended:
- CPU 8086 8 Mhz
- RAM 640 Kb
- GRAPHICS MCGA/VGA
- SOUND Adlib or compatible
- GRAVIS ULTRASOUND (To play Amiga Protracker MOD files)

*If you have a 286+ CPU, the engine can in theory draw very big sprites, but this is not tested.

Little Engine functions:
- Load and draw sprites (8x8, 16x16 and 32x32),with transparency and animations.
- Load, draw and scroll big TMX maps, created with tiled: https://www.mapeditor.org/
- Load and draw 256 colours bmp (overwrites the tileset).
- Hardware scroll (70 fps).
- Palette animations.
- Load and play IMF music for Adlib or compatible. 
- Load and play MOD music if we have a Gravis Ultrasound card (or compatible?). 

The "BAD" things:
- An 8088 will run the engine, but it will have slowdowns and flickering.
- A bit low resolution (304x176) it's needed to hide the borders being updated.	

Please feel free to copy this source code.
	
NOTES
-----

- Use only indexed bmp files with 256 colours.
- Little Engine uses 16x16 pixels tiles, and will split the tilesets in chunks of 16x16 pixels (from left to right / top to bottom).
- Sprites are split in chunks of 8x8,16x16 or 32x32. 
- TMX maps must be in XML format and orthogonal.
- All files (tilesets, maps, music and sprites) are limited to 64 kb.
- Only one song, one map and one tileset can be loaded in RAM, (the engine will allocate 256kb of ram for this purpose).
	Doing this, a lot of malloc trouble is fixed, the code is simplified and the engine works like a console, so every time you loaded
one map, tileset or music, the last one is overwritten.
- Loading a bmp will overwrite the tileset data.
- Little engine will store the Amiga Protracker patern data at the allocated space for adlib music (Max 64Kb).
	MOD Sample data will be loaded to GUS RAM (ussually 256Kb)
	
	
"Little Game Engine" para MCGA/VGA y 8086/286
---------------------------------------------

Un montón de código copiado de David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

Este es un programa de 16-bit, funcionará en MS-DOS, FreeDos y Windows 98.                     
Recuerda que hay que compilarlo usando el "LARGE memory model".                        

Requisitos mínimos:
- CPU 8088 4.77 Mhz
- RAM 512 Kb
- GRÁFICOS MCGA/VGA

Recomendado:
- CPU 8086 8 Mhz
- RAM 640 Kb
- GRÁFICOS MCGA/VGA
- SOUNIDO Adlib o compatible
- GRAVIS ULTRASOUND (para reproducir módulos Amiga Protracker - MOD)

*Si tienes un 286+, en teoría la cpu sería capaz de dibujar sprites muy grandes, pero no lo he probado.

Funciones de Little Engine:
- Cargar sprites de 8x8, 16x16 y 32x32, con transparencia y con animaciones.
- Cargar mapas TMX, creados con el programa tiled: https://www.mapeditor.org/
- Cargar archivos bmp de 256 colores (sobreescribe los conjuntos de patroneso tiles del mapa).
- Scroll por hardware de los mapas a 70 fps.
- Animaciones de paleta, puedes cambiar 8 primeros colores de la paleta para hacer animaciones.
- Cargar y reproducir música en formato IMF para el chip ym3812, OPL2, o Adlib, o cualquier Sound Blaster conpatible. 
- Cargar y reproducir música en formato MOD si tenemos una tarjeta Gravis Ultrasound (o conpatible?). 

Las cosas "MALAS":
- Un 8088 con VGA cargará el motor, pero tendrá ralentizaciones y parpadeos.
- Por lo que se, para usar una tarjeta Gravis Ultrasound necesitas como mínimo un 8086.
- Baja resolución de 304x176, necesaria para ocultar los bordes que son actualizados para el scroll.	

Este código es complétamente libre, cópialo o haz lo que quieras con el.
	

NOTAS
-----
- Usa solamente archivos bmp indexados con 256 colores.
- Little Engine usa tiles de 16x16, y corta los conjuntos de tiles en trozos de 16x16 (de izquierda a derecha y de arriba a abajo).
- Los sprites serán divididos en trozos de 8x8,16x16 or 32x32. 
- Los mapas TMX deben estar en formato XML y ortogonal.
- Todos los archivos (conjuntos de tiles, mapas, música y sprites) están limitados a 64 kb.
- Solamente una música, un mapa y un conjunto de tiles, pueden ser cargados RAM, (el motor reserva 256kb de RAM para eso).
Haciendo esto, se solucionan bugs de malloc, el código se simplifica, y el motor funciona como una consola, es decir, cada vez que cargues un mapa, conjunto de tiles, o una música, la anterior será sobreescrita.
	