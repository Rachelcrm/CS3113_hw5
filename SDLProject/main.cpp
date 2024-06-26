/**
* Author: Rachel Chen
* Assignment: Platformer
* Date due: 2024-04-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define LEVEL1_WIDTH 14
#define LEVEL1_HEIGHT 8
#define LEVEL1_LEFT_EDGE 5.0f
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "LevelA.h"
#include "LevelB.h"
#include "levelC.h"
#include "menu.h"
#include "Effects.h"

// ––––– CONSTANTS ––––– //
bool endgame = false;
bool pause = false;
bool collide = false;

const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 1.0f,
            BG_BLUE    = 1.0f,
            BG_GREEN   = 1.0f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const int   CD_QUAL_FREQ    = 44100,
            AUDIO_CHAN_AMT  = 2,
            AUDIO_BUFF_SIZE = 4096;


// ––––– GLOBAL VARIABLES ––––– //
Scene  *g_current_scene;
Menu *g_menu; //Start page
LevelA *g_levelA;
LevelB *g_levelB;
LevelC *g_levelC;

Effects *g_effects;
Scene   *g_levels[4];

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

Mix_Music* g_music;
Mix_Chunk* g_bouncing_sfx;

const char  BGM_FILEPATH[]              = "assets/bgmusic.mp3",
            BOUNCING_SFX_FILEPATH[]     = "assets/bounce.wav";
const int   LOOP_FOREVER     = -1;

bool g_is_colliding_bottom = false;

int lives = 3;

// ––––– GENERAL FUNCTIONS ––––– //
void switch_to_scene(Scene *scene)
{    
    g_current_scene = scene;
    g_current_scene->initialise(); // DON'T FORGET THIS STEP!
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    
    g_display_window = SDL_CreateWindow("Welcome!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_levelA = new LevelA();
    g_levelB = new LevelB();
    g_levelC = new LevelC();
    g_menu = new Menu();
    
    g_levels[0] = g_menu;
    g_levels[1] = g_levelA;
    g_levels[2] = g_levelB;
    g_levels[3] = g_levelC;
    
    // Start at level A
    switch_to_scene(g_levels[0]);
    Mix_OpenAudio(
        CD_QUAL_FREQ,
        MIX_DEFAULT_FORMAT,
        AUDIO_CHAN_AMT,
        AUDIO_BUFF_SIZE
    );
    
    //Set BGM
    g_current_scene->m_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(Mix_LoadMUS(BGM_FILEPATH), LOOP_FOREVER);
    Mix_VolumeMusic(MIX_MAX_VOLUME);
    
    //Set matrix
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    g_effects = new Effects(g_projection_matrix, g_view_matrix);
    g_effects->start(SHRINK, 2.0f);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    if (g_current_scene != g_menu){
        g_current_scene->m_state.player->set_movement(glm::vec3(0.0f));
        
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type) {
                    // End game
                case SDL_QUIT:
                case SDL_WINDOWEVENT_CLOSE:
                    g_game_is_running = false;
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_q:
                            // Quit the game with a keystroke
                            g_game_is_running = false;
                            break;
                            
                        case SDLK_SPACE:
                            // Jump
                            if (g_current_scene->m_state.player->m_collided_bottom)
                            {
                                g_current_scene->m_state.player->m_is_jumping = true;
                                Mix_PlayChannel(-1, g_current_scene->m_state.jump_sfx, 0);
                            }
                            break;
                            
                        case SDLK_p:
                            // Freeze the screen
                            endgame = true;
                            break;
                            
                        case SDLK_j:
                            //continue playing the game if still alive
                            if (lives != 0 && g_current_scene->m_state.player->get_position().y >= -8.0f){
                            endgame = false;
                        break;}
                            
                        default:
                            break;
                    }
                    
                default:
                    break;
            }
        }
        
        
        const Uint8 *key_state = SDL_GetKeyboardState(NULL);
        
        if (key_state[SDL_SCANCODE_LEFT])
        {
            g_current_scene->m_state.player->m_movement.x = -1.0f;
            g_current_scene->m_state.player->m_animation_indices = g_current_scene->m_state.player->m_walking[g_current_scene->m_state.player->LEFT];
        }
        else if (key_state[SDL_SCANCODE_RIGHT])
        {
            g_current_scene->m_state.player->m_movement.x = 1.0f;
            g_current_scene->m_state.player->m_animation_indices = g_current_scene->m_state.player->m_walking[g_current_scene->m_state.player->RIGHT];
        }
        
        if (glm::length(g_current_scene->m_state.player->m_movement) > 1.0f)
        {
            g_current_scene->m_state.player->m_movement = glm::normalize(g_current_scene->m_state.player->m_movement);
        }
    }
}

void update()
{
    if (!endgame){
        float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
        float delta_time = ticks - g_previous_ticks;
        g_previous_ticks = ticks;
        
        delta_time += g_accumulator;
        
        if (delta_time < FIXED_TIMESTEP)
        {
            g_accumulator = delta_time;
            return;
        }
        
        while (delta_time >= FIXED_TIMESTEP) {
            g_current_scene->update(FIXED_TIMESTEP);
            g_effects->update(FIXED_TIMESTEP);
            delta_time -= FIXED_TIMESTEP;
        }
        
        if (g_current_scene != g_menu){
            //if collide with the cat, lives - 1
            bool check_collision1 = g_current_scene -> m_state.player->check_collision(g_current_scene->m_state.enemies);
            bool check_collition2 = (g_current_scene -> m_state.player->get_position().y - g_current_scene -> m_state.enemies->get_position().y <= 0.5f) && (abs(g_current_scene -> m_state.player->get_position().x - g_current_scene -> m_state.enemies->get_position().x) <= 0.5f);
            if (check_collision1 || check_collition2) {
                lives -= 1;
                if (lives == 0) {collide = true;} //die because of collision
                else { //if still alive, start from the beginning of this scene
                    if (g_current_scene == g_levelA) {switch_to_scene(g_levelA);}
                    else if (g_current_scene == g_levelB) {switch_to_scene(g_levelB);}
                    else if (g_current_scene == g_levelC) {switch_to_scene(g_levelC);}
                }
            }
            
            //Shake effect
            if (g_is_colliding_bottom == false && g_current_scene->m_state.player->m_collided_bottom) g_effects->start(SHAKE, 1.0f);
            g_is_colliding_bottom = g_current_scene->m_state.player->m_collided_bottom;
            
            g_accumulator = delta_time;
            
            // Prevent the camera from showing anything outside of the "edge" of the level
            g_view_matrix = glm::mat4(1.0f);
            
            if (g_current_scene->m_state.player->get_position().x > LEVEL1_LEFT_EDGE) {
                g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_current_scene->m_state.player->get_position().x, 3.75, 0));
            } else {
                g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-5, 3.75, 0));
            }
            //Change scene if touches the bottom
            if (g_current_scene == g_levelA && g_current_scene->m_state.player->get_position().y < -10.0f) switch_to_scene(g_levelB);
            if (g_current_scene == g_levelB && g_current_scene->m_state.player->get_position().y < -10.0f) switch_to_scene(g_levelC);
            if (g_current_scene == g_levelC && g_current_scene->m_state.player->get_position().y < -8.0f) endgame = true;
            g_view_matrix = glm::translate(g_view_matrix, g_effects->m_view_offset);}}
}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);
 
    GLuint text_id = Utility::load_texture("assets/font1.png");
    //Show lives left on screen through the game
    if (g_current_scene != g_menu && endgame != true) {
        Utility::draw_text(&g_shader_program, text_id, "Lives: " + std::to_string(lives), 0.4, 0, glm::vec3(1.5, -1.0, 0.0));
    }
    
    if (collide == true && lives == 0) {
        Utility::draw_text(&g_shader_program, text_id, "You Lose", 0.4, 0, glm::vec3(2.5, -3.0, 0.0));
        endgame = true;
    }
    
    if (endgame == true && lives != 0 && g_current_scene->m_state.player->get_position().y < -8.0f) {
        LOG("WIN!");
        Utility::draw_text(&g_shader_program, text_id, "You Win", 0.4, 0, glm::vec3(16.0, -3.0, 0.0));
        endgame = true;
    }
    
    glUseProgram(g_shader_program.get_program_id());
    g_current_scene->render(&g_shader_program);
    g_effects->render();
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{    
    Mix_FreeChunk(g_bouncing_sfx);
    Mix_FreeMusic(g_music);
    
    delete g_menu;
    delete g_levelA;
    delete g_levelB;
    delete g_levelC;
    delete g_effects;
    
    SDL_Quit();
}

// ––––– DRIVER GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        update();
        
        if (g_current_scene->m_state.next_scene_id >= 0) switch_to_scene(g_levels[g_current_scene->m_state.next_scene_id]);
        
        render();
    }
    
    shutdown();
    return 0;
}
