#include "Menu.h"
#include "Utility.h"

Menu::~Menu()
{
    delete [] m_state.enemies;
    delete    m_state.player;
    delete    m_state.map;
    Mix_FreeChunk(m_state.jump_sfx);
    Mix_FreeMusic(m_state.bgm);
}

void Menu::initialise()
{
    m_state.next_scene_id = -1;
    /**
     BGM and SFX
     */
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    Mix_PlayMusic(m_state.bgm, -1);
    Mix_VolumeMusic(0.0f);
}

void Menu::update(float delta_time)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type)
                {
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                    case SDLK_RETURN:
                        m_state.next_scene_id = 1;
                    }
                }
            }
    }

void Menu::render(ShaderProgram *program)
{
    GLuint text_id = Utility::load_texture("assets/font1.png");
        Utility::draw_text(program, text_id, "Chinchilla :p", 1, 0, glm::vec3(-4.5, 0.0, 0.0));
        Utility::draw_text(program, text_id, "Press enter to start", 0.4, 0, glm::vec3(-4.0, -1.0, 0.0));
}
