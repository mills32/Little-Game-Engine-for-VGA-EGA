# Little-Game-Engine-for-VGA
Little Game Engine for VGA and 8088/8086/286
--------------------------------------------

This is a very simple "engine" to create games on MS-DOS for slow PCs. To run fast on the first PC's (8088/86 and 286), it uses all hardware capabilities of VGA that I know of:
does not use page flopping like most games of the time, because that's too slow for the PCs 
To play: 
-You need adlib and/or Sound Blaster, 8088 and VGA. 
-Controls: UP DOWN LEFT RIGHT S D ENTER ESC. 

A lot of code from David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

This is a 16-bit MS-DOS program, it will only work on MS-DOS, FreeDos and windows 98.                     
Remember to compile in the LARGE memory model!                        

Little engine works on every toaster, potato or emulator, as long as it finds an 8088 (or any x86 based CPU) and a VGA compatible video card.
The engine has been ported to VGA "MODE-X", so it runs faster now, because most of drawing operations are 32 bit VRAM to VRAM transfers.

I tested the requirements on PCem, an x86 emulator that simulates very accurately several CPU'S.
Also I tested a 286 at 6 Mhz, and people informed this works on other real machines: 8088, NEC V20... Several VGA and Adlib/Sound Blaster compatible cards seem to work, but sometimes the scroll will flicker a bit depending on the VGA card, that's why I include two scrolling functions.

Minimum requirements: (sprites will flicker a lot)
- CPU 8088 4.77 Mhz
- RAM 512 Kb.
- GRAPHICS VGA

Recommended requirements:
- CPU 8088/NEC V20 9 Mhz; 8086 8 Mhz; 286 6 Mhz
- RAM 640 Kb
- GRAPHICS VGA
- SOUND Adlib, Sound Blaster or compatible.

Ultra requirements: for 64x64 sprites
- CPU 286 6 Mhz

From 286+ CPU, the engine just works ok, even on modern cpu's (2018).

Little Engine functions:
- 320x240 60 FPS MODE, compatible with all cards
- Load and draw sprites (8x8, 16x16, 32x32, 64x64),with transparency and animations.
- Load, draw and scroll big TMX maps, created with tiled: https://www.mapeditor.org/
- Load and draw 256 colours bmp.
- Graphic print function (8x8 Fonts, 64 characters). Includes latin characters by destroying these: "#$%&\()+-<=>
- Hardware scroll (60 fps).
- Palette animations.
- Load and play IMF music for Adlib OPL2 or compatible + PCM samples (8 bit unsigned PCM) using Sound Blaster. 

The "BAD" things:
- 8088's at 4.77 will run the engine with slowdowns and flickering.
- Some Sound cards are not fully compatible with the extremely simple music playback function.

Please feel free to copy this source code.

Thanks to everyone who helped and / or released code (music player, image loader).
Thanks to "jojo" for sending me a real 286 :).

SOUND
-----

To use the adlib music player, just enable adlib or soundblaster in the emulator, or connect an Adlib/SB card to your retro PC.
Adlib IMF files must be 60/70 tics/second to work well on slow CPU's like the 8088.


NOTES
-----
- 	Read source to see how the functions are used in the samples, I tried to leave it as simple as possible.
- 	Use only indexed bmp files with 256 colours, save them in compatibility mode()Do not write color space information).
- 	Little Engine uses 16x16 pixels tiles, and it will split the tilesets in chunks of 16x16 pixels (from left to right / top to bottom).
- 	Sprites are split in chunks of 8x8, 16x16, 32x32, 64x64. 
- 	TMX maps must be in CSV format, orthogonal and must have two layers (background and collision).
- 	All files (tilesets, maps, music and sprites) are limited to 64 kb. 
	Except for the bmp image loader, which can load a 320x480 image to VRAM.
- 	Only one song, one map and one tileset can be loaded at the same time.
	Doing this, a lot of malloc trouble is fixed, the code is simplified and the engine works like a console.
	So every time you load a map, tileset or music, the last one is overwritten.
-	If a bmp is loaded as an image, map data will be erased.	
	
"Little Game Engine" para VGA y 8088/8086/286
---------------------------------------------

Para jugar: 
-Necesitas adlib y/o Sound Blaster, un procesador 8088 y tarjeta gráfica VGA. 

Un montón de código copiado de David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

Este es un programa de 16-bit, funcionará en MS-DOS, FreeDos y Windows 98.                     
Recuerda que hay que compilarlo usando el "LARGE memory model".                        

Little Engine funciona en cualquier patata, tostadora o emulador, mientras encuente un procesador 8088 (o cualquiera basado en x86), y una tarjeta compatible con VGA.
El motor ha sido portado al "MODO-X" de VGA, y ahora funciona más rápido, ya que las funciones de dibujado se hacen por transferencia de 32 bits de VRAM a VRAM.

He probado los requisitos en PCem, un emulador muy preciso. También lo he probado en un 286 real, y algunas personas me han informado de que el motor funciona en otras máquinas reales: 8088/NEC v20 a 9 Mhz; Varias tarjetas VGA y varias Adlib/Sound Blaster parecen funcionar, aunque el scroll puede dar pequeños saltos dependiendo de la tarjeta VGA, por eso he incluido dos funciones de scroll..

Requisitos mínimos: (los sprites parpadean mucho).
- CPU 8088 4.77 Mhz
- RAM 512 Kb
- GRÁFICOS VGA

Requisitos recomendados:
- CPU 8088/NEC V20 9 Mhz; 8086 8 Mhz; 286 6 Mhz.
- RAM 640 Kb
- GRÁFICOS VGA
- SONIDO Adlib/Sound Blaster o compatible

Requisitos Ultra: para sprites de 64x64
- CPU 286 6 Mhz

A partir de un modelo 286, funciona en cualquier cpu moderna x86 o x64 (2018) con cualquier tarjeta gráfica.
 

Funciones de Little Engine:
- Modo 320x240 60 FPS, compatible con todas las tarjetas.
- Cargar sprites de 8x8, 16x16, 32x32 y 64x64, con transparencia y con animaciones.
- Cargar mapas TMX, creados con el programa tiled: https://www.mapeditor.org/
- Cargar archivos bmp de 256 colores.
- Funcuion print grafica, con fuentes de 8x8, 64 caracteres y caracteres latinos añadidos sobre los siguientes: "#$%&\()+-<=>
- Scroll por hardware de los mapas a 60 fps.
- Animaciones de paleta, puedes cambiar 8 primeros colores de la paleta para hacer animaciones.
- Cargar y reproducir música en formato IMF para el chip ym3812, OPL2, o Adlib, o cualquier Sound Blaster conpatible. 

Las cosas "MALAS":
- Un 8088 con VGA cargará el motor, pero pude tener ralentizaciones y parpadeos.
- Algunas Sound Blaster no reproducen la música del opl con normalidad, ya que no son compatibles con la función extremadamente simple que hay implementada.

Este código es complétamente libre, cópialo o haz lo que quieras con él.

Gracias a todos los que ayudaron o cedieron código (reproductor de música, cargador de imágenes).
Gracias a jojo, por enviarme un 286 real :).
	
SONIDO
------

Para utilizar el reproductor de música IMF símplemente activa la emulación de Adlib o Sound blaster en un emulador.
Si tienes un retro PC, conecta una tarjeta Adlib o Sound Blaster o compatible, y listo.
Los archivos IMF deben tener 70 o 70 ticks/segundo para funcionar bien en CPUs lentas como el 8088.

NOTAS
-----
- 	Mira el código para ver como se usan las funciones, he intentado que no sea un desastre.
- 	Usa solamente archivos bmp indexados con 256 colores, y guárdalos en modo compatible (No escribir los datos del espacio de color).
- 	Little Engine usa tiles de 16x16, y corta los conjuntos de tiles en trozos de 16x16 (de izquierda a derecha y de arriba a abajo).
- 	Los sprites serán divididos en trozos de 8x8, 16x16, 32x32 o 64x64. 
- 	Los mapas TMX deben estar en formato CSV, ortogonal, y tener dos capas (fondo y colisión).
- 	Todos los archivos (conjuntos de tiles, mapas, música y sprites) están limitados a 64 kb.
	Excepto las imagenes bmp que pueden tener tamaños de hasta 320x480, ya que son cargadas directamente a VRAM.
- 	Solamente una música, un mapa y un conjunto de tiles, pueden ser cargados RAM.
	Haciendo esto, se solucionan bugs de malloc, el código se simplifica, y el motor funciona como una consola.
	Es decir, cada vez que cargues un mapa, conjunto de tiles, o una música, la anterior será sobreescrita.
- 	Cargar una imagen bmp sobreescribirá los datos del mapa.
