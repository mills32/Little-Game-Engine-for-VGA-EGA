# Little-Game-Engine-for-VGA
Little Game Engine for VGA and 8088/8086/286
--------------------------------------------

This is a very simple "engine" to create MS-DOS games for slow PCs. To run fast on the first PC's (8088/86 and 286), it uses VGA hardware capabilities, and gets rid of some slow techniques that were used at the time, (due to lack of documentation about the VGA card).

I keep updating this with any changes I made, so this is not any final release, it is a bunch of testing menus, stages etc, so keep in mind it will have bugs.
The predefined keys are: 
- Move player/menus = UP DOWN LEFT RIGHT arrows.
- load test in menu/Jump in platform stage = D
- choose option in menu = ENTER
- Exit stage/game = ESC. 

So. why does it run so fast?
- Uses Hardware scrolling
- With VRAM to VRAM transfers (32 bits), you can update things on screen much faster.
- Update only a row and/or one column of 16x16 tiles, and the sprites (Updating the whole screen is too slow). This way we don't need the page flipping technique, but it causes sprites to flicker or even disappear if they are near the top part of the screen.
- Very simple "physics": no inertia, no nothing... This might be bad for some people, but the controls feel very responsive for me.

A lot of code from David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

This is a 16-bit MS-DOS program, it will only work on MS-DOS, FreeDos and windows 98.                     
Remember to compile in the LARGE memory model!                        

Little engine works on every toaster, potato or emulator, as long as it finds an 8088 (or any x86 based CPU) and a VGA compatible video card.
The engine has been ported to VGA "MODE-X", so it runs faster now, because most of drawing operations are 32 bit VRAM to VRAM transfers.

I tested the requirements on PCem, an x86 emulator that simulates very accurately several CPU'S.
Also I tested a 286 at 6 Mhz, and people informed this works on other real machines: 8088, NEC V20; Several VGA and Adlib/Sound Blaster compatible cards seem to work, but sometimes the scroll will flicker a bit depending on the VGA card, that's why I include two scrolling functions.

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

Please copy this source code, change it, do whatever you want. That's the point of github. Isn't it?

Thanks to everyone who helped and released code (music player, image loader).
Thanks to "jojo" for sending me a real 286 :).

SOUND
-----

To use the adlib music player, just enable adlib or soundblaster in the emulator, or connect an Adlib/SB card to your retro PC.
Adlib IMF files must be 60/70 tics/second to work well on slow CPU's like the 8088.

To use your own tunes, I recommend using Reality Adlib Tracker or Adlib Tracker II, and them record a .dro file using dosbox. Now you only have to convert this .DRO to .IMF with dro2imf (50 ticks/second)

I added sample playback using Sound Blaster, it is not possible to play more than one sound at the same time (PCM 8 bits). The first Sound Blaster cards have only one channel.

Warning!: samples are stored in "LT_tile_tempdata" and they will be overwritten if a map or a sprite is loaded, so load samples after loading maps/sprites.

PCM Drums
---------
If you activate "PCM Drums" (Music_PCMDrums = 1), Sound Blaster will play audio samples sincronized with music. This way you can add PCM drums to the adlib music so that it sounds even better :).

This function uses "0xB0, 0xB1 and 0xB2" instructions from the IMF file, to play samples number "0, 1 or 2". If one of these values have been read by the music player, a "set frequency" instruction has been sent to the OPL chip channel ("0, 1 or 2"), and the music player will play the first, second or the third sample.

In other words, when you create an adlib tune, you must remember channels 0, 1 and 2, are going to activate sample playback, so you must use quiet instruments in these channels (or even use a muted instrument). You must also add plain notes, (with no arpeggio or note changing effects)
Notes pasted on channel 0 will activate playback of sample 0, and so on... 

If you activated "PCM Drums", you must load the samples used for the music in the first three positions:
```
LT_Clear_Samples();
sb_load_sample("drum.wav");
sb_load_sample("snare.wav");
sb_load_sample("other.wav");
```
Then you can load more samples for sfx, but remember you can only play one sound, so they will interrupt each other.

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

Este es un motor muy simple para crear juegos de MS-DOS que funcionan en PC's muy lentos. Para que funcionase bien en los primeros PC (8088/86 y 286), el motor utiliza capacidades hardware de VGA, y evita técnicas muy lentas utilizadas en su epoca (por falta de información sobre el funcionamiento de la tarjeta VGA).

Sigo actualizando cualquier cambio y subiendolo aquí, por lo que esta no es ni mucho menos una versión final, sino un montón de menús y pruebas de niveles a medias de terminar (por lo que habrá muchos bugs).

Las teclas predefinidas son: 
- Mover jugador/menus = teclas direccionales.
- cargar test desde el menu/saltar en el nivel plataformas = D
- Elegir opcion en menús = ENTER
- Salir de nivel/juego = ESC. 

Pero... ¿Por qué va tan rápido en PC's tan lentos?.
- Utiliza la función "scrolling", o movimiento del fondo, incluída en el hardware de la tarjeta VGA.
- Gracias a la estructura de la Memoria de video (VRAM) del modo X de VGA, puedes hacer transferencias de datos de 32 bits, de VRAM a VRAM.
- Sólamente se actualiza una columna y/o una fila de tiles de 16x16 pixels, y los sprites (actualizar toda la pantalla es dolorosamente lento para estos primeros PC). Esto es muy rápido y hace que no necesitemos "page flipping", pero puede causar que los sprites parpadeen, si se acercan a la parte superior de la pantalla. 
- Motor físico súper simple, sin inercia, sin nada... Esto puede resultar incómodo para mucha gente, pero en mi opinión, se controla muy bien.

Un montón de código copiado de David Brackeen                                   
http://www.brackeen.com/home/vga/                                     

Este es un programa de 16-bit, funcionará en MS-DOS, FreeDos y Windows 98.                     
Recuerda que hay que compilarlo usando el "LARGE memory model".                        

Little Engine funciona en cualquier patata, tostadora o emulador, mientras encuente un procesador 8088 (o cualquiera basado en x86), y una tarjeta compatible con VGA.
El motor ha sido portado al "MODO-X" de VGA, y ahora funciona más rápido, ya que las funciones de dibujado se hacen por transferencia de 32 bits de VRAM a VRAM.

He probado los requisitos en PCem, un emulador muy preciso. También lo he probado en un 286 real, y algunas personas me han informado de que el motor funciona en otras máquinas reales: 8088/NEC v20 a 9 Mhz; Varias tarjetas VGA y varias Adlib/Sound Blaster parecen funcionar (aunque el scroll puede dar pequeños saltos dependiendo de la tarjeta VGA, por eso he incluido dos funciones de scroll).

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

Este código es complétamente libre,. Por favor, cópialo, modifícalo, haz lo que quieras con él, para eso lo he puesto en github, no?.

Gracias a todos los que ayudaron o cedieron código (reproductor de música, cargador de imágenes).
Gracias a jojo, por enviarme un 286 real :).
	
SONIDO
------

Para utilizar el reproductor de música IMF, símplemente activa la emulación de Adlib o Sound blaster en un emulador.
Si tienes un retro PC, conecta una tarjeta Adlib o Sound Blaster o compatible, y listo.

Para utilizar tus propias melodías, recomiendo crear cualquier música con Reality Adlib Tracker o Adlib Tracker II, y luego capturar un archivo .DRO con dosbox. Después solamente debes convertir el archivo .DRO a .IMF (con dro2imf ) a 50 ticks/segundo.

He añadido la reproducción de samples utilizando Sound Blaster, y no es posible reproducir más de un sonido al mismo tiempo (PCM 8 bits). Las primeras Sound Blaster solo cuentan con un canal.

Atención!: los samples de audio son almacenados en "LT_tile_tempdata" y seran sobreescritos si se carga un mapa o un sprite, así que carga los samples despues de haber cargado tus mapas/sprites.

PCM Drums
---------

Si activas la función "PCM Drums" (Music_PCMDrums = 1), la Sound Blaster reproducirá samples de audio sincronizados con la música. De esta forma puedes añadir sonidos de percusión a la musica adlib para que suene aun mejor :).

Ésta función utiliza las instrucciones "0xB0, 0xB1 o 0xB2" del archivo IMF, para reproducir los samples "0, 1 o 2". Es decir, cuando aparece alguna de ellas en el IMF, significa que se ha enviado la instrucción "seleccionar frecuencia" para los canales 0, 1 o 2 del chip OPL, y el reproductor de música enviará la orden de reproducir el sample 0, 1 o 2.

Explicado con un ejemplo, primero debes crear tu melodía adlib, pero teniendo en cuenta que los canales 0, 1 y 2 van a activar la reproducción de samples, por lo que en estos canales debes utilizar un instrumento adlib que a penas suene (o diréctamente uno que no produzca sonido). Además no debes añadir efectos de ningún tipo al instrumento (ni cambios de frecuencia ni arpegios ni nada), simplemente pega una nota donde quieres que suene el sample.
Las notas que estén en el canal 0, activaran la reproduccion del sample 0, las notas del canal 1 activan el sample 1, y las notas del canal 2 pues el sample 2.

Si as activado "PCM Drums", ten en cuenta que deberás cargar los samples que vas a usar en la música en las tres primeras posiciones:
```
LT_Clear_Samples();
sb_load_sample("drum.wav");
sb_load_sample("snare.wav");
sb_load_sample("other.wav");
```
Después puedes cargar más samples para efectos en el juego, pero ya sabes que se van a interrumpir unos a otros.



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
