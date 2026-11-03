#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdlib>
#include <utility>
#include <string>

#define WINDOW_HEIGHT 480
#define WINDOW_WIDTH 640
#define WINDOW_TITLE "Pong Game"
#define MOVE_SPEED 5
#define TARGET_FPS 60
#define FRAME_TIME 1000/TARGET_FPS

struct Game {
    SDL_Window *window;
    SDL_Renderer *renderer;

    int score = 0;
    int frame = 0;

    SDL_FRect paddle1 = {20,180,20,100};
    SDL_FRect paddle2 = {600,180,20,100};
    SDL_FRect ball = {310,230,20,20};

    std::pair<int,int> ball_direction = {1,1};
    int ball_speed=2;
};

void draw(struct Game *g);
void update(struct Game *g);
void game_free(struct Game *g);
int get_paddle_direction();
std::pair<int,int> test_window_bounds(SDL_FRect rect);
void bind_to_window(SDL_FRect *rect);
int update_ball_direction(SDL_FRect ball, std::pair<int,int> *dir, SDL_FRect paddle1, SDL_FRect paddle2);
bool collide_paddle_ball(SDL_FRect &rect1, SDL_FRect rect2);
void game_over(struct Game *g);
void draw_game_over(SDL_Renderer *r);

void draw(struct Game *g) {
    SDL_SetRenderDrawColor(g->renderer,0,0,0,255);
    SDL_RenderClear(g->renderer);

    SDL_SetRenderDrawColor(g->renderer,255,255,255,255);
    SDL_SetRenderScale(g->renderer, 4.0f, 4.0f);
    SDL_RenderDebugTextFormat(g->renderer,310 / 4.0f,20 / 4.0f,"%d",g->score);

    SDL_SetRenderScale(g->renderer, 1.0f, 1.0f);
    SDL_RenderFillRect(g->renderer, &g->paddle1);
    SDL_RenderFillRect(g->renderer, &g->paddle2);
    SDL_RenderFillRect(g->renderer, &g->ball);

    SDL_RenderPresent(g->renderer);
}

void draw_game_over(SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r,0,0,0,255);
    SDL_RenderClear(r);
    SDL_SetRenderDrawColor(r,255,255,255,255);
    SDL_SetRenderScale(r, 5.0f, 5.0f);
    SDL_RenderDebugText(r, (float)(640-50*SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)/2/5.0f, (480-SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE*5)/2/5.0f,"Game over!");
    SDL_RenderPresent(r);
}

void update(struct Game *g) {
    int dir=get_paddle_direction();
    g->paddle1.y += dir*MOVE_SPEED;
    g->paddle2.y += dir*MOVE_SPEED;

    g->ball.x += g->ball_direction.first*2;
    g->ball.y += g->ball_direction.second*2;

    bind_to_window(&g->paddle1);
    bind_to_window(&g->paddle2);

    if(update_ball_direction(g->ball, &g->ball_direction, g->paddle1, g->paddle2)) {
        game_over(g);
        draw_game_over(g->renderer);
        SDL_Delay(2000);
    }

}

void game_free(struct Game *g) {
    if(g->renderer) {
        SDL_DestroyRenderer(g->renderer);
        g->renderer = NULL;
    }
    if(g->window) {
        SDL_DestroyWindow(g->window);
        g->window = NULL;
    }
}

int get_paddle_direction() {
    const bool *key_states = SDL_GetKeyboardState(NULL);

    int direction=0;

    if(key_states[SDL_SCANCODE_UP])
        direction = -1;
    
    if(key_states[SDL_SCANCODE_DOWN])
        direction = 1;

    return direction;
}

std::pair<int,int> test_window_bounds(SDL_FRect rect) {
    std::pair<int,int> result = {0,0};

    if(rect.x < 0)
        result.first = 1;
    if(rect.y < 0)
        result.second = 1;
    if(rect.y + rect.h > WINDOW_HEIGHT)
        result.second = -1;
    if(rect.x + rect.w > WINDOW_WIDTH)
        result.first = -1;

    return result;
}

void bind_to_window(SDL_FRect *rect) {
    std::pair<int,int> bounds = test_window_bounds(*rect);

    if(bounds.first == 1)
        rect->x = 0;
    if(bounds.second == 1)
        rect->y = 0;
    if(bounds.second == -1)
        rect->y = WINDOW_HEIGHT - rect->h;
    if(bounds.first == -1)
        rect->x = WINDOW_WIDTH - rect->w;
}

bool collide_paddle_ball(SDL_FRect &rect1, SDL_FRect rect2) {
    if(!((rect1.y > rect2.y && rect1.y < rect2.y + rect2.h) || (rect1.y+rect1.h > rect2.y && rect1.y+rect1.h < rect2.y+rect2.h)))
        return false;
    if(rect1.x > rect2.x && rect1.x < rect2.x + rect2.w) {
        rect1.x = rect2.x + rect2.w;
        return true;
    }
    if(rect1.x + rect1.w > rect2.x && rect1.x + rect1.w < rect2.x + rect2.w) {
        rect1.x = rect2.x - rect1.w;
        return true;
    }
    return false;
}

int update_ball_direction(SDL_FRect ball, std::pair<int,int> *dir, SDL_FRect paddle1, SDL_FRect paddle2) {
    std::pair<int,int> bounds = test_window_bounds(ball);

    if(bounds.second)
        dir->second *= -1;
    if(bounds.first)
        return 1;

    if(collide_paddle_ball(ball,paddle1) || collide_paddle_ball(ball,paddle2))
        dir->first *= -1;
    return 0;
}

void game_over(struct Game *g) {
    g->paddle1 = {20,180,20,100};
    g->paddle2 = {600,180,20,100};
    g->ball = {310,230,20,20};
    g->ball_direction = {1,1};
    g->score = 0;
    g->frame = 0;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error initializing video: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    struct Game *game = new Game{};
    *appstate = game;

    if(!SDL_CreateWindowAndRenderer(WINDOW_TITLE,WINDOW_WIDTH,WINDOW_HEIGHT,SDL_WINDOW_RESIZABLE,&game->window,&game->renderer)) {
        SDL_Log("Error creating window: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    Uint32 start_ticks = SDL_GetTicks();

    struct Game* g = static_cast<Game*>(appstate);

    draw(g);
    update(g);

    Uint32 frame_ticks = SDL_GetTicks() - start_ticks;

    if(g->frame == 60)
        g->score++,g->frame=0;

    g->ball_speed = 2 + g->score/5;

    if(frame_ticks < FRAME_TIME)
        SDL_Delay(FRAME_TIME - frame_ticks);
    g->frame++;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if(event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    struct Game* g = static_cast<Game*>(appstate);
    if(g) {
        game_free(g);
        delete g;
    }
    SDL_Quit();
}