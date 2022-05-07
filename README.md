# Chip8 emulator in C

### Development

on linux:

```sh
mkdir build
cd build
cmake ..
make
../bin/sheep8
```

on mingw:
```sh
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
make
../bin/sheep8
```

### Reference/Resource:

https://tobiasvl.github.io/blog/write-a-chip-8-emulator/

https://chip8.danirod.es/docs/current/chip8.html

https://wiki.libsdl.org

http://nicktasios.nl/posts/making-sounds-using-sdl-and-visualizing-them-on-a-simulated-oscilloscope.html


### Todo:

- [ ] refactor
- [x] better way for user to specify roms filepath than argv[1]
- [x] sound
- [x] keyboard
- [x] buttons
