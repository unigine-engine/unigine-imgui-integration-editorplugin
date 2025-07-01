#include "ImGuiSamplesPlugin.h"

#include "ISample.h"
#include "SamplesWindow.h"
#include "samples/SampleComponents.h"
#include "samples/SampleEditorImmediate.h"
#include "samples/SampleSplineEditor.h"

#include <editor/UnigineConstants.h>
#include <editor/UnigineEngineGuiWindow.h>
#include <editor/UnigineWindowManager.h>
#include <UnigineWorld.h>

#include <QMenu>

namespace
{

const char *PLUGIN_TITLE = "Plugin - ImGuiSamples";

} // anonymous namespace

using UnigineEditor::WindowManager;

SamplesSystemLogic::~SamplesSystemLogic() = default;

int SamplesSystemLogic::update()
{
	for (ISample *sample : samples_)
	{
		if (sample->isEnabled())
		{
			sample->update();
		}
	}
	return 1;
}

void SamplesSystemLogic::addSample(ISample *sample)
{
	if (samples_.contains(sample) == false)
	{
		samples_.append(sample);
	}
}


SamplesWorldLogic::~SamplesWorldLogic() = default;

int SamplesWorldLogic::init()
{
	for (ISample *sample : samples_)
	{
		sample->worldInit();
	}
	return 1;
}

int SamplesWorldLogic::update()
{
	for (ISample *sample : samples_)
	{
		if (sample->isEnabled())
		{
			sample->worldUpdate();
		}
	}
	return 1;
}

int SamplesWorldLogic::shutdown()
{
	for (ISample *sample : samples_)
	{
		sample->worldShutdown();
	}
	return 1;
}

void SamplesWorldLogic::addSample(ISample *sample)
{
	if (samples_.contains(sample) == false)
	{
		samples_.append(sample);
	}
}

ImGuiSamplesPlugin::ImGuiSamplesPlugin() = default;
ImGuiSamplesPlugin::~ImGuiSamplesPlugin() = default;

bool ImGuiSamplesPlugin::init()
{
	init_samples();
	init_samples_logics();
	init_samples_window();
	return true;
}

void ImGuiSamplesPlugin::shutdown()
{
	shutdown_samples_window();
	shutdown_samples_logics();
	shutdown_samples();
}

void ImGuiSamplesPlugin::init_samples()
{
	samples_.append(std::make_unique<SampleSplineEditor>());
	samples_.append(std::make_unique<SampleEditorImmediate>());
	samples_.append(std::make_unique<SampleComponents>());
}

void ImGuiSamplesPlugin::shutdown_samples()
{
	samples_.clear();
}

void ImGuiSamplesPlugin::init_samples_window()
{
	QMenu *menu_windows = WindowManager::findMenu(UnigineEditor::Constants::MM_WINDOWS);

	create_samples_window_action_ = menu_windows->addAction(PLUGIN_TITLE, this,
		&ImGuiSamplesPlugin::create_samples_window);
}

void ImGuiSamplesPlugin::shutdown_samples_window()
{
	QMenu *menu_windows = WindowManager::findMenu(UnigineEditor::Constants::MM_WINDOWS);
	menu_windows->removeAction(create_samples_window_action_);
	create_samples_window_action_ = nullptr;

	if (samples_window_)
	{
		WindowManager::remove(samples_window_);

		delete samples_window_;
		samples_window_ = nullptr;
	}
}

void ImGuiSamplesPlugin::init_samples_logics()
{
	samples_system_logic_ = std::make_unique<SamplesSystemLogic>();
	Unigine::Engine::get()->addSystemLogic(samples_system_logic_.get());
	samples_world_logic_ = std::make_unique<SamplesWorldLogic>();
	Unigine::Engine::get()->addWorldLogic(samples_world_logic_.get());

	for (const auto &sample : samples_)
	{
		samples_system_logic_->addSample(sample.get());
		sample->init();

		samples_world_logic_->addSample(sample.get());
		if (Unigine::World::isLoaded())
		{
			sample->worldInit();
		}
	}
}

void ImGuiSamplesPlugin::shutdown_samples_logics()
{
	Unigine::Engine::get()->removeSystemLogic(samples_system_logic_.get());
	samples_system_logic_.reset();
	Unigine::Engine::get()->removeWorldLogic(samples_world_logic_.get());
	samples_world_logic_.reset();

	for (const auto &sample : samples_)
	{
		if (Unigine::World::isLoaded())
		{
			sample->worldShutdown();
		}

		sample->shutdown();
	}
}

void ImGuiSamplesPlugin::create_samples_window()
{
	if (samples_window_)
	{
		if (WindowManager::isHidden(samples_window_))
		{
			WindowManager::show(samples_window_);
		}
		return;
	}

	samples_window_ = new SamplesWindow;
	samples_window_->setWindowTitle(PLUGIN_TITLE);
	for (const auto &sample : samples_)
	{
		samples_window_->addSample(sample.get());
	}

	WindowManager::add(samples_window_, WindowManager::NEW_FLOATING_AREA);

	connect(samples_window_, &QObject::destroyed, this, [this]() { samples_window_ = nullptr; });

	connect(samples_window_, &SamplesWindow::onCurrentSampleChanged, this, [this](ISample *sample) {
		for (const auto &s : samples_)
		{
			s->setEnabled(s.get() == sample);
		}
	});

	WindowManager::show(samples_window_);
}
