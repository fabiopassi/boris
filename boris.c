#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>


/* Program: Boris the fish moving in the window */

/* IMPOVEMENTS TO DO:
    -> Make boris breathe
    -> Implement death by excessive food
    -> do not let Boris come too close to treasure to avoid bugs with water drops
 */


/* Symbolic constants */
#define FONT "/usr/share/fonts/truetype/freefont/FreeSerif.ttf"
#define WINDOW_HEIGHT 700
#define WINDOW_WIDTH 700
#define FPS 60
#define TIME_FIXED_MOTION (2 * FPS)
#define F4_DURATION 3
#define F4_MESSAGE "F4 BASITO"
#define AVG_TIME_BETWEEN_BUBBLE (5 * FPS)
#define TREASURE_OPEN_STATE_DURATION (3 * FPS)
#define BUBBLE_VELOCITY 40
#define MIN_VELOCITY 10
#define MAX_VELOCITY 50
#define STD_RED 130
#define STD_GREEN 160
#define STD_BLUE 190
#define GREENER 0.0005
#define MAX_GREEN 200
#define MAX_TIME_DIRTY_WATER 60*FPS
#define MAX_TIME_BEFORE_SKELETON 120*FPS
#define FOOD_PIECES 6
#define random_int(MIN, MAX) (rand() % ((MAX)-(MIN)+1) + MIN)


/* Typedef */

typedef struct {
    float x, y;
    float v_x, v_y;
    float time_left_fixed_v;
    enum {DEAD, ALIVE} health_state;
    int skeleton_counter;
    int bool_skeleton;
    int time_dirty_water;
    int stop_f4;
} Fish;


typedef struct {
    int x,y;
    int animation_fps;
    int num_frames;
    int fps_counter;
    int animation_step;
} Plant;


typedef struct {
    int x, y;
    enum {CLOSE, OPEN} state;
    int countdown_closure;
} Treasure;


typedef struct {
    float x, y;
    int v_x, v_y;
    int exists;
    int animation_fps;
    int num_frames;
    int fps_counter;
    int animation_step;
} Bubble;


typedef struct {
    float x[FOOD_PIECES];
    float y[FOOD_PIECES];
    int v_x[FOOD_PIECES];
    int v_y[FOOD_PIECES];
    enum {EATEN, NOT_EATEN} state[FOOD_PIECES];
    int width, height;
    int exists;
} Food;



/* Global variables */
const float dt = (float) 1.0/FPS;
int quit = 0;
Fish boris = {0};
Plant algae_1 = {0};
Plant algae_2 = {0};
Plant bonsai = {0};
Treasure treasure = {0};
Bubble bubble = {0};
Food food;



/* Functions */

void randomize_v(){
    boris.v_x = random_int(MIN_VELOCITY, MAX_VELOCITY);
    boris.v_x *= (rand() % 2 == 0) ? (1) : (-1);
    boris.v_y = random_int(MIN_VELOCITY, MAX_VELOCITY);
    boris.v_y *= (rand() % 2 == 0) ? (1) : (-1);
    return;
}


void init_fish(SDL_Rect* fish_rect) {
    boris.x = WINDOW_WIDTH/2 - fish_rect->w/2 + random_int(-WINDOW_WIDTH/4, WINDOW_WIDTH/4);
    boris.y = WINDOW_HEIGHT/2 - fish_rect->h/2 + random_int(-WINDOW_HEIGHT/4, WINDOW_HEIGHT/4);
    randomize_v();
    boris.time_left_fixed_v = TIME_FIXED_MOTION;
    boris.health_state = ALIVE;
    boris.time_dirty_water = 0;
    boris.stop_f4 = 0;
    return;
}


void init_treasure(SDL_Rect* treasure_rect){
    treasure.x = (int) WINDOW_WIDTH - 1.3 * treasure_rect->w;
    treasure.y = (int) WINDOW_HEIGHT - treasure_rect->h;
    treasure.state = CLOSE;
    treasure.countdown_closure = 0;
    return;
}


void init_bubble(SDL_Rect* treasure_rect, SDL_Rect* bubble_rect){
    bubble.x = treasure.x + treasure_rect->w/2 - bubble_rect->w/2;
    bubble.y = treasure.y;
    bubble.v_x = 0;
    bubble.v_y = -BUBBLE_VELOCITY;
    bubble.exists = 1;
    return;
}


void init_plants(SDL_Rect* bonsai_rect, SDL_Rect* algae_rect){
    bonsai.y  = (int) WINDOW_HEIGHT - bonsai_rect->h;
    bonsai.x  = (int) bonsai_rect->w * 0.3;
    algae_1.y =(int) WINDOW_HEIGHT - algae_rect->h;
    algae_1.x =(int) WINDOW_WIDTH/2 - algae_rect->w * 1.2;
    algae_2.y =(int) algae_1.y;
    algae_2.x =(int) algae_1.x + algae_rect->w * 1.2;
    return;
}


void init_food(){
    food.width = WINDOW_WIDTH / 50;
    food.height = WINDOW_WIDTH / 50;
    food.exists = 1;

    int dx = WINDOW_WIDTH/3;
    int center_x = random_int(dx, WINDOW_WIDTH - food.width - dx);    

    for(int i = 0; i < FOOD_PIECES; i++){
        food.x[i] = random_int(center_x-dx, center_x+dx);
        food.y[i] = -food.height;
        food.v_x[i] = 0;
        food.v_y[i] = MIN_VELOCITY;
        food.state[i] = NOT_EATEN;
    }
    
    return;
}


void move_fish(SDL_Rect* fish_rect) {

    if (boris.health_state == ALIVE){

        /* Check if motion has to be changed ( classic red fish habit) */
        boris.time_left_fixed_v --;

        if (boris.time_left_fixed_v < 0) {
            randomize_v();
            boris.time_left_fixed_v = TIME_FIXED_MOTION;
        }
        /* Make the fish attracted by the closest food piece, if there is food */
        if (food.exists){
            int closer = 0;
            float min_dist = WINDOW_HEIGHT * WINDOW_HEIGHT;
            float dist;
            for (int i = 0; i < FOOD_PIECES; i++){
                if (food.state[i] == NOT_EATEN){
                    dist = pow(boris.x - food.x[i], 2) + pow(boris.y - food.y[i], 2);
                    if (dist < min_dist){
                        closer = i;
                        min_dist = dist;
                    }
                }
            }
            float dx = food.x[closer] - boris.x;
            float dy = food.y[closer] - boris.y;

            if (min_dist > 1){
                boris.v_x = dx / sqrt(min_dist) * MAX_VELOCITY * 2;
                boris.v_y = dy / sqrt(min_dist) * MAX_VELOCITY * 2;
            }
        }
        /* Check if the fish is arrived at the border and, in case, invert motion */
        if (boris.x < 1) { boris.v_x = random_int(MIN_VELOCITY, MAX_VELOCITY); }
        if (boris.y < 1) { boris.v_y = random_int(MIN_VELOCITY, MAX_VELOCITY); }
        if (boris.x > WINDOW_WIDTH - fish_rect->w - 1) { boris.v_x = (-1) * random_int(MIN_VELOCITY, MAX_VELOCITY); }
        if (boris.y > WINDOW_HEIGHT - fish_rect->h - 1) { boris.v_y = (-1) * random_int(MIN_VELOCITY, MAX_VELOCITY); }

    } else {
        boris.v_x = 0;
        boris.v_y = ( boris.y > 0 ? -MIN_VELOCITY : 0);
    }

    boris.x += boris.v_x * dt;
    boris.y += boris.v_y * dt;

    return;
}


void move_food(){
    for(int i = 0; i < FOOD_PIECES; i++){
        food.x[i] += food.v_x[i] * dt;
        food.y[i] += food.v_y[i] * dt;
    }

    return;
}


int bubble_boris_collision(SDL_Rect* fish_rect, SDL_Rect* bubble_rect){
    int bool_collision = 0;
    float dx = bubble.x - boris.x;
    float dy = bubble.y - boris.y;
    if (dx < fish_rect->w && dx > -bubble_rect->w && dy < fish_rect->h && dy > -bubble_rect->h){
        bool_collision = 1;
    }

    return bool_collision;
}


void check_boris_is_eating(SDL_Rect* fish_rect){
    if (boris.health_state == DEAD) { return; }

    for(int i = 0; i < FOOD_PIECES; i++){
        float dx = food.x[i] - boris.x;
        float dy = food.y[i] - boris.y;
        if (food.state[i] == NOT_EATEN && dx < fish_rect->w && dx > -food.width && dy < fish_rect->h && dy > -food.height){
            food.state[i] = EATEN;
        }
    }

    return;
}


void draw_fish(SDL_Renderer* rend, SDL_Texture* fish_tex, SDL_Rect* fish_rect) {
    fish_rect->x = (int) boris.x;
    fish_rect->y = (int) boris.y;
    SDL_RenderCopy(rend, fish_tex, NULL, fish_rect);

    return;
}


void free_audio_traces(Mix_Music* trace_1, Mix_Music* trace_2, Mix_Music* trace_3, Mix_Music* trace_4){
    Mix_FreeMusic(trace_1);
    Mix_FreeMusic(trace_2);
    Mix_FreeMusic(trace_3);
    Mix_FreeMusic(trace_4);
    return;
}



/* Main function */
int main(int argc, char** argv) {

    /* Initialize algae animation */
    algae_1.animation_fps = 17;                 					algae_2.animation_fps = 17;
    algae_1.num_frames = 3;                     					algae_2.num_frames = 3;
    algae_1.animation_step = random_int(0, algae_1.num_frames-1);  	algae_2.animation_step = random_int(0, algae_2.num_frames-1);
    algae_1.fps_counter = random_int(0, algae_1.animation_fps-1);     algae_2.fps_counter = random_int(0, algae_2.animation_fps-1);

    /* Initialize bubble animation */
    bubble.animation_fps = 10;
    bubble.num_frames = 4;
    bubble.animation_step = 0;
    bubble.fps_counter = 0;

    /* SDL initialization */
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		return 1;
    }

    /* Initialize SDL_mixer */
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16SYS;       /* Or try MIX_DEFAULT_FORMAT */
    int audio_channels = 2;
    int audio_buffers= 4096;

    if( Mix_OpenAudio( audio_rate, audio_format, audio_channels, audio_buffers) != 0 ) {
        fprintf(stderr, "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
        SDL_Quit();
        return 1;
    }

    /* Initialize the music */
    Mix_Music* audio_trace_monnezza = Mix_LoadMUS("./audios/monnezza.mp3");
    Mix_Music* audio_trace_smarmella = Mix_LoadMUS("./audios/smarmella.mp3");
    Mix_Music* audio_trace_cazzo_di_cane = Mix_LoadMUS("./audios/a_cazzo_di_cane.mp3");
    Mix_Music* audio_trace_qualita = Mix_LoadMUS("./audios/la_qualita_ci_ha_rotto_il_cazzo.mp3");
    if (!audio_trace_cazzo_di_cane || !audio_trace_monnezza || !audio_trace_qualita || !audio_trace_smarmella) {
        if (audio_trace_cazzo_di_cane != NULL) {Mix_FreeMusic(audio_trace_cazzo_di_cane);}
        if (audio_trace_monnezza != NULL) {Mix_FreeMusic(audio_trace_monnezza);}
        if (audio_trace_qualita != NULL) {Mix_FreeMusic(audio_trace_qualita);}
        if (audio_trace_smarmella != NULL) {Mix_FreeMusic(audio_trace_smarmella);}
        SDL_CloseAudio();
        SDL_Quit();
    }

    /* TTF initialization */
    if (TTF_Init() != 0) {
        fprintf(stderr, "SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
	}

    /* Define the font to write messages */
	TTF_Font* font = TTF_OpenFont(FONT, 96);
	if (!font){
		fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
		TTF_Quit();
        free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
        Mix_CloseAudio();
		SDL_Quit();
		return 1;
	}

    /* Create window for the game */
	SDL_Window* win = SDL_CreateWindow(
		"Gli occhi del cuore",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_HEIGHT,
		WINDOW_WIDTH,
		0
	);

    if (!win){
        fprintf(stderr, "Error in window initialization: %s\n\n", SDL_GetError());
        TTF_CloseFont(font);
        TTF_Quit();
        free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
	}

    /* Create renderer */
	uint32_t rend_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	SDL_Renderer* rend = SDL_CreateRenderer(win, -1, rend_flags);

	if (!rend){
		fprintf(stderr, "Error in renderer initialization: %s\n\n", SDL_GetError());
		SDL_DestroyWindow(win);
		TTF_CloseFont(font);
		TTF_Quit();
        free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
        Mix_CloseAudio();
		SDL_Quit();
		return 1;
	}

    /* Load fish images and other fancy stuff */
	SDL_Surface* fish_right_surf = IMG_Load("./images/right_fish.png");
	SDL_Surface* fish_left_surf = IMG_Load("./images/left_fish.png");
    SDL_Surface* fish_dead_surf = IMG_Load("./images/boris_dead.png");
    SDL_Surface* fish_skeleton_surf = IMG_Load("./images/boris_skeleton.png");
    SDL_Surface* treasure_closed_surf = IMG_Load("./images/treasure_closed.png");
    SDL_Surface* treasure_open_surf = IMG_Load("./images/treasure_open.png");
    SDL_Surface* bubble_surf = IMG_Load("./images/bubble_animation.png");
    SDL_Surface* bonsai_surf = IMG_Load("./images/bonsai.png");
    SDL_Surface* algae_surf = IMG_Load("./images/algae_animated.png");
    /* Load written text */
    SDL_Color basito_color = {255, 255, 255, SDL_ALPHA_OPAQUE};
    SDL_Surface* basito_surf = TTF_RenderText_Solid(font, F4_MESSAGE, basito_color);

	if (!fish_right_surf || !fish_left_surf || !fish_dead_surf || !fish_skeleton_surf || !treasure_closed_surf || !treasure_open_surf || !bubble_surf || !bonsai_surf || !algae_surf || !basito_surf){
		fprintf(stderr, "Error in surfaces initialization: %s\n\n", SDL_GetError());
		if (fish_left_surf != NULL){SDL_FreeSurface(fish_left_surf);}
		if (fish_right_surf != NULL){SDL_FreeSurface(fish_right_surf);}
		if (fish_dead_surf != NULL){SDL_FreeSurface(fish_dead_surf);}
		if (fish_skeleton_surf != NULL){SDL_FreeSurface(fish_skeleton_surf);}
        if (treasure_closed_surf != NULL){SDL_FreeSurface(treasure_closed_surf);}
        if (treasure_open_surf != NULL){SDL_FreeSurface(treasure_open_surf);}
        if (bubble_surf != NULL){SDL_FreeSurface(bubble_surf);}
        if (bonsai_surf != NULL){SDL_FreeSurface(bonsai_surf);}
        if (algae_surf != NULL){SDL_FreeSurface(algae_surf);}
        if (basito_surf != NULL) {SDL_FreeSurface(basito_surf);}
		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(win);
		TTF_CloseFont(font);
		TTF_Quit();
        free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
        Mix_CloseAudio();
		SDL_Quit();
		return 1;
	}


    /* Put images on textures and free the surfaces */
	SDL_Texture* fish_right = SDL_CreateTextureFromSurface(rend, fish_right_surf);
	SDL_FreeSurface(fish_right_surf);
    SDL_Texture* fish_left = SDL_CreateTextureFromSurface(rend, fish_left_surf);
	SDL_FreeSurface(fish_left_surf);
    SDL_Texture* fish_dead = SDL_CreateTextureFromSurface(rend, fish_dead_surf);
	SDL_FreeSurface(fish_dead_surf);
    SDL_Texture* fish_skeleton = SDL_CreateTextureFromSurface(rend, fish_skeleton_surf);
	SDL_FreeSurface(fish_skeleton_surf);
    SDL_Texture* treasure_closed = SDL_CreateTextureFromSurface(rend, treasure_closed_surf);
    SDL_FreeSurface(treasure_closed_surf);
    SDL_Texture* treasure_open = SDL_CreateTextureFromSurface(rend, treasure_open_surf);
    SDL_FreeSurface(treasure_open_surf);
    SDL_Texture* bubble_tex = SDL_CreateTextureFromSurface(rend, bubble_surf);
    SDL_FreeSurface(bubble_surf);
    SDL_Texture* bonsai_tex = SDL_CreateTextureFromSurface(rend, bonsai_surf);
    SDL_FreeSurface(bonsai_surf);
    SDL_Texture* algae_tex = SDL_CreateTextureFromSurface(rend, algae_surf);
    SDL_FreeSurface(algae_surf);
    /* Put text on texture */
    SDL_Texture* basito_tex = SDL_CreateTextureFromSurface(rend, basito_surf);
    SDL_FreeSurface(basito_surf);

	if (!fish_left || !fish_right || !fish_dead || !fish_skeleton || !treasure_closed || !treasure_open || !bubble_tex || !bonsai_tex || !algae_tex || !basito_tex) {
		fprintf(stderr, "Error in textures initialization: %s\n\n", SDL_GetError());
		if (fish_left != NULL) {SDL_DestroyTexture(fish_left);}
		if (fish_right != NULL) {SDL_DestroyTexture(fish_right);}
		if (fish_dead != NULL) {SDL_DestroyTexture(fish_dead);}
		if (fish_skeleton != NULL) {SDL_DestroyTexture(fish_skeleton);}
		if (treasure_closed != NULL) {SDL_DestroyTexture(treasure_closed);}
		if (treasure_open != NULL) {SDL_DestroyTexture(treasure_open);}
        if (bubble_tex != NULL) {SDL_DestroyTexture(bubble_tex);}
        if (bonsai_tex != NULL) {SDL_DestroyTexture(bonsai_tex);}
        if (algae_tex != NULL) {SDL_DestroyTexture(algae_tex);}
        if (basito_tex != NULL) {SDL_DestroyTexture(basito_tex);}
		SDL_DestroyRenderer(rend);
		SDL_DestroyWindow(win);
		TTF_CloseFont(font);
		TTF_Quit();
        free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
        Mix_CloseAudio();
		SDL_Quit();
		return 1;
	}

    /* Put textures in rect structs and tune the size of the objects  */
	SDL_Rect fish_rect;
	SDL_QueryTexture(fish_right, NULL, NULL, &fish_rect.w, &fish_rect.h);
    float fish_ratio = (float) fish_rect.h / fish_rect.w;
	fish_rect.w = WINDOW_WIDTH / 5 ;
	fish_rect.h = (int) fish_rect.w * fish_ratio;

    SDL_Rect treasure_rect;
    SDL_QueryTexture(treasure_closed, NULL, NULL, &treasure_rect.w, &treasure_rect.h);
    float treasure_ratio = (float) treasure_rect.h / treasure_rect.w;
    treasure_rect.w = WINDOW_WIDTH / 5;
    treasure_rect.h = (int) treasure_rect.w * treasure_ratio;

    SDL_Rect bubble_rect;
    SDL_QueryTexture(bubble_tex, NULL, NULL, &bubble_rect.w, &bubble_rect.h);
    float bubble_ratio = (float) bubble_rect.h / (bubble_rect.w / bubble.num_frames);
	bubble_rect.w = treasure_rect.w / 2 ;
	bubble_rect.h = (int) bubble_rect.w * bubble_ratio;

    SDL_Rect bubble_src_rect;
    SDL_QueryTexture(bubble_tex, NULL, NULL, &bubble_src_rect.w, &bubble_src_rect.h);
    bubble_src_rect.w /= bubble.num_frames;
    bubble_src_rect.x = 0;
    bubble_src_rect.y = 0;

    SDL_Rect bonsai_rect;
    SDL_QueryTexture(bonsai_tex, NULL, NULL, &bonsai_rect.w, &bonsai_rect.h);
    float bonsai_ratio = (float) bonsai_rect.h / bonsai_rect.w;
	bonsai_rect.w = WINDOW_WIDTH / 5;
	bonsai_rect.h = (int) bonsai_rect.w * bonsai_ratio;

    SDL_Rect algae_1_rect, algae_2_rect;
    SDL_QueryTexture(algae_tex, NULL, NULL, &algae_1_rect.w, &algae_1_rect.h);
    float algae_ratio = (float) algae_1_rect.h / (algae_1_rect.w / algae_1.num_frames);
	algae_1_rect.w = WINDOW_WIDTH / 6;
	algae_1_rect.h = (int) algae_1_rect.w * algae_ratio;
    algae_2_rect.w = algae_1_rect.w;
    algae_2_rect.h = algae_1_rect.h;

    SDL_Rect algae_1_src_rect;
    SDL_QueryTexture(algae_tex, NULL, NULL, &algae_1_src_rect.w, &algae_1_src_rect.h);
    algae_1_src_rect.w /= algae_1.num_frames;
    algae_1_src_rect.y = 0;

    SDL_Rect algae_2_src_rect;
    SDL_QueryTexture(algae_tex, NULL, NULL, &algae_2_src_rect.w, &algae_2_src_rect.h);
    algae_2_src_rect.w /= algae_2.num_frames;
    algae_2_src_rect.x = algae_2.animation_step * algae_2_src_rect.w;
    algae_2_src_rect.y = 0;

    SDL_Rect basito_rect;
    SDL_QueryTexture(basito_tex, NULL, NULL, &basito_rect.w, &basito_rect.h);
    basito_rect.x = (WINDOW_WIDTH - basito_rect.w) / 2;
    basito_rect.y = (WINDOW_HEIGHT - basito_rect.h) / 2;

    SDL_Rect food_rect;

    /* Initialize everything (except bubble, initially non-existent) */
    init_fish(&fish_rect);
    init_treasure(&treasure_rect);
    init_plants(&bonsai_rect, &algae_1_rect);

    /* Fix positions of plants and treasure */
    treasure_rect.x = (int) treasure.x;
    treasure_rect.y = (int) treasure.y;
    bonsai_rect.x  = bonsai.x;
    bonsai_rect.y  = bonsai.y;
    algae_1_rect.x = algae_1.x;
    algae_2_rect.x = algae_2.x;
    algae_1_rect.y = algae_1.y;
    algae_2_rect.y = algae_2.y;

    /* "Game" loop */
    struct timeval begin, end;
	long seconds, microseconds;
	double elapsed;

    float red = STD_RED;
    float green = STD_GREEN;
    float blue = STD_BLUE;

    while( ! quit ) {

		/* Start measuring time */
    	gettimeofday(&begin, 0);

		/* Read input */
		SDL_Event input;
		while (SDL_PollEvent(&input)) {

			switch (input.type) {
                /* Quit */
                case SDL_QUIT:
                    quit = 1;
                    break;
                /* Key pressed */
                case SDL_KEYDOWN:
                    switch (input.key.keysym.scancode) {
                        case SDL_SCANCODE_F4:
                            boris.stop_f4 = 1;
                            break;
                        case SDL_SCANCODE_M:
                            Mix_PlayMusic(audio_trace_monnezza, 1);
                            break;
                        case SDL_SCANCODE_S:
                            Mix_PlayMusic(audio_trace_smarmella, 1);
                            break;
                        case SDL_SCANCODE_A:
                            Mix_PlayMusic(audio_trace_cazzo_di_cane, 1);
                            break;
                        case SDL_SCANCODE_Q:
                            Mix_PlayMusic(audio_trace_qualita, 1);
                            break;
                        case SDL_SCANCODE_C:
                            green = STD_GREEN;
                            boris.time_dirty_water = 0;
                            break;
                        case SDL_SCANCODE_F:
                            if (! food.exists){
                                init_food();
                                food.exists = 1;
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                /* Rest */
                default:
                    break;
            }
		}

        /* Greener background with time */
        if (green < MAX_GREEN) green += GREENER;

        /* Increase boris toxicity if water is dirty */
        if (green >= MAX_GREEN) boris.time_dirty_water++;

        /* Boris dies if he stays in bad water too long */
        if (boris.time_dirty_water == MAX_TIME_DIRTY_WATER) boris.health_state = DEAD;

        if ( !boris.stop_f4 ) {

            /* If boris is dead and time goes by, it becomes a skeleton after a while */
            if (boris.health_state == DEAD && boris.skeleton_counter++ > MAX_TIME_BEFORE_SKELETON){
                boris.bool_skeleton = 1;
            }

            /* Move food, if existent */
            if (food.exists){ move_food(); }

            /* Check if boris has eaten the food */
            if (food.exists){ check_boris_is_eating(&fish_rect); }

            /* Update food existence (if all eaten, the food.exists variable is zero) */
            food.exists = 0;
            for (int i = -1; ++i < FOOD_PIECES; food.exists += food.state[i]);

            /* Move boris (attracted by food, if exists) */
            move_fish(&fish_rect);

            /* Spawn bubble (if not existent) */
            if ( ! bubble.exists && bubble.animation_step == 0 && rand() % AVG_TIME_BETWEEN_BUBBLE == 0 ){
                init_bubble(&treasure_rect, &bubble_rect);
                treasure.state = OPEN;
                treasure.countdown_closure = TREASURE_OPEN_STATE_DURATION;
            }

            /* Remove bubble if beyond screen and existent */
            if (bubble.exists && bubble.y < 0) {
                bubble.animation_step = 1;
                bubble.exists = 0;
            }

            /* Destroy bubble if existent and hits boris */
            if( bubble.exists && bubble_boris_collision(&fish_rect, &bubble_rect)){
                bubble.animation_step = 1;
                bubble.exists = 0;
            }

            /* Move bubble, if exists and is not exploding */
            if (bubble.exists && bubble.animation_step == 0){
                bubble.x += bubble.v_x * dt;
                bubble.y += bubble.v_y * dt;
            }

            /* If treasure is open, update countdown to closure. If time is over, close it */
            if (treasure.state == OPEN && --treasure.countdown_closure == 0){
                treasure.state = CLOSE;
            }

            /* Rendering */
            SDL_RenderClear(rend);

            /* Draw Boris */
            if ( boris.v_x >= 0 && boris.health_state == ALIVE ) draw_fish(rend, fish_right, &fish_rect);
            if ( boris.v_x <= 0 && boris.health_state == ALIVE ) draw_fish(rend, fish_left, &fish_rect);
            if ( boris.health_state == DEAD && boris.bool_skeleton == 0 ) draw_fish(rend, fish_dead, &fish_rect);
            if ( boris.health_state == DEAD && boris.bool_skeleton == 1 ) draw_fish(rend, fish_skeleton, &fish_rect);

            /* Draw treasure */
            if (treasure.state == CLOSE) {
                SDL_RenderCopy(rend, treasure_closed, NULL, &treasure_rect);
            } else {
                SDL_RenderCopy(rend, treasure_open, NULL, &treasure_rect);
            }

            /* If bubble exists, draw it. If it does not exists but it is exploding, draw it with animation */
            if (bubble.exists) {
                bubble_rect.x = (int) bubble.x;
                bubble_rect.y = (int) bubble.y;
                SDL_RenderCopy(rend, bubble_tex, &bubble_src_rect, &bubble_rect);
            }
            if (bubble.animation_step != 0){
                SDL_RenderCopy(rend, bubble_tex, &bubble_src_rect, &bubble_rect);
                bubble.fps_counter++;
                if (bubble.fps_counter == bubble.animation_fps){
                    bubble.fps_counter = 0;
                    if (bubble.animation_step == bubble.num_frames-1){
                        bubble.x = 0;
                        bubble.y = 0;
                        bubble.animation_step = 0;
                    } else {
                        bubble.animation_step++;
                    }
                    bubble_src_rect.x = bubble.animation_step * bubble_src_rect.w;
                }
            }

            /* Draw food */
            for (int i = 0; i < FOOD_PIECES; i++){
                if (food.state[i] == NOT_EATEN){
                    food_rect.x = (int) food.x[i];
                    food_rect.y = (int) food.y[i];
                    food_rect.w = food.width;
                    food_rect.h = food.height;
                    SDL_SetRenderDrawColor(rend, 150, 100, 10, 255);
                    SDL_RenderFillRect(rend, &food_rect);
                }
            }

            /* Draw bonsai */
            SDL_RenderCopy(rend, bonsai_tex, NULL, &bonsai_rect);

            /* Draw moving algae */
            algae_1.fps_counter ++;
            if (algae_1.fps_counter == algae_1.animation_fps){
                algae_1.fps_counter = 0;
                algae_1.animation_step = ( algae_1.animation_step == algae_1.num_frames-1 ? 0 : algae_1.animation_step+1 );
	            algae_1_src_rect.x = algae_1.animation_step * algae_1_src_rect.w;
            }
            SDL_RenderCopy(rend, algae_tex, &algae_1_src_rect, &algae_1_rect);


            algae_2.fps_counter ++;
            if (algae_2.fps_counter == algae_2.animation_fps){
                algae_2.fps_counter = 0;
                algae_2.animation_step = ( algae_2.animation_step == algae_2.num_frames-1 ? 0 : algae_2.animation_step+1 );
	            algae_2_src_rect.x = algae_2.animation_step * algae_2_src_rect.w;
            }
            SDL_RenderCopy(rend, algae_tex, &algae_2_src_rect, &algae_2_rect);

            /* Water background  */
            SDL_SetRenderDrawColor(rend, (int) red, (int) green, (int) blue, 255);
            SDL_RenderPresent(rend);

        } else {

            /* If F4 pressed, simply draw background with "basito" written on it */
            SDL_RenderClear(rend);
            SDL_RenderCopy(rend, basito_tex, NULL, &basito_rect);
            SDL_SetRenderDrawColor(rend, 10, 10, 10, 255);
            SDL_RenderPresent(rend);
            SDL_Delay(F4_DURATION * 1000);
            boris.stop_f4 = 0;

        }

        /* Wait time if necessary to achieve desired FPS */
        gettimeofday(&end, 0);
    	seconds = end.tv_sec - begin.tv_sec;
    	microseconds = end.tv_usec - begin.tv_usec;
    	elapsed = seconds + microseconds*1e-6;
		if (elapsed < 1000 * dt) { SDL_Delay(1000 * dt - elapsed); }

    }

    /* Stop whatever music is playing */
    Mix_HaltMusic();

    /* Free resources */
	SDL_DestroyTexture(fish_left);
	SDL_DestroyTexture(fish_right);
	SDL_DestroyTexture(fish_dead);
	SDL_DestroyTexture(fish_skeleton);
    SDL_DestroyTexture(treasure_closed);
    SDL_DestroyTexture(treasure_open);
    SDL_DestroyTexture(bubble_tex);
    SDL_DestroyTexture(bonsai_tex);
    SDL_DestroyTexture(algae_tex);
    SDL_DestroyTexture(basito_tex);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	TTF_CloseFont(font);
	TTF_Quit();
    free_audio_traces(audio_trace_monnezza, audio_trace_smarmella, audio_trace_cazzo_di_cane, audio_trace_qualita);
    Mix_CloseAudio();
	SDL_Quit();

    return 0;
}
