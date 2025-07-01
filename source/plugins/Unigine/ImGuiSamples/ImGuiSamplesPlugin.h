#pragma once

#include <UnigineLogic.h>
#include <UnigineVector.h>
#include <editor/UniginePlugin.h>

#include <memory>

class QAction;
class SamplesWindow;
class ISample;

class SamplesSystemLogic : public Unigine::SystemLogic
{
public:
	~SamplesSystemLogic() override;

	int update() override;
	void addSample(ISample *sample);

private:
	Unigine::Vector<ISample *> samples_;
};

class SamplesWorldLogic : public Unigine::WorldLogic
{
public:
	~SamplesWorldLogic() override;

	int init() override;
	int update() override;
	int shutdown() override;

	void addSample(ISample *sample);

private:
	Unigine::Vector<ISample *> samples_;
};

class ImGuiSamplesPlugin final
	: public QObject
	, public ::UnigineEditor::Plugin
{
	Q_OBJECT
	Q_DISABLE_COPY(ImGuiSamplesPlugin)
	Q_PLUGIN_METADATA(IID UNIGINE_EDITOR_PLUGIN_IID FILE "ImGuiSamplesPlugin.json")
	Q_INTERFACES(UnigineEditor::Plugin)

public:
	ImGuiSamplesPlugin();
	~ImGuiSamplesPlugin() override;

	bool init() override;
	void shutdown() override;

private:
	void init_samples();
	void shutdown_samples();

	void init_samples_window();
	void shutdown_samples_window();

	void init_samples_logics();
	void shutdown_samples_logics();

	void create_samples_window();

	SamplesWindow *samples_window_{};

	QAction *create_samples_window_action_{};

	std::unique_ptr<SamplesSystemLogic> samples_system_logic_;
	std::unique_ptr<SamplesWorldLogic> samples_world_logic_;

	Unigine::Vector<std::unique_ptr<ISample>> samples_;
};
