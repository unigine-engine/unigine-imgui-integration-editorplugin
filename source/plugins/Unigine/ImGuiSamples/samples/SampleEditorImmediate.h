#pragma once
#include "ISample.h"

#include <UnigineMathLib.h>
#include <UnigineVector.h>
#include <UnigineWidgets.h>
#include <editor/UnigineShortcutManager.h>

class InputImmediate final
{
public:
	~InputImmediate();
	void grabInput(Unigine::Input::KEY key);
	void releaseInput(Unigine::Input::KEY key);
	bool isKeyDown(Unigine::Input::KEY key);
	void setActive(bool v);

private:
	UnigineEditor::ShortcutContextPtr context_;
	Unigine::String key_shortcuts_[Unigine::Input::NUM_KEYS]{};
};

class LabelImmediate final
{
public:
	void begin();
	void end();
	void destroy();
	void show(const char *format, ...);

private:
	Unigine::WidgetLabelPtr label_;
	Unigine::WidgetVBoxPtr label_bg_;
};

class ManipulatorsImmediate final
{
public:
	void begin();
	void end();
	void destroy();
	bool show(Unigine::Math::Mat4 &transform,
		Unigine::Widget::TYPE type = Unigine::Widget::WIDGET_MANIPULATOR_TRANSLATOR,
		const Unigine::Math::Mat4 &basis = Unigine::Math::Mat4_identity,
		int mask = Unigine::WidgetManipulator::MASK_XYZ);

private:
	template<typename T>
	Unigine::Ptr<T> create(Unigine::Math::Mat4 &transform, Unigine::Widget::TYPE show_type,
		const Unigine::Math::Mat4 &basis, int mask)
	{
		auto manipulator = T::create(frame_data_.gui);

		if (show_type != manipulator->getType())
		{
			manipulator->setHidden(true);
		}

		manipulator->setBasis(basis);
		manipulator->setModelview(frame_data_.modelview);
		manipulator->setProjection(frame_data_.projection);
		manipulator->setTransform(transform);
		manipulator->setRenderGui(frame_data_.gui);
		manipulator->setSize(96);
		manipulator->setMask(mask);
		frame_data_.gui->addChild(manipulator, Unigine::Gui::ALIGN_OVERLAP);

		return manipulator;
	}

	struct FrameData
	{
		Unigine::GuiPtr gui;
		Unigine::Math::Mat4 modelview;
		Unigine::Math::mat4 projection;

		int num_manipulators{0};
		bool changed{false};
	};

	struct Manipulators
	{
		Unigine::WidgetManipulatorTranslatorPtr translate;
		Unigine::WidgetManipulatorRotatorPtr rotate;
		Unigine::WidgetManipulatorScalerPtr scale;
	};

	FrameData frame_data_;
	Unigine::Vector<Manipulators> manipulators_;
};

class SampleEditorImmediate : public ISample
{
public:
	const char *getName() override { return "Editor (Immediate Mode)"; }

	void init() override;
	void update() override;
	void shutdown() override;
	void updateGui() override;

	void setEnabled(bool v) override;
	bool isEnabled() const override { return enabled_; }

private:
	InputImmediate input_immediate_;
	LabelImmediate label_immediate_;
	ManipulatorsImmediate manipulators_immediate_;

	bool enabled_{false};
	bool edit_mode_{false};
	Unigine::Vector<Unigine::Math::Vec3> points_;
};
