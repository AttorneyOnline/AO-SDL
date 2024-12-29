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

    int width, height, num_channels;

    stbi_set_flip_vertically_on_load(true);
    uint8_t* bg_pixels = stbi_load("S:\\Projects\\tsurushiage\\assets\\base\\background\\default\\defenseempty.png",
                                   &width, &height, &num_channels, 0);
    Image bg_img(width, height, bg_pixels, num_channels);

    uint8_t* desk_pixels = stbi_load("S:\\Projects\\tsurushiage\\assets\\base\\background\\default\\defensedesk.png",
                                     &width, &height, &num_channels, 0);
    Image desk_img(width, height, desk_pixels, num_channels);

    RenderState blank;

    uint64_t ticks = 0;

    while (true) {
        RenderState new_state;
        LayerGroup main;

        if (ticks >= 0 && ticks < 10) {
            Layer thing(bg_img, 0);
            Layer thing2(desk_img, 1);
            main.add_layer(0, thing);
            main.add_layer(1, thing2);
        }
        else if (ticks >= 10 && ticks < 20) {
            Layer thing(desk_img, 0);
            main.add_layer(0, thing);
        }
        else {
            Layer thing(desk_img, 0);
            main.add_layer(0, thing);
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
