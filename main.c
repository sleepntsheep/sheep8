#include "main.h"

int SPEED = 5;

/* chip8 */

int screen[HEIGHT][WIDTH]; /* chip8 screen, not SDL screen */
int paused = 0;
int idx = 0; /* INDEX */
int pc = 0x200; /* program counter */
uint8_t memory[MEMORY_SIZE]; /* memory for loading the ROM - 2kb */
uint8_t v[NUM_REGISTERS]; /* 16 8 bit register */
uint8_t delayTimer, soundTimer; /* sound timer make sound if more than 0 */
uint16_t stack[256]; /* array for stack */
uint8_t sp; /* stack pointer */

/* keyboard state */

int state[16];

/* SDL stuff */

Uint32 time_step_ms = 1000 / 60; /* fps */

int running = 1;
SDL_Window* window = NULL;
SDL_Renderer* renderer;
SDL_Event event;
SDL_Surface* surface;
SDL_Texture *texture;
SDL_Rect rect;
TTF_Font *font;
SDL_Color textColor = { 255, 255, 255 };

SDL_AudioSpec want, have;
uint64_t samples_played = 0;
SDL_AudioDeviceID audio_device_id;

/* array of last op osd */
uint16_t lastop[lastOPcount];

int main(int argc, char** argv) {
	if (argc == 1) {
		printf("You must supply rom file path to open");
		return -1;
	}
	if (argc > 2) {
		SPEED = strtol(argv[2], NULL, 10);
	}

	loadRom(argv[1]);

	if (init(argv[1]) != 0)
		return -1;

	Uint32 next_game_step = SDL_GetTicks();

	while (running) {
		Uint32 now = SDL_GetTicks();
		while (SDL_PollEvent( &event )) {
            handleEvent();
		}

		if (next_game_step <= now) {
			int i;
			for (i = 0; i < SPEED; i++) {
				if (!paused) {
					uint16_t opcode = (memory[pc] << 8 | memory[pc+1]);
					interpretOP(opcode);
				}
			}
			display();
			next_game_step += time_step_ms;
			if (!paused)
				updateTimers();
		}
		else {
			SDL_Delay(next_game_step - now);
		}
	}

	TTF_Quit();
	SDL_CloseAudio();
	SDL_Quit();

	return 0;
}

int init(char* title) {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initing SDL: %s\n", SDL_GetError());
		return -1;
	}

	if (!TTF_WasInit() && TTF_Init() == -1) {
		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
		return -1;
	}


	char buffer[100];
	sprintf(buffer, "Chip8 emulator: %s", title);

	window = SDL_CreateWindow(buffer, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * SCALE + 200, HEIGHT * SCALE, 0);

	if (!window) {
		fprintf(stderr, "failed creating window: %s", SDL_GetError());
		return -1;
	}

	surface = SDL_GetWindowSurface(window);

	if (!surface) {
		fprintf(stderr, "failed creating surface: %s", SDL_GetError());
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);

	if (!renderer) {
		fprintf(stderr, "failed creating renderer: %s", SDL_GetError());
		return -1;
	}

	font = TTF_OpenFont(FONT_PATH, 20);
	if (!font) {
		printf("TTF_OpenFont: %s\n", TTF_GetError());
	}

	SDL_memset(&want, 0, sizeof want);

	want.freq = 44100;
	want.format = AUDIO_F32;
	want.channels = 2;
	want.samples = 512;
	want.callback = audio_callback;
	want.userdata = (void*)&samples_played;

	audio_device_id = SDL_OpenAudioDevice(
		NULL, 0, &want, &have, SDL_AUDIO_ALLOW_ANY_CHANGE);

	if (!audio_device_id) {
		fprintf(stderr, "Error creating SDL audio device. SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_PauseAudioDevice(audio_device_id, 0);

	return 0;
}

void audio_callback(void* userdata, uint8_t* stream, int len) {
	uint64_t* samples_played = (uint64_t*) userdata;
	float* fstream = (float*)(stream);

	static const float volume = 0.2;
	static const float frequency = 441.0;

	int sid;
	for (sid = 0; sid < (len / 8); ++sid) {
		double time = (*samples_played + sid) / 44100.0;
		double x = 2.0 * 3.14159 * time * frequency;
		fstream[2 * sid + 0] = volume * sin(x);
		fstream[2 * sid + 1] = volume * sin(x);
	}

	*samples_played += len / 8;
}

int flipPixel(int x, int y) {
	if (y < 0 || y >= HEIGHT) return 0;
	if (x < 0 || x >= WIDTH) return 0;
	screen[y][x] = screen[y][x] ^ 1;
	return !screen[y][x];
}

void clearScr() {
	memset(screen, 0, sizeof(int) * WIDTH * HEIGHT);
}

void display() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	int i, j;
	for (i = 0; i < HEIGHT; i++) {
		for (j = 0; j < WIDTH; j++) {
			if (!screen[i][j])
				continue;
			rect.x = j * SCALE;
			rect.y = i * SCALE;
			rect.w = SCALE;
			rect.h = SCALE;
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillRect(renderer, &rect);
		}
	}
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderDrawLine(renderer, 0, HEIGHT * SCALE, WIDTH * SCALE, HEIGHT * SCALE);
	SDL_RenderDrawLine(renderer, WIDTH * SCALE, 0, WIDTH * SCALE, HEIGHT * SCALE);

	/* display 5 last op code */
	char buffer[16];
	drawText( "Operations", WIDTH * SCALE + 10, 10 )	;
	for (i = 0; i < lastOPcount; i++) {
		sprintf(buffer, "%x", lastop[i]);
		drawText( buffer, WIDTH * SCALE + 10, i * 20 + 40 );
	}

	drawText( "KeyStates", WIDTH * SCALE + 10, 250 );
	sprintf( buffer, "%d %d %d %d", state[1], state[2], state[3], state[0xC]);
	drawText( buffer, WIDTH * SCALE + 10, 280);
	sprintf( buffer, "%d %d %d %d", state[4], state[5], state[6], state[0xD]);
	drawText( buffer, WIDTH * SCALE + 10, 300);
	sprintf( buffer, "%d %d %d %d", state[7], state[8], state[9], state[0xE]);
	drawText( buffer, WIDTH * SCALE + 10, 320);
	sprintf( buffer, "%d %d %d %d", state[0xA], state[0], state[0xB], state[0xF]);
	drawText( buffer, WIDTH * SCALE + 10, 340);

	sprintf( buffer, "Index: %x", idx);
	drawText( buffer, WIDTH * SCALE + 10, 370 );
	sprintf( buffer, "PC: %x", pc );
	drawText( buffer, WIDTH * SCALE + 10, 390 );
	sprintf( buffer, "Delay timer: %x", delayTimer );
	drawText( buffer, WIDTH * SCALE + 10, 410 );
	sprintf( buffer, "Sound timer: %x", soundTimer );
	drawText( buffer, WIDTH * SCALE + 10, 430 );

	SDL_RenderPresent(renderer);
}

void handleEvent() {
    switch ( event.type ) {
        case SDL_QUIT:
            running = 0;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            Uint8 sym = event.key.keysym.sym;
            int keydown = (event.type == SDL_KEYDOWN);
            switch (sym) {
            	case SDLK_1:
            		state[0x1] = keydown;
            		break;
            	case SDLK_2:
            		state[0x2] = keydown;
            		break;
            	case SDLK_3:
            		state[0x3] = keydown;
            		break;
            	case SDLK_4:
            		state[0xC] = keydown;
            		break;
            	case SDLK_q:
            		state[0x4] = keydown;
            		break;
            	case SDLK_w:
            		state[0x5] = keydown;
            		break;
            	case SDLK_e:
            		state[0x6] = keydown;
            		break;
        		case SDLK_r:
        			state[0xD] = keydown;
        			break;
        		case SDLK_a:
        			state[0x7] = keydown;
        			break;
        		case SDLK_s:
        			state[0x8] = keydown;
        			break;
        		case SDLK_d:
        			state[0x9] = keydown;
        			break;
        		case SDLK_f:
        			state[0xE] = keydown;
        			break;
        		case SDLK_z:
        			state[0xA] = keydown;
        			break;
        		case SDLK_x:
        			state[0x0] = keydown;
        			break;
        		case SDLK_c:
        			state[0xB] = keydown;
        			break;
        		case SDLK_v:
        			state[0xF] = keydown;
        			break;
            }
            if (event.type == SDL_KEYDOWN) {
                if ( sym == SDLK_SPACE ) {
                    paused ^= 1;
                }
            }
            break;
        }
    }
}

void drawText(char * text, int x, int y) {
	surface = TTF_RenderText_Solid( font, text, textColor );
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	int textW, textH;
	SDL_QueryTexture(texture, NULL,  NULL, &textW, &textH);
	SDL_Rect dstrect;
	dstrect.x = x;
	dstrect.y = y;
	dstrect.w = textW;
	dstrect.h = textH;
	SDL_FreeSurface(surface);
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
}

void interpretOP(uint16_t op) {
	pc += 2;

	/* OP -> AxyB */
	unsigned char x = (op & 0x0F00) >> 8;
	unsigned char y = (op & 0x00F0) >> 4;

	switch (op & 0xF000) {
		case 0x0000:
			switch (op) {
				case 0x00E0:
					/* CLR */
					clearScr();
					break;
				case 0x00EE:
					/* RET */
					pc = stack[sp--];
					break;
			}
			break;
		case 0x1000:
		{
			/* JUMP TO nnn */
			pc = op & 0xFFF;
			break;
		}
		case 0x2000:
		{
			/* call subroutine at nnn */
			stack[++sp] = pc;
			pc = op & 0xFFF;
			break;
		}
		case 0x3000:
			/* SE Vx, byte */
			if (v[x] == (op & 0x00FF))
				pc += 2;
			break;
		case 0x4000:
			/* SNE Vx, byte */
			if (v[x] != (op & 0x00FF))
				pc += 2;
			break;
		case 0x5000:
			/* SE Vx, Vy */
			if (v[x] == v[y])
				pc += 2;
			break;
		case 0x6000:
			v[x] = op & 0x00FF;
			break;
		case 0x7000:
			v[x] += op & 0x00FF;
			break;
		case 0x8000:
			switch (op & 0xF) {
				case 0x0:
					v[x] = v[y];
					break;
				case 0x1:
					v[x] = v[x] | v[y];
					break;
				case 0x2:
					v[x] = v[x] & v[y];
					break;
				case 0x3:
					v[x] = v[x] ^ v[y];
					break;
				case 0x4:
					if (v[x] + v[y] > 0xFF)
						v[0xF] = 1;
					else
						v[0xF] = 0;
					v[x] += v[y];
					break;
				case 0x5:
					if (v[x] > v[y])
						v[0xF] = 1;
					else
						v[0xF] = 0;
					v[x] -= v[y];
					break;
				case 0x6:
					v[0xF] = v[x] & 0x1;
					v[x] >>= 1;
					break;
				case 0x7:
					if (v[y] > v[x])
						v[0xF] = 1;
					else
						v[0xF] = 0;
					v[x] = v[y] - v[x];
					break;
				case 0xE:
					v[0xF] = v[x] & 0x80;
					v[x] <<= 2;
					break;
			}
			break;
		case 0x9000:
			if (v[x] != v[y])
				pc += 2;
			break;
		case 0xA000:
			idx = op & 0x0FFF;
			break;
		case 0xB000:
			pc = v[0] + (op & 0x0FFF);
			break;
		case 0xC000:
			v[x] = (rand() % 256) & (op & 0x00FF);
			break;
		case 0xD000:
			{
				/* DRW, Vx, Vy, nibble */
				unsigned char width = 8;
				unsigned char height = op & 0xF;
				v[0xF] = 0;
				int row, col;
				for (row = 0; row < height; row++) {
					uint8_t sprite = memory[idx + row];

					for (col = 0; col < width; col++) {
						if ((sprite & 0x80) > 0) {
							if (flipPixel(v[x] + col, v[y] + row))  {
								v[0xF] = 1;
								/* if register 0xF is set to 1, collision happened */
							}
						}
						sprite <<= 1;
					}
				}
			}
			break;
		case 0xE000:
			switch (op & 0x00FF) {
				case 0x9E:
					if (state[v[x]])
						pc += 2;
					break;
				case 0xA1:
					if (!state[v[x]])
						pc += 2;
					break;
			}
			break;
		case 0xF000:
			switch (op & 0x00FF) {
				case 0x07:
					v[x] = delayTimer;
					break;
				case 0x0A:
					v[x] = SDL_WaitEvent(&event);
                    handleEvent();
					break;
				case 0x15:
					delayTimer = v[x];
					break;
				case 0x18:
					soundTimer = v[x];
					break;
				case 0x1E:
					idx += v[x];
					break;
				case 0x29:
					idx = v[x] * 5;
					break;
				case 0x33:
					memory[(int)idx] = (v[x] / 100) % 10;
					memory[(int)idx+1] = (v[x] / 10) % 10;
					memory[(int)idx+2] = (v[x] % 10);
					break;
				case 0x55: {
					int i;
					for (i = 0; i <= x; i++) {
						memory[idx+i] = v[i];
					}
					break;
				}
				case 0x65: {
					int i;
					for (i = 0; i <= x; i++) {
						v[i] = memory[idx+i];
					}
					break;
				}
			}
	}

	int i;
	for (i = lastOPcount - 2; i >= 0; i--) {
		lastop[i] = lastop[i+1];
	}
	lastop[lastOPcount - 1] = op;
}

void loadSpriteToMem() {
	const uint8_t sprites[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,
        0x20, 0x60, 0x20, 0x20, 0x70,
        0xF0, 0x10, 0xF0, 0x80, 0xF0,
        0xF0, 0x10, 0xF0, 0x10, 0xF0,
        0x90, 0x90, 0xF0, 0x10, 0x10,
        0xF0, 0x80, 0xF0, 0x10, 0xF0,
        0xF0, 0x80, 0xF0, 0x90, 0xF0,
        0xF0, 0x10, 0x20, 0x40, 0x40,
        0xF0, 0x90, 0xF0, 0x90, 0xF0,
        0xF0, 0x90, 0xF0, 0x10, 0xF0,
        0xF0, 0x90, 0xF0, 0x90, 0x90,
        0xE0, 0x90, 0xE0, 0x90, 0xE0,
        0xF0, 0x80, 0x80, 0x80, 0xF0,
        0xE0, 0x90, 0x90, 0x90, 0xE0,
        0xF0, 0x80, 0xF0, 0x80, 0xF0,
        0xF0, 0x80, 0xF0, 0x80, 0x80 
	};
	/* font for 0-F */
	size_t n = sizeof(sprites)/sizeof(sprites[0]);
	int i;
	for (i = 0; i < n; i++)
		memory[i] = sprites[i];
}

void loadProgramToMem(size_t size, uint8_t *arr) {
	int i;
	for (i = 0; i < size; i++)
		memory[0x200 + i] = arr[i];
}

void updateTimers() {
	if (delayTimer > 0)
		delayTimer--;
	if (soundTimer > 0) 
		soundTimer--;
	SDL_PauseAudioDevice(audio_device_id, !soundTimer);
}

int loadRom(char *path) {
	printf("Loading rom: %s\n", path);

	FILE *rom = fopen(path, "rb");
	if (rom == NULL) {
		fprintf(stderr, "Failed to open ROM\n");
		return -1;
	}

	fseek(rom, 0, SEEK_END);
	size_t rom_size = ftell(rom);
	rewind(rom);

	uint8_t *rom_buffer = (uint8_t *) malloc(sizeof(uint8_t) * rom_size);
	if (rom_buffer == NULL) {
		fprintf(stderr, "Failed to allocate memory for ROM\n");
		return -1;
	}

	size_t result = fread(rom_buffer, sizeof(uint8_t), (size_t) rom_size, rom);
	if (result != rom_size) {
		fprintf(stderr, "Failed to read ROM\n");
		return -1;
	}

	if ((MEMORY_SIZE - 0x200) > rom_size) {
		int i;
		for (i = 0; i < rom_size; ++i) {
			memory[i+0x200] = rom_buffer[i];
		}
	}
	else  {
		fprintf(stderr, "Rom too large to fit in memory\n");
		return -1;
	}

	fclose(rom);
	free(rom_buffer);

	return 1;
}
