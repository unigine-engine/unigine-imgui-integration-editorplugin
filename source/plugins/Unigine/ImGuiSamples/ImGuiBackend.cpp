#include "ImGuiBackend.h"

#include "imgui/imgui.h"

#include <UnigineControls.h>
#include <UnigineEngine.h>
#include <UnigineFileSystem.h>
#include <UnigineMaterials.h>
#include <UnigineMeshDynamic.h>
#include <UnigineRender.h>
#include <UnigineTextures.h>

using namespace Unigine;
using namespace Math;

namespace
{

static ImGuiKey keymap[Input::NUM_KEYS];

static const vec4 BG_COLOR = vec4(65 / 255.0f, 66 / 255.0f, 69 / 255.0f, 1.0f);

static void set_clipboard_text(void *, const char *text)
{
	Input::setClipboard(text);
}

static char const *get_clipboard_text(void *)
{
	return Input::getClipboard();
}

} // anonymous namespace

void ImGuiBackend::init()
{
	IMGUI_CHECKVERSION();

	if (ctx_)
	{
		return;
	}

	ctx_ = ImGui::CreateContext();
	ImGui::SetCurrentContext(ctx_);

	Input::getEventKeyDown().connect(event_connections_, this, &ImGuiBackend::key_pressed);
	Input::getEventKeyUp().connect(event_connections_, this, &ImGuiBackend::key_released);
	Input::getEventMouseDown().connect(event_connections_, this, &ImGuiBackend::button_pressed);
	Input::getEventMouseUp().connect(event_connections_, this, &ImGuiBackend::button_released);
	Input::getEventTextPress().connect(event_connections_, this,
		&ImGuiBackend::unicode_key_pressed);
	Engine::get()->getEventBeginRender().connect(event_connections_, this,
		&ImGuiBackend::render_callback);

	ImGuiIO &io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	io.BackendPlatformName = "imgui_impl_unigine";
	io.BackendRendererName = "imgui_impl_unigine";

	init_keymap();

	io.SetClipboardTextFn = set_clipboard_text;
	io.GetClipboardTextFn = get_clipboard_text;
	io.ClipboardUserData = nullptr;

	create_font_texture();
	create_mesh();
	create_material();

	init_theme();
}

void ImGuiBackend::shutdown()
{
	material_.deleteForce();

	event_connections_.disconnectAll();

	if (ctx_)
	{
		ImGui::DestroyContext(ctx_);
	}
}

void ImGuiBackend::newFrame(Unigine::Math::ivec2 context_pos, Unigine::Math::ivec2 context_size)
{
	if (ctx_ == nullptr)
	{
		return;
	}

	ImGui::SetCurrentContext(ctx_);

	auto &io = ImGui::GetIO();

	ControlsApp::setEnabled(!io.WantCaptureKeyboard);

	io.DisplaySize = ImVec2(Math::toFloat(context_size.x), Math::toFloat(context_size.y));
	io.DeltaTime = Engine::get()->getIFps();

	if (io.WantSetMousePos)
	{
		Input::setMousePosition(Math::ivec2(Math::ftoi(io.MousePos.x), Math::ftoi(io.MousePos.y)));
	}

	const Math::ivec2 mouse_coord = Input::getMousePosition() - context_pos;

	io.MousePos = ImVec2(static_cast<float>(mouse_coord.x), static_cast<float>(mouse_coord.y));
	io.MouseWheel += static_cast<float>(Input::getMouseWheel());
	io.MouseWheelH += static_cast<float>(Input::getMouseWheelHorizontal());

	ImGui::NewFrame();
}

void ImGuiBackend::render(const Unigine::TexturePtr &texture)
{
	if (texture.isNull())
	{
		return;
	}

	ImGui::SetCurrentContext(ctx_);
	ImGui::Render();
	ImDrawData *frame_draw_data = ImGui::GetDrawData();

	if (material_.isNull() || texture.isNull())
	{
		return;
	}

	Input::setMouseHandle(Input::MOUSE_HANDLE::MOUSE_HANDLE_SOFT);

	// clear with editor's color theme
	texture->clearBuffer(BG_COLOR);

	if (frame_draw_data == nullptr)
	{
		return;
	}

	auto draw_data = frame_draw_data;
	frame_draw_data = nullptr;

	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
	{
		return;
	}

	auto render_target = Render::getTemporaryRenderTarget();
	render_target->bindColorTexture(0, texture);

	// Render state
	RenderState::saveState();
	RenderState::clearStates();
	RenderState::setBlendFunc(RenderState::BLEND_SRC_ALPHA, RenderState::BLEND_ONE_MINUS_SRC_ALPHA,
		RenderState::BLEND_OP_ADD);
	RenderState::setPolygonCull(RenderState::CULL_NONE);
	RenderState::setDepthFunc(RenderState::DEPTH_NONE);
	RenderState::setViewport(static_cast<int>(draw_data->DisplayPos.x),
		static_cast<int>(draw_data->DisplayPos.y), static_cast<int>(draw_data->DisplaySize.x),
		static_cast<int>(draw_data->DisplaySize.y));

	// Orthographic projection matrix
	float left = draw_data->DisplayPos.x;
	float right = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float top = draw_data->DisplayPos.y;
	float bottom = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

	Math::mat4 proj;
	proj.m00 = 2.0f / (right - left);
	proj.m03 = (right + left) / (left - right);
	proj.m11 = 2.0f / (top - bottom);
	proj.m13 = (top + bottom) / (bottom - top);
	proj.m22 = 0.5f;
	proj.m23 = 0.5f;
	proj.m33 = 1.0f;

	Renderer::setProjection(proj);
	auto shader = material_->getShaderForce("imgui");
	auto pass = material_->getRenderPass("imgui");
	Renderer::setShaderParameters(pass, shader, material_, false);

	mesh_->bind();

	// Write vertex and index data into dynamic mesh
	mesh_->clearVertex();
	mesh_->clearIndices();
	mesh_->allocateVertex(draw_data->TotalVtxCount);
	mesh_->allocateIndices(draw_data->TotalIdxCount);
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[i];

		mesh_->addVertexArray(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size);
		mesh_->addIndicesArray(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size);
	}
	mesh_->flushVertex();
	mesh_->flushIndices();

	render_target->enable();
	{
		int global_idx_offset = 0;
		int global_vtx_offset = 0;
		ImVec2 clip_off = draw_data->DisplayPos;
		// Draw command lists
		for (int i = 0; i < draw_data->CmdListsCount; ++i)
		{
			const ImDrawList *cmd_list = draw_data->CmdLists[i];
			for (int j = 0; j < cmd_list->CmdBuffer.Size; ++j)
			{
				const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];

				if (cmd->UserCallback != nullptr)
				{
					if (cmd->UserCallback != ImDrawCallback_ResetRenderState)
					{
						cmd->UserCallback(cmd_list, cmd);
					}
				}
				else
				{
					float width = (cmd->ClipRect.z - cmd->ClipRect.x) / draw_data->DisplaySize.x;
					float height = (cmd->ClipRect.w - cmd->ClipRect.y) / draw_data->DisplaySize.y;
					float x = (cmd->ClipRect.x - clip_off.x) / draw_data->DisplaySize.x;
					float y = 1.0f - height
						- (cmd->ClipRect.y - clip_off.y) / draw_data->DisplaySize.y;

					RenderState::setScissorTest(x, y, width, height);
					RenderState::flushStates();

					auto texture = TexturePtr(reinterpret_cast<Texture *>(cmd->GetTexID()));
					material_->setTexture("imgui_texture", texture);

					mesh_->renderInstancedSurface(MeshDynamic::MODE_TRIANGLES,
						cmd->VtxOffset + global_vtx_offset, cmd->IdxOffset + global_idx_offset,
						cmd->IdxOffset + global_idx_offset + cmd->ElemCount, 1);
				}
			}
			global_vtx_offset += cmd_list->VtxBuffer.Size;
			global_idx_offset += cmd_list->IdxBuffer.Size;
		}

		RenderState::setScissorTest(0.0f, 0.0f, 1.0f, 1.0f);
	}
	render_target->disable();
	mesh_->unbind();

	RenderState::restoreState();

	render_target->unbindColorTexture(0);
	Render::releaseTemporaryRenderTarget(render_target);
}

void ImGuiBackend::create_font_texture()
{
	auto &io = ImGui::GetIO();
	unsigned char *pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	font_texture_ = Texture::create();
	font_texture_->create2D(width, height, Texture::FORMAT_RGBA8, Texture::SAMPLER_FILTER_LINEAR);

	auto blob = Blob::create();
	blob->setData(pixels, width * height * 32);
	font_texture_->setBlob(blob);
	blob->setData(nullptr, 0);

	io.Fonts->SetTexID(font_texture_.get());
}

void ImGuiBackend::create_mesh()
{
	mesh_ = MeshDynamic::create(MeshDynamic::USAGE_DYNAMIC_ALL);

	MeshDynamic::Attribute attributes[3]{};
	attributes[0].offset = 0;
	attributes[0].size = 2;
	attributes[0].type = MeshDynamic::TYPE_FLOAT;
	attributes[1].offset = 8;
	attributes[1].size = 2;
	attributes[1].type = MeshDynamic::TYPE_FLOAT;
	attributes[2].offset = 16;
	attributes[2].size = 4;
	attributes[2].type = MeshDynamic::TYPE_UCHAR;
	mesh_->setVertexFormat(attributes, 3);

	assert(mesh_->getVertexSize() == sizeof(ImDrawVert)
		&& "Vertex size of MeshDynamic is not equal to size of ImDrawVert");
}

void ImGuiBackend::create_material()
{
	material_ = Materials::findManualMaterial("imgui")->inherit();
}

void ImGuiBackend::init_keymap()
{
	keymap[Input::KEY_ESC] = ImGuiKey_Escape;
	keymap[Input::KEY_F1] = ImGuiKey_F1;
	keymap[Input::KEY_F2] = ImGuiKey_F2;
	keymap[Input::KEY_F3] = ImGuiKey_F3;
	keymap[Input::KEY_F4] = ImGuiKey_F4;
	keymap[Input::KEY_F5] = ImGuiKey_F5;
	keymap[Input::KEY_F6] = ImGuiKey_F6;
	keymap[Input::KEY_F7] = ImGuiKey_F7;
	keymap[Input::KEY_F8] = ImGuiKey_F8;
	keymap[Input::KEY_F9] = ImGuiKey_F9;
	keymap[Input::KEY_F10] = ImGuiKey_F10;
	keymap[Input::KEY_F11] = ImGuiKey_F11;
	keymap[Input::KEY_F12] = ImGuiKey_F12;
	keymap[Input::KEY_PRINTSCREEN] = ImGuiKey_None;
	keymap[Input::KEY_SCROLL_LOCK] = ImGuiKey_None;
	keymap[Input::KEY_PAUSE] = ImGuiKey_None;
	keymap[Input::KEY_BACK_QUOTE] = ImGuiKey_None;
	keymap[Input::KEY_DIGIT_1] = ImGuiKey_1;
	keymap[Input::KEY_DIGIT_2] = ImGuiKey_2;
	keymap[Input::KEY_DIGIT_3] = ImGuiKey_3;
	keymap[Input::KEY_DIGIT_4] = ImGuiKey_4;
	keymap[Input::KEY_DIGIT_5] = ImGuiKey_5;
	keymap[Input::KEY_DIGIT_6] = ImGuiKey_6;
	keymap[Input::KEY_DIGIT_7] = ImGuiKey_7;
	keymap[Input::KEY_DIGIT_8] = ImGuiKey_8;
	keymap[Input::KEY_DIGIT_9] = ImGuiKey_9;
	keymap[Input::KEY_DIGIT_0] = ImGuiKey_0;
	keymap[Input::KEY_MINUS] = ImGuiKey_Minus;
	keymap[Input::KEY_EQUALS] = ImGuiKey_Equal;
	keymap[Input::KEY_BACKSPACE] = ImGuiKey_Backspace;
	keymap[Input::KEY_TAB] = ImGuiKey_Tab;
	keymap[Input::KEY_Q] = ImGuiKey_Q;
	keymap[Input::KEY_W] = ImGuiKey_W;
	keymap[Input::KEY_E] = ImGuiKey_E;
	keymap[Input::KEY_R] = ImGuiKey_R;
	keymap[Input::KEY_T] = ImGuiKey_T;
	keymap[Input::KEY_Y] = ImGuiKey_Y;
	keymap[Input::KEY_U] = ImGuiKey_U;
	keymap[Input::KEY_I] = ImGuiKey_I;
	keymap[Input::KEY_O] = ImGuiKey_O;
	keymap[Input::KEY_P] = ImGuiKey_P;
	keymap[Input::KEY_LEFT_BRACKET] = ImGuiKey_LeftBracket;
	keymap[Input::KEY_RIGHT_BRACKET] = ImGuiKey_RightBracket;
	keymap[Input::KEY_ENTER] = ImGuiKey_Enter;
	keymap[Input::KEY_CAPS_LOCK] = ImGuiKey_CapsLock;
	keymap[Input::KEY_A] = ImGuiKey_A;
	keymap[Input::KEY_S] = ImGuiKey_S;
	keymap[Input::KEY_D] = ImGuiKey_D;
	keymap[Input::KEY_F] = ImGuiKey_F;
	keymap[Input::KEY_G] = ImGuiKey_G;
	keymap[Input::KEY_H] = ImGuiKey_H;
	keymap[Input::KEY_J] = ImGuiKey_J;
	keymap[Input::KEY_K] = ImGuiKey_K;
	keymap[Input::KEY_L] = ImGuiKey_L;
	keymap[Input::KEY_SEMICOLON] = ImGuiKey_Semicolon;
	keymap[Input::KEY_QUOTE] = ImGuiKey_None;
	keymap[Input::KEY_BACK_SLASH] = ImGuiKey_Backslash;
	keymap[Input::KEY_LEFT_SHIFT] = ImGuiKey_LeftShift;
	keymap[Input::KEY_LESS] = ImGuiKey_None;
	keymap[Input::KEY_Z] = ImGuiKey_Z;
	keymap[Input::KEY_X] = ImGuiKey_X;
	keymap[Input::KEY_C] = ImGuiKey_C;
	keymap[Input::KEY_V] = ImGuiKey_V;
	keymap[Input::KEY_B] = ImGuiKey_B;
	keymap[Input::KEY_N] = ImGuiKey_N;
	keymap[Input::KEY_M] = ImGuiKey_M;
	keymap[Input::KEY_COMMA] = ImGuiKey_Comma;
	keymap[Input::KEY_DOT] = ImGuiKey_Comma;
	keymap[Input::KEY_SLASH] = ImGuiKey_None;
	keymap[Input::KEY_RIGHT_SHIFT] = ImGuiKey_RightShift;
	keymap[Input::KEY_LEFT_CTRL] = ImGuiKey_LeftCtrl;
	keymap[Input::KEY_LEFT_CMD] = ImGuiKey_LeftSuper;
	keymap[Input::KEY_LEFT_ALT] = ImGuiKey_LeftAlt;
	keymap[Input::KEY_SPACE] = ImGuiKey_Space;
	keymap[Input::KEY_RIGHT_ALT] = ImGuiKey_RightAlt;
	keymap[Input::KEY_RIGHT_CMD] = ImGuiKey_RightSuper;
	keymap[Input::KEY_MENU] = ImGuiKey_None;
	keymap[Input::KEY_RIGHT_CTRL] = ImGuiKey_RightCtrl;
	keymap[Input::KEY_INSERT] = ImGuiKey_Insert;
	keymap[Input::KEY_DELETE] = ImGuiKey_Delete;
	keymap[Input::KEY_HOME] = ImGuiKey_Home;
	keymap[Input::KEY_END] = ImGuiKey_End;
	keymap[Input::KEY_PGUP] = ImGuiKey_PageUp;
	keymap[Input::KEY_PGDOWN] = ImGuiKey_PageDown;
	keymap[Input::KEY_UP] = ImGuiKey_UpArrow;
	keymap[Input::KEY_LEFT] = ImGuiKey_LeftArrow;
	keymap[Input::KEY_DOWN] = ImGuiKey_DownArrow;
	keymap[Input::KEY_RIGHT] = ImGuiKey_RightArrow;
	keymap[Input::KEY_NUM_LOCK] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIVIDE] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_MULTIPLY] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_MINUS] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_7] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_8] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_9] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_PLUS] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_4] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_5] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_6] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_1] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_2] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_3] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_ENTER] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DIGIT_0] = ImGuiKey_None;
	keymap[Input::KEY_NUMPAD_DOT] = ImGuiKey_None;
	keymap[Input::KEY_ANY_SHIFT] = ImGuiKey_None;
	keymap[Input::KEY_ANY_CTRL] = ImGuiKey_None;
	keymap[Input::KEY_ANY_ALT] = ImGuiKey_None;
	keymap[Input::KEY_ANY_CMD] = ImGuiKey_None;
	keymap[Input::KEY_ANY_UP] = ImGuiKey_None;
	keymap[Input::KEY_ANY_LEFT] = ImGuiKey_None;
	keymap[Input::KEY_ANY_DOWN] = ImGuiKey_None;
	keymap[Input::KEY_ANY_RIGHT] = ImGuiKey_None;
	keymap[Input::KEY_ANY_ENTER] = ImGuiKey_None;
	keymap[Input::KEY_ANY_DELETE] = ImGuiKey_None;
	keymap[Input::KEY_ANY_INSERT] = ImGuiKey_None;
	keymap[Input::KEY_ANY_HOME] = ImGuiKey_None;
	keymap[Input::KEY_ANY_END] = ImGuiKey_None;
	keymap[Input::KEY_ANY_PGUP] = ImGuiKey_None;
	keymap[Input::KEY_ANY_PGDOWN] = ImGuiKey_None;
}

void ImGuiBackend::init_theme()
{
	// set editor's color theme
	ImGui::StyleColorsDark();
	ImVec4 *colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.33f, 0.34f, 0.35f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.39f, 0.40f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.38f, 0.39f, 0.40f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.45f, 0.47f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.37f, 0.38f, 0.39f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.42f, 0.43f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.51f, 0.62f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.16f, 0.31f, 0.42f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.21f, 0.21f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.41f, 0.52f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.41f, 0.52f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.67f, 0.05f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
}

int ImGuiBackend::key_event(Unigine::Input::KEY key, bool down)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();
	io.AddKeyEvent(keymap[key], down);

	switch (keymap[key])
	{
	case ImGuiKey_LeftCtrl:
	case ImGuiKey_RightCtrl: io.AddKeyEvent(ImGuiMod_Ctrl, down); break;
	case ImGuiKey_LeftShift:
	case ImGuiKey_RightShift: io.AddKeyEvent(ImGuiMod_Shift, down); break;
	case ImGuiKey_LeftAlt:
	case ImGuiKey_RightAlt: io.AddKeyEvent(ImGuiMod_Alt, down); break;
	case ImGuiKey_LeftSuper:
	case ImGuiKey_RightSuper: io.AddKeyEvent(ImGuiMod_Super, down); break;
	default: break;
	}

	return 0;
}

int ImGuiBackend::key_pressed(Unigine::Input::KEY key)
{
	return key_event(key, true);
}

int ImGuiBackend::key_released(Unigine::Input::KEY key)
{
	return key_event(key, false);
}

int ImGuiBackend::button_event(Unigine::Input::MOUSE_BUTTON button, bool down)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();

	switch (button)
	{
	case Input::MOUSE_BUTTON_LEFT: io.AddMouseButtonEvent(ImGuiMouseButton_Left, down); break;
	case Input::MOUSE_BUTTON_RIGHT: io.AddMouseButtonEvent(ImGuiMouseButton_Right, down); break;
	case Input::MOUSE_BUTTON_MIDDLE: io.AddMouseButtonEvent(ImGuiMouseButton_Middle, down); break;
	default: break;
	}

	return 0;
}

int ImGuiBackend::button_pressed(Unigine::Input::MOUSE_BUTTON button)
{
	return button_event(button, true);
}

int ImGuiBackend::button_released(Unigine::Input::MOUSE_BUTTON button)
{
	return button_event(button, false);
}

int ImGuiBackend::unicode_key_pressed(unsigned int key)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();
	io.AddInputCharacter(key);

	return 0;
}

void ImGuiBackend::render_callback()
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();
	if (io.WantCaptureMouse)
	{
		Gui::getCurrent()->setMouseButtons(0);
	}
}
