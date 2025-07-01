#include "SamplesWindow.h"

#include "ISample.h"
#include "imgui/imgui.h"

#include <UnigineWindowManager.h>

using namespace Unigine;
using UnigineEditor::Undo;

SamplesWindow::SamplesWindow(QWidget *parent)
	: UnigineEditor::EngineGuiWindow(parent)
{
	const auto gui = getGui();

	// sprite to show UI
	sprite_ = WidgetSprite::create("white.texture");
	sprite_->setOrder(128); // fix for UIDesigner order
	gui->addChild(sprite_, Gui::ALIGN_OVERLAP);

	texture_ = Texture::create();
	sprite_->setRender(texture_);

	// init ImGui backend
	imgui_backend_.init();

	removeShortcutsExclusiveContext();
}

SamplesWindow::~SamplesWindow()
{
	// shutdown ImGui backend
	imgui_backend_.shutdown();
}

void SamplesWindow::addSample(ISample *sample)
{
	if (samples_.contains(sample) == false)
	{
		samples_.append(sample);
	}
}

void SamplesWindow::setCurrentSample(ISample *sample)
{
	change_tab_request(sample, false);
}

void SamplesWindow::onUpdate()
{
	EngineGuiWindow::onUpdate();

	if (samples_.empty())
	{
		return;
	}

	const auto gui = getGui();

	sprite_->setPosition(0, 0);
	sprite_->setWidth(gui->getWidth());
	sprite_->setHeight(gui->getHeight());

	if (sprite_->getWidth() != texture_->getWidth()
		|| sprite_->getHeight() != texture_->getHeight())
	{
		// resize texture
		texture_->create2D(sprite_->getWidth(), sprite_->getHeight(), Texture::FORMAT_RGBA8,
			Texture::FORMAT_USAGE_RENDER);
	}

	Math::ivec2 context_pos;
	Math::ivec2 context_size;

	EngineWindowPtr main_window = WindowManager::getMainWindow();

	if (main_window)
	{
		const float dpi_scale = main_window->getDpiScale();
		context_size = main_window->getClientSize();
		context_size = Math::ivec2(Math::vec2(context_size) * dpi_scale);
		context_pos = main_window->getClientPosition();
	}
	else
	{
		context_pos = gui->getPosition();
		context_size = gui->getSize();
	}

	imgui_backend_.newFrame(context_pos, context_size);

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(gui->getWidth(), gui->getHeight()), ImGuiCond_Always);
	ImGui::Begin("MainWindow", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);

	if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_None))
	{
		for (ISample *sample : samples_)
		{
			if (ImGui::BeginTabItem(sample->getName()))
			{
				change_tab_request(sample, true);
				sample->updateGui();
				ImGui::EndTabItem();
			}
		}

		change_tab();
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void SamplesWindow::onRender()
{
	if (samples_.empty() == false)
	{
		imgui_backend_.render(texture_);
	}

	EngineGuiWindow::onRender();
}

bool SamplesWindow::change_tab_request(ISample *sample, bool from_ui)
{
	if (change_tab_request_.sample || sample == current_sample_)
	{
		return false;
	}

	change_tab_request_.sample = sample;
	change_tab_request_.from_ui = from_ui;
	return true;
}

void SamplesWindow::change_tab()
{
	if (change_tab_request_.sample)
	{
		if (change_tab_request_.from_ui)
		{
			Undo::instance()->push(
				new ChangeTabAction(*this, current_sample_, change_tab_request_.sample));
		}
		else
		{
			if (ImGui::BeginTabItem(change_tab_request_.sample->getName(), nullptr,
					ImGuiTabItemFlags_SetSelected))
			{
				ImGui::EndTabItem();
			}
		}

		current_sample_ = change_tab_request_.sample;
		change_tab_request_.sample = nullptr;
		emit onCurrentSampleChanged(current_sample_);
	}
}
