# Little-Game-Engine-for-VGA
Little Game Engine optimized for VGA/EGA and very slow computers (8088/8086/286)
----------------------------------------------------------------------------

<img src="play_011.png">

This is a very simple engine to create MS-DOS games for slow PCs. To run fast on the first PC's (8088/86 and 286), it uses VGA/EGA hardware capabilities, and gets rid of some slow techniques that were used at the time, such as redrawing the entire screen (due to lack of documentation about the VGA card).

The engine is now using the more common VGA 320x200 mode, but it has been tweaked to run at 60 Hz.
Also EGA 320x200@60Hz was added, and due to the complexity of EGA hardware, sprites are only 1 bit color (black and white).

The predefined keys are: 
- Move player/menus = UP DOWN LEFT RIGHT arrows.
- Jump in platform levels = D
- choose option in menu = ENTER
- Exit stage/game = ESC. 

So. why does it run so fast?
- Uses Hardware scrolling
- With VRAM to VRAM transfers (4 pixels at a time on VGA, and 8 pixels on EGA), you can update things on screen much faster.
- Updates only one column of tiles, and the sprites.
- Very simple "physics".

A lot of code from David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

This is a 16-bit MS-DOS program, it will only work on MS-DOS, FreeDos and windows 98.                     
Remember to compile in the LARGE memory model!                        

I tested the requirements on PCem, and 86Box emulator (they simulate very accurately 8088 CPU's).
Also tested on a 286 at 6 Mhz and 8088/NEC V20 from 4.77 to 10 MHz.

Requirements:
- CPU: 8088/NEC V20 4.77 Mhz (7.16Mhz recommended).
- RAM: about 380 Kb free.
- GRAPHICS: EGA (128Kb), VGA.
- SOUND: PC Speaker (Adlib recommended).

From 286+ CPU, the engine just works ok, even on modern cpu's (2018) using freedos.

Little Engine functions:
- 320x200 60 FPS MODE, compatible with all cards
- Load and draw sprites 8x8, 16x16, 32x32, 64x64 sprites (64x64 sprites don't support transparencies).
- Load, draw and scroll big TMX maps (map sizes are fixed 256x19 tiles), created with tiled: https://www.mapeditor.org/
- Load and draw 256 colours bmp (only 16 colours used on EGA).
- Graphic print function (8x8 Fonts, 64 characters). Includes latin characters by destroying these: "#$%&\()+-<=>
- Hardware scroll (60 fps).
- Palette animation 64+8 colors on VGA, 4 colors on EGA (animations for water and similar things).
- Fake Parallax on VGA (64 colors palette animations)
- Load and play IMF music for Adlib OPL2 or compatible. 


Please copy this source code, change it, do whatever you want. That's the point of github. Isn't it?

Thanks to everyone who helped and released code (music player, image loader).
Thanks to "jojo" for sending me a real 286 :).

SOUND
-----

To use the adlib music player, just enable adlib or soundblaster in the emulator, fpga... or connect an Adlib/SB card to your retro PC.
Adlib IMF files must be 60/70 tics/second to work well on slow CPU's.

To use your own tunes, I recommend using Reality Adlib Tracker or Adlib Tracker II, and them record a .dro file using dosbox. Now you only have to convert this .DRO to .IMF with dro2imf (60 ticks/second)


Install, Make and Run
---------------
* Install the required compiler in drive C:
	```
	Borland Turbo C++ 3.0
	Borland Turbo Assembler 2.0 
	```
* Assuming you run DOSBOX, install TC++ and TASM on drive C: 
	```
	set PATH=Z:\;C:\TC\BIN;C:\TASM
	```
* Go to project dir then `make`
* If nothing goes wrong, working `play.exe` should exist

NOTES
-----
- 	Read source to see how the functions are used in the samples, I tried to leave it as simple as possible.
- 	Use only indexed bmp files with 256 colours (even for EGA, but just use the first 16 and set them to the default EGA palette),
        save them in compatibility mode (Do not write color space information).
- 	Little Engine uses 16x16 pixels tiles, and it will split the tilesets in chunks of 16x16 pixels (from left to right / top to bottom).
- 	Sprites are split in chunks of 8x8, 16x16, 32x32, 64x64.
- 	EGA sprites are 16 colour bmp (4 bits per pixel) but they only use 3 colors: 0 = transparent, 1 = black; 2 = white.
- 	Font is a 16 colour bmp (4 bits per pixel) but it only uses 4 colors.
- 	TMX maps must be in CSV format 256x19, orthogonal and must have two layers (background and collision).
- 	All files (tilesets, maps, music and sprites) are limited to 64 kb. 
- 	Only one song, one map and one tileset can be loaded at the same time.
	Doing this, a lot of malloc trouble is fixed, the code is simplified and the engine works like a console.
	So every time you load a map, tileset or music, the last one is overwritten.
-	If a bmp is loaded as an image, map data will be erased.	

"Little Game Engine" optimizado para VGA/RGA y ordenadores/computadoras muy muy lentos (8088/8086/286)
--------------------------------------------------------------------------------------------------

Este es un motor muy simple para crear juegos de MS-DOS que funcionan en PC's muy lentos. Para que funcionase bien en los primeros PC (8088/86 y 286), el motor utiliza capacidades hardware de VGA y EGA, y evita técnicas muy lentas utilizadas en su época (por falta de información sobre el funcionamiento de la tarjetas).

Ahora usa el modo 320x200, modificado para funcionar a 60Hz en VGA. Además he añadido el modo EGA 320x200@60, pero los sprites en este modo, son en blanco y negro, debido a la complejidad del hardware EGA. 

Las teclas predefinidas son: 
- Mover jugador/menus = teclas direccionales.
- Saltar en el nivel plataformas = D
- Elegir opcion en menús = ENTER
- Salir de nivel/juego = ESC. 

Pero... ¿Por qué va tan rápido en PC's tan lentos?.
- Utiliza la función "scrolling", o movimiento del fondo, incluída en el hardware de las tarjetas VGA y EGA.
- Gracias a la estructura de la Memoria de video (VRAM) del modo X de VGA y de EGA, puedes hacer transferencias de datos de 32 bits, de VRAM a VRAM.
- Sólamente se actualiza una columna de tiles de 16x16 pixels, y los sprites (actualizar toda la pantalla es dolorosamente lento para estos primeros PC). 
- Motor físico súper simple.

Un montón de código copiado de David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

Este es un programa de 16-bit, funcionará en MS-DOS, FreeDos y Windows 98.                     
Recuerda que hay que compilarlo usando el "LARGE memory model".                        

He probado los requisitos en PCem y 86Box unos emuladores muy precisos. También lo he probado en un 286 real, y algunas personas me han informado de que el motor funciona en otras máquinas reales: 8088/NEC v20 a 9 Mhz; Varias tarjetas VGA y varias Adlib/Sound Blaster parecen funcionar.

Requisitos rmínimos:
- CPU: 8088/NEC V20 4.77 Mhz (Recomendado 8088 a 7.16MHz).
- RAM: unos 380 Kb libres.
- GRÁFICOS: EGA(128Kb),VGA.
- SONIDO: PC Speaker (Recomendado Adlib).

A partir de un modelo 286, funciona en cualquier cpu moderna x86 o x64 (2022) con cualquier tarjeta gráfica.

Funciones de Little Engine:
- Modo 320x200 60 FPS, compatible con todas las tarjetas.
- Cargar sprites de 8x8, 16x16, 32x32 y 64x64. Los sprites de 64x64 no soportan transparencia.
- Cargar mapas TMX creados con el programa tiled: https://www.mapeditor.org/
- Cargar archivos bmp de 256 colores, (los graficos para EGA, solo usan los primeros 16 colores).
- Funcuion print grafica, con fuentes de 8x8, 64 caracteres y caracteres latinos añadidos sobre los siguientes: "#$%&\()+-<=>
- Scroll por hardware de los mapas a 60 fps.
- Animaciones de paleta, (64 + 8 colores en VGA; 4 colores en EGA) puedes cambiar colores de la paleta para hacer animaciones.
- Falso segundo plano de "parallax" en VGA, con las animaciones de paleta de 64 colores, puedes simular un falso plano detras del principal.
- Cargar y reproducir música en formato IMF para el chip ym3812, OPL2, o Adlib, o cualquier Sound Blaster conpatible. 


Este código es complétamente libre,. Por favor, cópialo, modifícalo, haz lo que quieras con él, para eso lo he puesto en github, no?.

Gracias a todos los que ayudaron o cedieron código (reproductor de música, cargador de imágenes).
Gracias a jojo, por enviarme un 286 real :).
	
SONIDO
------

Para utilizar el reproductor de música IMF, símplemente activa la emulación de Adlib o Sound blaster en un emulador.
Si tienes un retro PC, conecta una tarjeta Adlib o Sound Blaster o compatible, y listo.

Para utilizar tus propias melodías, recomiendo crear cualquier música con Reality Adlib Tracker o Adlib Tracker II, y luego capturar un archivo .DRO con dosbox. Después solamente debes convertir el archivo .DRO a .IMF (con dro2imf ) a 50 ticks/segundo.


Instalación, compilar y ejecutar
---------------
* Instala el compilador en el disco C:
	```
	Borland Turbo C++ 3.0
	Borland Turbo Assembler 2.0 
	```
* Suponiendo que utilizas DOSBOX, instala TC++ y TASM en el disco C: 
	```
	set PATH=Z:\;C:\TC\BIN;C:\TASM
	```
* Ve al directorio del proyecto y escribe `make`
* Si todo va bien, un archivo llamado `play.exe` aparecerá en el directorio.

NOTAS
-----
- 	Mira el código para ver como se usan las funciones, he intentado que no sea un desastre.
- 	Usa solamente archivos bmp indexados con 256 colores, y guárdalos en modo compatible (No escribir los datos del espacio de color).
- 	Little Engine usa tiles de 16x16, y corta los conjuntos de tiles en trozos de 16x16 (de izquierda a derecha y de arriba a abajo).
- 	Los sprites serán divididos en trozos de 8x8, 16x16, 32x32 o 64x64. 
- 	Los sprites para EGA son bmp de 16 colores (4 bits por pixel) y usan los 3 primeros colores: 0 = transparente, 1 = negr0; 2 = blanco.
- 	Las fuentes son bmp de 16 colores tanto para EGA como para VGA, y usan solo los 4 primeros.
- 	Los mapas TMX deben estar en formato CSV, ortogonal, tener un tamaño de 256x19 tiles y tener dos capas (fondo y colisión).
- 	Todos los archivos (conjuntos de tiles, mapas, música y sprites) están limitados a 64 kb.
- 	Solamente una música, un mapa y un conjunto de tiles, pueden ser cargados RAM.
	Haciendo esto, se solucionan bugs de malloc, el código se simplifica, y el motor funciona como una consola.
	Es decir, cada vez que cargues un mapa, conjunto de tiles, o una música, la anterior será sobreescrita.
- 	Cargar una imagen bmp sobreescribirá los datos del mapa.
