# sheep8

# Chip8 emulator in C

- Run on [the web](https://b.papangkorn.com/wasm/sheep8.html)
- Use raylib with raygui intermediate mode ui
- C99

### Development

on linux:

```sh
make
./sheep8
```

on everywhere else:

```sh
write the build system config yourself im lazy
```

### Reference/Resource:

https://tobiasvl.github.io/blog/write-a-chip-8-emulator/

https://chip8.danirod.es/docs/current/chip8.html

http://nicktasios.nl/posts/making-sounds-using-sdl-and-visualizing-them-on-a-simulated-oscilloscope.html

https://github.com/Timendus/chip8-test-suite

### Todo:

- [ ] refactor
- [x] wasm
- [x] better way for user to specify roms filepath than argv[1]
- [x] sound
- [x] keyboard
- [x] buttons
