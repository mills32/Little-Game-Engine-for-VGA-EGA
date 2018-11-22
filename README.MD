# Little-Game-Engine-for-VGA-MCGA
Little Game Engine for MCGA/VGA and 8086/286
--------------------------------------------

If yu just want to test it:
	Arrow keys = move player and menu.
	Enter = Select option.
	D = Jump.
	S = Hit ball.
	ESC = Back to menu / Exit to DOS.

A lot of code from David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

This is a 16-bit MS-DOS program, it will only work on MS-DOS, FreeDos and windows 98.                     
Remember to compile in the LARGE memory model!                        

Little engine works on every toaster, potato or emulator, as long as it finds an 8088 (or any x86 based CPU) and a VGA compatible video card.

Minimum requirements (scroll map + sprites 8x8 + Adlib):
- CPU 8088 4.77 Mhz
- RAM 512 Kb.
- GRAPHICS MCGA/VGA
- SOUND Adlib or compatible

Medium requirements (scroll + sprites 16x16 + Adlib / MOD without effects):
- CPU 8086 8 Mhz
- RAM 640 Kb
- GRAPHICS MCGA/VGA
- SOUND Adlib or compatible
- GRAVIS ULTRASOUND (To play Protracker MOD files)

Ultra requirements (sprites 32x32 or more + Adlib + MOD with all effects):
- CPU 286...

From 286+ CPU, the engine just works ok, even on modern cpu's (2018).

Little Engine functions:
- Load and draw sprites (8x8, 16x16 and 32x32),with transparency and animations.
- Load, draw and scroll big TMX maps, created with tiled: https://www.mapeditor.org/
- Load and draw 256 colours bmp (overwrites the tileset).
- Hardware scroll (70 fps).
- Palette animations.
- Load and play IMF music for Adlib or compatible. 
- Load and play MOD music if you have a Gravis Ultrasound card (or compatible?). 

The "BAD" things:
- An 8088 will run the engine, but it will have slowdowns and flickering.
- I think a 8088 won't be able to use a GUS card.
- A bit low resolution (304x176) it's needed to hide the borders being updated. It does not work on some monitors.	

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
- 	Sprites are split in chunks of 8x8,16x16 or 32x32. 
- 	TMX maps must be in CSV format, orthogonal and must have two layers (background and collision).
- 	All files (tilesets, maps, music and sprites) are limited to 64 kb. 
	Little engine will store the Amiga Protracker patern data at the allocated space for adlib music (Max 64Kb).
	MOD Sample data will be loaded to GUS RAM (ussually 256Kb).
- 	Only one song, one map and one tileset can be loaded in RAM, (the engine will allocate 256kb of ram for this purpose).
	Doing this, a lot of malloc trouble is fixed, the code is simplified and the engine works like a console.
	So every time you load a map, tileset or music, the last one is overwritten.
- 	Loading a bmp will overwrite the tileset data.
	
	
"Little Game Engine" para MCGA/VGA y 8086/286
---------------------------------------------

So solamente quieres probarlo:
	Arrow keys = mover jugador y menu.
	Enter = Seleccionar opción.
	D = Saltar.
	S = Lanzar bola.
	ESC = Volver al menu / Salir a DOS.

Un montón de código copiado de David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

Este es un programa de 16-bit, funcionará en MS-DOS, FreeDos y Windows 98.                     
Recuerda que hay que compilarlo usando el "LARGE memory model".                        

Little Engine funciona en cualquier patata, tostadora o emulador, mientras encuente un procesador 8088 (o cualquiera basado en x86), y una tarjeta compatible con VGA.

Requisitos mínimos (mover mapa + sprites 8x8 + Adlib): 
- CPU 8088 4.77 Mhz
- RAM 512 Kb
- GRÁFICOS MCGA/VGA
- SONIDO Adlib o compatible

Requisitos medios (mover mapa + sprites 16x16 + Adlib / música MOD sin efectos):
- CPU 8086 8 Mhz
- RAM 640 Kb
- GRÁFICOS MCGA/VGA
- SONIDO Adlib o compatible
- GRAVIS ULTRASOUND (para reproducir módulos Protracker - MOD)

Requisitos Ultra (sprites de 32x32 o más + Adlib / MOD con efectos):
- CPU 286

A partir de un modelo 286, funciona en cualquier cpu moderna x86 o x64 (2018) con cualquier tarjeta gráfica.
 

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
- Baja resolución de 304x176, necesaria para ocultar los bordes que son actualizados para el scroll. No funciona en algunos monitores.	

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
- 	Los sprites serán divididos en trozos de 8x8,16x16 or 32x32. 
- 	Los mapas TMX deben estar en formato CSV, ortogonal, y tener dos capas (fondo y colisión).
- 	Todos los archivos (conjuntos de tiles, mapas, música y sprites) están limitados a 64 kb.
	Little engine carga los datos de secuancia de los archivos Protracker, en la memoria reservada para la música adlib (Max 64Kb).
	Los datos de los samples se cargarán en la RAM de la tarjeta GUS (generalmente 256Kb).
- 	Solamente una música, un mapa y un conjunto de tiles, pueden ser cargados RAM, (el motor reserva 256kb de RAM para eso).
	Haciendo esto, se solucionan bugs de malloc, el código se simplifica, y el motor funciona como una consola.
	Es decir, cada vez que cargues un mapa, conjunto de tiles, o una música, la anterior será sobreescrita.
- 	Cargar un bmp sobreescribirá los datos de los tiles.
