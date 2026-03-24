#include "ao/game/AOSceneCompositor.h"

#include "ao/game/AOBackground.h"
#include "ao/game/AOTextBox.h"
#include "ao/game/ActiveICState.h"
#include "asset/ShaderAsset.h"
#include "render/Layer.h"
#include "render/RenderState.h"

static constexpr int BASE_W = 256;
static constexpr int BASE_H = 192;

class RainbowTextProvider : public ShaderUniformProvider {
  public:
    RainbowTextProvider(float time) : time_(time) {
    }
    std::unordered_map<std::string, UniformValue> get_uniforms() const override {
        return {{"u_time", time_}};
    }

  private:
    float time_;
};

static void add_character_layer(LayerGroup& scene, const ActiveICState& ic, int z_index, int base_w, int base_h) {
    if (!ic.emote_player.has_frame())
        return;
    Layer char_layer(ic.emote_player.asset(), ic.emote_player.current_frame_index(), z_index);
    float sprite_aspect = (float)ic.emote_player.asset()->width() / (float)ic.emote_player.asset()->height();
    float viewport_aspect = (float)base_w / (float)base_h;
    char_layer.transform().scale({sprite_aspect / viewport_aspect, 1.0f});
    scene.add_layer(z_index, std::move(char_layer));
}

// Add background + character + desk to a group using the shared background.
static void compose_scene_group(LayerGroup& group, const AOBackground& background, const ActiveICState* ic) {
    if (background.bg_asset())
        group.add_layer(0, Layer(background.bg_asset(), 0, 0));
    if (ic)
        add_character_layer(group, *ic, 5, BASE_W, BASE_H);
    if (background.desk_asset() && ic && ic->show_desk)
        group.add_layer(10, Layer(background.desk_asset(), 0, 10));
}

// Add background + character + desk to a group using IC-snapshotted assets
// (for the departing scene during a slide, where the shared background has
// already changed to the new position).
static void compose_departing_group(LayerGroup& group, const ActiveICState& ic) {
    if (ic.bg_asset)
        group.add_layer(0, Layer(ic.bg_asset, 0, 0));
    add_character_layer(group, ic, 5, BASE_W, BASE_H);
    if (ic.desk_asset && ic.show_desk)
        group.add_layer(10, Layer(ic.desk_asset, 0, 10));
}

// Add textbox layers (chatbox bg, text mesh, nameplate) to a group.
static void compose_textbox(LayerGroup& group, AOTextBox& textbox, float scene_time_s) {
    if (textbox.text_state() == AOTextBox::TextState::INACTIVE)
        return;

    auto chatbox_bg = textbox.chatbox_background();
    if (chatbox_bg && chatbox_bg->frame_count() > 0) {
        const auto& rect = textbox.chatbox_position();
        float ndc_w = (float)chatbox_bg->width() / BASE_W * 2.0f;
        float ndc_h = (float)chatbox_bg->height() / BASE_H * 2.0f;
        float ndc_x = ((float)rect.x / BASE_W) * 2.0f - 1.0f + ndc_w * 0.5f;
        float ndc_y = 1.0f - ((float)rect.y / BASE_H) * 2.0f - ndc_h * 0.5f;

        Layer bg_layer(chatbox_bg, 0, 20);
        bg_layer.transform().scale({ndc_w * 0.5f, ndc_h * 0.5f});
        bg_layer.transform().translate({ndc_x, ndc_y});
        group.add_layer(20, std::move(bg_layer));
    }

    auto atlas = textbox.message_atlas();
    auto mesh = textbox.message_mesh();
    auto shader = textbox.text_shader();
    if (atlas && mesh && mesh->index_count() > 0 && shader) {
        shader->set_uniform_provider(std::make_shared<RainbowTextProvider>(scene_time_s));
        Layer text_layer(atlas, 0, 21);
        text_layer.set_mesh(mesh);
        text_layer.set_shader(shader);
        group.add_layer(21, std::move(text_layer));
    }

    auto nameplate = textbox.get_nameplate();
    if (nameplate) {
        auto nl = textbox.nameplate_layout();
        float ndc_w = (float)nameplate->width() * nl.scale / BASE_W * 2.0f;
        float ndc_h = (float)nameplate->height() / (float)BASE_H * 2.0f;
        float ndc_x = ((float)nl.x / BASE_W) * 2.0f - 1.0f + ndc_w * 0.5f;
        float ndc_y = 1.0f - ((float)nl.y / BASE_H) * 2.0f - ndc_h * 0.5f;

        Layer nameplate_layer(nameplate, 0, 25);
        nameplate_layer.transform().scale({ndc_w * 0.5f, ndc_h * 0.5f});
        nameplate_layer.transform().translate({ndc_x, ndc_y});
        group.add_layer(25, std::move(nameplate_layer));
    }
}

RenderState AOSceneCompositor::compose(const AOBackground& background, const ActiveICState* active_ic,
                                       const ActiveICState* departing_ic, AOTextBox& textbox, float scene_time_s) {
    RenderState state;

    if (departing_ic) {
        // Slide transition: two separate layer groups for independent transforms.
        // Group 0 = departing (snapshotted bg), Group 1 = arriving (live bg).
        LayerGroup departing_group;
        compose_departing_group(departing_group, *departing_ic);
        state.add_layer_group(0, departing_group);

        LayerGroup arriving_group;
        compose_scene_group(arriving_group, background, active_ic);
        compose_textbox(arriving_group, textbox, scene_time_s);
        state.add_layer_group(1, arriving_group);
    }
    else {
        // Normal: single layer group.
        LayerGroup scene;
        compose_scene_group(scene, background, active_ic);
        compose_textbox(scene, textbox, scene_time_s);
        state.add_layer_group(0, scene);
    }

    return state;
}
