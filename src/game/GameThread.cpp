#include "GameThread.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "render/Image.h"
#include "render/Layer.h"
#include "render/RenderManager.h"
#include "render/RenderState.h"
#include "render/StateBuffer.h"

#include "utils/Log.h"

#include "render/RenderState.h"

GameThread::GameThread(StateBuffer& render_buffer)
    : running(true), render_buffer(render_buffer), tick_thread(&GameThread::game_loop, this) {
}

void GameThread::stop() {
    running = false;
    tick_thread.join();
}

void GameThread::game_loop() {
    uint64_t t = 0;

    while (running) {
        tick(t);
        t++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void GameThread::render(RenderState new_state) {
    RenderState* state = render_buffer.get_producer_buf();
    *state = new_state;
    render_buffer.present();
}

void GameThread::tick(uint64_t t) {
    int width, height, num_channels;

    // todo:
    // *** THIS LEAKS A LOT OF MEMORY, AND FAST ***
    // Doing this every tick is very, very, very bad
    // It is like this right now until we have properly asynchronous asset loading
    // And probably also a cache to keep memory usage sane
    // *** THIS LEAKS A LOT OF MEMORY, AND FAST ***
    stbi_set_flip_vertically_on_load(true);
    uint8_t* bg_pixels = stbi_load(
        "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\base\\background\\default\\defenseempty.png",
        &width, &height, &num_channels, 0);
    Image bg_img(width, height, bg_pixels, num_channels);

    uint8_t* desk_pixels = stbi_load(
        "C:\\Users\\Marisa\\Documents\\aolibs\\tsurushiage\\assets\\base\\background\\default\\defensedesk.png", &width,
        &height, &num_channels, 0);
    Image desk_img(width, height, desk_pixels, num_channels);

    uint64_t ticks = t % 20;

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

    new_state.add_layer_group(0, main);

    render(new_state);
}
