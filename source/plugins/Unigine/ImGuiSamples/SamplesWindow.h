#pragma once

#include "ImGuiBackend.h"

#include <UnigineWidgets.h>
#include <editor/UnigineEngineGuiWindow.h>
#include <editor/UnigineUndo.h>

class ISample;

class SamplesWindow final : public UnigineEditor::EngineGuiWindow
{
	Q_OBJECT

public:
	SamplesWindow(QWidget *parent = nullptr);
	~SamplesWindow() override;

	void addSample(ISample *sample);

	void setCurrentSample(ISample *sample);

signals:
	void onCurrentSampleChanged(ISample *sample);

protected:
	void onUpdate() override;
	void onRender() override;

private:
	bool change_tab_request(ISample *sample, bool from_ui);
	void change_tab();

	struct
	{
		ISample *sample{};
		bool from_ui{false};
	} change_tab_request_;

	ISample *current_sample_{};

	ImGuiBackend imgui_backend_;

	Unigine::Vector<ISample *> samples_;
	Unigine::TexturePtr texture_;
	Unigine::WidgetSpritePtr sprite_;
};

class ChangeTabAction : public UnigineEditor::Action
{
public:
	ChangeTabAction(SamplesWindow &window, ISample *prev, ISample *next)
		: window_(window)
		, prev_(prev)
		, next_(next)
	{}

	void apply() override {}
	void undo() override { window_.setCurrentSample(prev_); }
	void redo() override { window_.setCurrentSample(next_); }

private:
	SamplesWindow &window_;
	ISample *prev_;
	ISample *next_;
};
