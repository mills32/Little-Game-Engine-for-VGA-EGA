# Little-Game-Engine-for-VGA
Little Game Engine for VGA and 8088/8086/286
--------------------------------------------

To play: 
-You need adlib and/or GUS, 8088 and VGA. 
-Controls: UP DOWN LEFT RIGHT S D ENTER ESC. 

A lot of code from David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

This is a 16-bit MS-DOS program, it will only work on MS-DOS, FreeDos and windows 98.                     
Remember to compile in the LARGE memory model!                        

Little engine works on every toaster, potato or emulator, as long as it finds an 8088 (or any x86 based CPU) and a VGA compatible video card.
The engine has been ported to VGA "MODE-X", so it runs faster now, because most of drawing operations are VRAM to VRAM transfers (4 pixels at a time).

Requirements are tested on PCem, an x86 emulator that simulates very accurately several CPU'S.

Minimum requirements: smooth scroll maps, adlib and several 8x8 or 16x16 sprites with a lot of flickering.
- CPU 8088 4.77 Mhz
- RAM 512 Kb.
- GRAPHICS VGA
- SOUND Adlib or compatible

Medium requirements: smooth scroll maps, adlib or MOD (without effects) and sprites up to 32x32 with little flickering.
- CPU 8086 8 Mhz
- RAM 640 Kb
- GRAPHICS VGA
- SOUND Adlib or compatible
- GRAVIS ULTRASOUND (To play Protracker MOD files)

Ultra requirements: can draw several 32x32 sprites, one 64x64 sprite, play MOD with effects, no flickering.
- CPU 286 or better

From 286+ CPU, the engine just works ok, even on modern cpu's (2018).

Little Engine functions:
- 320x240 60 FPS MODE, compatible with all cards
- Load and draw sprites (8x8, 16x16, 32x32, 64x64),with transparency and animations.
- Load, draw and scroll big TMX maps, created with tiled: https://www.mapeditor.org/
- Load and draw 256 colours bmp.
- Hardware scroll (60 fps).
- Palette animations.
- Load and play IMF music for Adlib or compatible. 
- Load and play MOD music if you have a Gravis Ultrasound card (or compatible?). 

The "BAD" things:
- An 8088 will run the engine, but it may have slowdowns and flickering.
- I think a 8088 won't be able to use a GUS card.	

Please feel free to copy this source code.

Thanks to everyone who helped and / or released code (music player, image loader).

SOUND
-----

To use the adlib music player, just enable adlib or soundblaster in the emulator, or connect an Adlib/SB card to a retro PC.
Adlib IMF files must be 70 tics/second to work well on slow CPU's like the 8088.

To use MOD player, enable GUS emulation or connect a GUS to your retro PC, then run "setgus.bat" I think that's all, (no drivers needed)
The GUS can play MOD files with samples up to 44100Hz and also you can use more than 4 channels.
You can make the mod player to work better on slow CPU's by setting the mode to 1 "PlayMOD(1)". This will disable some effects.

NOTES
-----
- 	Read source to see how the functions are used in the samples, I tried to leave it as simple as possible.
- 	Use only indexed bmp files with 256 colours.
- 	Little Engine uses 16x16 pixels tiles, and will split the tilesets in chunks of 16x16 pixels (from left to right / top to bottom).
- 	Sprites are split in chunks of 8x8,16x16 or 32x32, 64x64. 
- 	TMX maps must be in CSV format, orthogonal and must have two layers (background and collision).
- 	All files (tilesets, maps, music and sprites) are limited to 64 kb. 
	Little engine will store the Amiga Protracker patern data at the allocated space for adlib music (Max 64Kb).
	MOD Sample data will be loaded to GUS RAM (ussually 256Kb).
- 	Only one song, one map and one tileset can be loaded at the same time.
	Doing this, a lot of malloc trouble is fixed, the code is simplified and the engine works like a console.
	So every time you load a map, tileset or music, the last one is overwritten.
	
	
"Little Game Engine" para VGA y 8088/8086/286
---------------------------------------------

Para jugar: 
-Necesitas adlib y/o GUS, 8088 y VGA. 
-Controles: Teclas ireccionales, S, D, ENTER, ESC. 

Un montón de código copiado de David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

Este es un programa de 16-bit, funcionará en MS-DOS, FreeDos y Windows 98.                     
Recuerda que hay que compilarlo usando el "LARGE memory model".                        

Little Engine funciona en cualquier patata, tostadora o emulador, mientras encuente un procesador 8088 (o cualquiera basado en x86), y una tarjeta compatible con VGA.
El motor ha sido portado al "MODO-X" de VGA, y ahora funciona má rápido, ya que las funciones de dibujado se hacen por transferencia de VRAM a VRAM (de 4 en 4 pixels).

Requisitos mínimos: scroll de mapas a 60 fps, adlib, y varios sprites de 8x8 o 16x16, con parpadeos.
- CPU 8088 4.77 Mhz
- RAM 512 Kb
- GRÁFICOS VGA
- SONIDO Adlib o compatible

Requisitos medios: scroll de mapas a 60 fps, adlib y MOD(sin efectos especiales), sprites de hasta 32x32, con pocos parpadeos.
- CPU 8086 8 Mhz
- RAM 640 Kb
- GRÁFICOS MCGA/VGA
- SONIDO Adlib o compatible
- GRAVIS ULTRASOUND (para reproducir módulos Protracker - MOD)

Requisitos Ultra: sprites de 32x32 o más y Adlib / MOD con efectos.
- CPU 286

A partir de un modelo 286, funciona en cualquier cpu moderna x86 o x64 (2018) con cualquier tarjeta gráfica.
 

Funciones de Little Engine:
- Modo 320x240 60 FPS, compatible con todas las tarjetas.
- Cargar sprites de 8x8, 16x16, 32x32 y 64x64, con transparencia y con animaciones.
- Cargar mapas TMX, creados con el programa tiled: https://www.mapeditor.org/
- Cargar archivos bmp de 256 colores.
- Scroll por hardware de los mapas a 60 fps.
- Animaciones de paleta, puedes cambiar 8 primeros colores de la paleta para hacer animaciones.
- Cargar y reproducir música en formato IMF para el chip ym3812, OPL2, o Adlib, o cualquier Sound Blaster conpatible. 
- Cargar y reproducir música en formato MOD si tenemos una tarjeta Gravis Ultrasound (o conpatible?). 

Las cosas "MALAS":
- Un 8088 con VGA cargará el motor, pero pude tener ralentizaciones y parpadeos.
- Por lo que se, para usar una tarjeta Gravis Ultrasound necesitas como mínimo un 8086.

Este código es complétamente libre, cópialo o haz lo que quieras con él.

Gracias a todos los que ayudaron o cedieron código (reproductor de música, cargador de imágenes).
	
SONIDO
------

Para utilizar el reproductor de música IMF símplemente activa la emulación de Adlib o Sound blaster en un emulador.
Si tienes un retro PC, conecta una tarjeta Adlib o Sound Blaster o compatible, y listo.
Los archivos IMF deben tener 70 ticks/segundo para funcionar bien en CPUs lentas como el 8088.

Para utilizar el reproductor MOD, activa la emulacion GUS en el emulador, o conecta una tarjeta GUS al retro PC (8086+).
Después ejecuta "setgus.bat" y listo, creo que no necesita más drivers.
La tarjeta GUS podrá reproducir archimos MOD de más de 4 canales, y utilizar samples de hasta 44100 Hz.

NOTAS
-----
- 	Mira el código para ver como se usan las funciones, he intentado que no sea un desastre.
- 	Usa solamente archivos bmp indexados con 256 colores.
- 	Little Engine usa tiles de 16x16, y corta los conjuntos de tiles en trozos de 16x16 (de izquierda a derecha y de arriba a abajo).
- 	Los sprites serán divididos en trozos de 8x8, 16x16, 32x32 o 64x64. 
- 	Los mapas TMX deben estar en formato CSV, ortogonal, y tener dos capas (fondo y colisión).
- 	Todos los archivos (conjuntos de tiles, mapas, música y sprites) están limitados a 64 kb.
	Little engine carga los datos de secuancia de los archivos Protracker, en la memoria reservada para la música adlib (Max 64Kb).
	Los datos de los samples se cargarán en la RAM de la tarjeta GUS (generalmente 256Kb).
- 	Solamente una música, un mapa y un conjunto de tiles, pueden ser cargados RAM.
	Haciendo esto, se solucionan bugs de malloc, el código se simplifica, y el motor funciona como una consola.
	Es decir, cada vez que cargues un mapa, conjunto de tiles, o una música, la anterior será sobreescrita.
- 	Cargar un bmp sobreescribirá los datos de los tiles.
