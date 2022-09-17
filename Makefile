CFLAGS := -std=c11 -g
INCLUDE :=
LIBS := -lraylib -lSDL2 -lm
SRCS := main.c chip8.c audio.c

all: sheep8

sheep8: $(SRCS)
	cc $(SRCS) -o $@ $(CFLAGS) $(LIBS) $(INCLUDE)

web: $(SRCS)
	emcc -o wasm/sheep8.html $(SRCS) -Os -Wall ./raylib/libraylib.a -I. \
	-Iraylib -L. -Lraylib -s USE_GLFW=3 --shell-file wasm/shell.html -DPLATFORM_WEB \
	-sEXPORTED_RUNTIME_METHODS=[ccall] \
	--preload-file roms/BRIX \
	--preload-file roms/TETRIS \
	--preload-file roms/octopaint.ch8

raylib-web:
	cd raylib && emcc -c rcore.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	cd raylib && emcc -c rshapes.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	cd raylib && emcc -c rtextures.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	cd raylib && emcc -c rtext.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	cd raylib && emcc -c rmodels.c -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
	cd raylib && emcc -c utils.c -Os -Wall -DPLATFORM_WEB
	cd raylib && emcc -c raudio.c -Os -Wall -DPLATFORM_WEB

	cd raylib && emar rcs libraylib.a rcore.o rshapes.o rtextures.o rtext.o rmodels.o utils.o raudio.o
	cd ..

