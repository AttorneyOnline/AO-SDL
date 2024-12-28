#include "GameThread.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <Image.h>
#include <Layer.h>
#include <RenderState.h>
#include <RenderManager.h>
#include <StateBuffer.h>

#include "utils/Log.h"

#include <RenderState.h>

GameThread::GameThread(StateBuffer& render_buffer) : render_buffer(render_buffer), tick_thread(&GameThread::tick, this) {

}

void GameThread::tick() {
    RenderState initial_state;

    int width, height, num_channels;

    stbi_set_flip_vertically_on_load(true);
    uint8_t* bg_pixels = stbi_load(
        "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\base\\background\\default\\defenseempty.png",
        &width, &height, &num_channels, 0);
    Image bg_img(width, height, bg_pixels, num_channels);

    uint8_t* desk_pixels = stbi_load(
        "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\base\\background\\default\\defensedesk.png", &width,
        &height, &num_channels, 0);
    Image desk_img(width, height, desk_pixels, num_channels);

    uint8_t* char_pixels = stbi_load(
        "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\base\\characters\\Phoenix\\(a)normal.apng", &width,
        &height, &num_channels, 0);
    Animation char_img(width, height, char_pixels, num_channels);

    Layer background(bg_img, 0);
    Layer character(char_img, 1);
    Layer desk(desk_img, 2);

    LayerGroup main_layers;
    main_layers.add_layer(0, background);
    main_layers.add_layer(1, desk);
    main_layers.add_layer(2, character);

    initial_state.add_layer_group(0, main_layers);

    RenderState blank;

    uint64_t ticks = 0;

    while (true) {
        RenderState new_state;
        LayerGroup main;

        if (ticks > 0 && ticks < 10) {
            Layer thing(bg_img, 0);
            main.add_layer(0, thing);
        }
        else if (ticks >= 10 && ticks < 20) {
            Layer thing(desk_img, 0);
            main.add_layer(0, thing);
        }
        else {
            ticks = 0;
        }

        new_state.add_layer_group(0, main);

        RenderState* state = render_buffer.get_producer_buf();
        *state = new_state;
        render_buffer.present();
        
        //Log::log_print(INFO, "Ticked game logic");

        ticks++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}