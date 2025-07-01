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

	io.KeyMap[ImGuiKey_Tab] = Input::KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = Input::KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = Input::KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = Input::KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = Input::KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = Input::KEY_PGUP;
	io.KeyMap[ImGuiKey_PageDown] = Input::KEY_PGDOWN;
	io.KeyMap[ImGuiKey_Home] = Input::KEY_HOME;
	io.KeyMap[ImGuiKey_End] = Input::KEY_END;
	io.KeyMap[ImGuiKey_Insert] = Input::KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = Input::KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = Input::KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = Input::KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = Input::KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = Input::KEY_ESC;
	io.KeyMap[ImGuiKey_KeyPadEnter] = Input::KEY_ENTER;
	io.KeyMap[ImGuiKey_A] = Input::KEY_A;
	io.KeyMap[ImGuiKey_C] = Input::KEY_C;
	io.KeyMap[ImGuiKey_V] = Input::KEY_V;
	io.KeyMap[ImGuiKey_X] = Input::KEY_X;
	io.KeyMap[ImGuiKey_Y] = Input::KEY_Y;
	io.KeyMap[ImGuiKey_Z] = Input::KEY_Z;

	io.SetClipboardTextFn = set_clipboard_text;
	io.GetClipboardTextFn = get_clipboard_text;
	io.ClipboardUserData = nullptr;

	create_font_texture();
	create_mesh();
	create_material();

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

	io.KeyCtrl = Input::isKeyPressed(Input::KEY_ANY_CTRL);
	io.KeyShift = Input::isKeyPressed(Input::KEY_ANY_SHIFT);
	io.KeyAlt = Input::isKeyPressed(Input::KEY_ANY_ALT);
	io.KeySuper = Input::isKeyPressed(Input::KEY_ANY_CMD);

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

					auto txt = TexturePtr(static_cast<Texture *>(cmd->TextureId));
					material_->setTexture("imgui_texture", txt);

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
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();

	io.Fonts->AddFontFromFileTTF(
		FileSystem::getAbsolutePath("plugins/Unigine/ImGuiSamples/Roboto-Light.ttf"), 15.0f,
		nullptr, io.Fonts->GetGlyphRangesCyrillic());

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

	io.Fonts->TexID = font_texture_.get();
}

void ImGuiBackend::create_mesh()
{
	mesh_ = MeshDynamic::create(ObjectDynamic::DYNAMIC_ALL);

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
	MaterialPtr mat = Materials::findManualMaterial("imgui");

	if (mat.isNull())
	{
		Log::error("Cound't find imgui material\n");
		return;
	}

	material_ = mat->inherit();
}

int ImGuiBackend::key_pressed(Unigine::Input::KEY key)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();
	io.KeysDown[key] = true;
	return 0;
}

int ImGuiBackend::key_released(Unigine::Input::KEY key)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();
	io.KeysDown[key] = false;
	return 0;
}

int ImGuiBackend::button_pressed(Unigine::Input::MOUSE_BUTTON button)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();

	switch (button)
	{
	case Input::MOUSE_BUTTON_LEFT: io.MouseDown[0] = true; break;
	case Input::MOUSE_BUTTON_RIGHT: io.MouseDown[1] = true; break;
	case Input::MOUSE_BUTTON_MIDDLE: io.MouseDown[2] = true; break;
	case Input::MOUSE_BUTTON_UNKNOWN:
	case Input::MOUSE_BUTTON_DCLICK:
	case Input::MOUSE_BUTTON_AUX_0:
	case Input::MOUSE_BUTTON_AUX_1:
	case Input::MOUSE_BUTTON_AUX_2:
	case Input::MOUSE_BUTTON_AUX_3:
	case Input::MOUSE_NUM_BUTTONS: break;
	}

	return 0;
}

int ImGuiBackend::button_released(Unigine::Input::MOUSE_BUTTON button)
{
	ImGui::SetCurrentContext(ctx_);
	auto &io = ImGui::GetIO();

	switch (button)
	{
	case Input::MOUSE_BUTTON_LEFT: io.MouseDown[0] = false; break;
	case Input::MOUSE_BUTTON_RIGHT: io.MouseDown[1] = false; break;
	case Input::MOUSE_BUTTON_MIDDLE: io.MouseDown[2] = false; break;
	case Input::MOUSE_BUTTON_UNKNOWN:
	case Input::MOUSE_BUTTON_DCLICK:
	case Input::MOUSE_BUTTON_AUX_0:
	case Input::MOUSE_BUTTON_AUX_1:
	case Input::MOUSE_BUTTON_AUX_2:
	case Input::MOUSE_BUTTON_AUX_3:
	case Input::MOUSE_NUM_BUTTONS: break;
	}

	return 0;
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
