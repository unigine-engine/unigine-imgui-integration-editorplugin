#pragma once

#include <UnigineTextures.h>
#include <UnigineInput.h>
#include <UnigineMaterials.h>
#include <UnigineMeshDynamic.h>

struct ImGuiContext;

class ImGuiBackend final
{
public:
	void init();
	void shutdown();

	void newFrame(Unigine::Math::ivec2 context_pos, Unigine::Math::ivec2 context_size);
	void render(const Unigine::TexturePtr &texture);

private:
	void create_font_texture();
	void create_mesh();
	void create_material();

	int key_pressed(Unigine::Input::KEY key);
	int key_released(Unigine::Input::KEY key);
	int button_pressed(Unigine::Input::MOUSE_BUTTON button);
	int button_released(Unigine::Input::MOUSE_BUTTON button);
	int unicode_key_pressed(unsigned int key);
	void render_callback();

	ImGuiContext *ctx_{};
	Unigine::TexturePtr font_texture_;
	Unigine::MeshDynamicPtr mesh_;
	Unigine::MaterialPtr material_;
	Unigine::EventConnections event_connections_;
};
