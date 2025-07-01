#include "SampleEditorImmediate.h"

#include <UnigineVisualizer.h>
#include <editor/UnigineObjectMode.h>
#include <editor/UnigineViewportManager.h>
#include <imgui/imgui.h>

using namespace Unigine;
using namespace Math;
using UnigineEditor::ViewportManager;
using UnigineEditor::ObjectMode;
using UnigineEditor::ShortcutManager;
using UnigineEditor::ShortcutContext;
using UnigineEditor::Shortcut;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// InputImmediate
////////////////////////////////////////////////////////////////////////////////////////////////////

InputImmediate::~InputImmediate()
{
	if (context_)
	{
		ShortcutManager::removeContext(context_->getID());
	}
}

void InputImmediate::grabInput(Input::KEY key)
{
	if (context_.isNull())
	{
		context_ = ShortcutManager::createContext("input_immediate_context", "Input Immediate",
			ShortcutContext::TYPE_SHARED, 1);

		for (unsigned int i = 0; i < Input::NUM_KEYS; ++i)
		{
			key_shortcuts_[i] = String::format("input_immediate_%d", i);
		}
	}

	auto shortcut = context_->getShortcut(key_shortcuts_[key]);

	if (shortcut.isNull())
	{
		shortcut = context_->createShortcut(key_shortcuts_[key]);
	}

	shortcut->setKey(key);
}

void InputImmediate::releaseInput(Input::KEY key)
{
	if (context_)
	{
		context_->removeShortcut(key_shortcuts_[key]);
	}
}

bool InputImmediate::isKeyDown(Input::KEY key)
{
	if (context_.isNull())
	{
		return false;
	}

	if (auto shortcut = context_->getShortcut(key_shortcuts_[key]))
	{
		return shortcut->isDown();
	}

	return false;
}

void InputImmediate::setActive(bool v)
{
	if (context_)
	{
		context_->setActive(v);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// LabelImmediate
////////////////////////////////////////////////////////////////////////////////////////////////////

void LabelImmediate::begin()
{
	if (!label_)
	{
		auto viewport = ViewportManager::getActiveViewportWindow();

		if (viewport == nullptr)
		{
			return;
		}

		GuiPtr gui = viewport->getGui();

		if (gui == nullptr)
		{
			return;
		}

		label_bg_ = WidgetVBox::create(gui);
		label_bg_->setPosition(10, 25);
		label_bg_->setBackground(1);
		label_bg_->setBackgroundColor(vec4(0, 0, 0, 0.5f));
		gui->addChild(label_bg_, Gui::ALIGN_OVERLAP);

		label_ = WidgetLabel::create(gui);
		label_->setPosition(20, 35);
		label_->setFontOutline(1);
		label_->setFontRich(1);
		gui->addChild(label_, Gui::ALIGN_OVERLAP);
	}

	label_bg_->setHidden(true);
	label_->setText("");
}

void LabelImmediate::end()
{
	if (!label_)
	{
		return;
	}

	label_->arrange();
	label_bg_->setWidth(label_->getWidth() + 20);
	label_bg_->setHeight(label_->getHeight() + 20);
}

void LabelImmediate::destroy()
{
	if (label_)
	{
		label_.deleteLater();
		label_bg_.deleteLater();
	}
}

void LabelImmediate::show(const char *format, ...)
{
	if (!label_)
	{
		return;
	}

	label_bg_->setHidden(false);

	StringStack<> ret;
	va_list argptr;
	va_start(argptr, format);
	ret.vprintf(format, argptr);
	va_end(argptr);

	label_->setText(String::format("%s%s<br>", label_->getText(), ret.get()));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// ManipulatorsImmediate
////////////////////////////////////////////////////////////////////////////////////////////////////

void ManipulatorsImmediate::begin()
{
	auto viewport = ViewportManager::getActiveViewportWindow();

	if (viewport == nullptr)
	{
		frame_data_ = FrameData{};
		return;
	}

	auto player = viewport->getPlayer();

	if (player)
	{
		auto camera = player->getCamera();
		frame_data_.modelview = camera->getModelview();
		frame_data_.projection = camera->getProjection();
	}

	frame_data_.gui = viewport->getGui();
	frame_data_.num_manipulators = 0;
	frame_data_.changed = false;
}

void ManipulatorsImmediate::end()
{
	// hide unused manipulators
	for (int i = frame_data_.num_manipulators; i < manipulators_.size(); ++i)
	{
		Manipulators &m = manipulators_[i];
		m.translate->setHidden(true);
		m.rotate->setHidden(true);
		m.scale->setHidden(true);
	}

	// disable rectangle selection if manipulator has been moving
	if (ViewportManager::getLastHoveredViewportWindow() != nullptr)
	{
		ViewportManager::setEnabledRectangleSelection(!frame_data_.changed);
	}
}

void ManipulatorsImmediate::destroy()
{
	// destroy widget manipulators
	for (int i = 0; i < manipulators_.size(); ++i)
	{
		Manipulators &m = manipulators_[i];
		m.translate.deleteLater();
		m.rotate.deleteLater();
		m.scale.deleteLater();
	}
	manipulators_.clear();
}

bool ManipulatorsImmediate::show(Mat4 &transform, Widget::TYPE type, const Mat4 &basis, int mask)
{
	bool changed = false;

	// create new manipulator
	if (manipulators_.size() <= frame_data_.num_manipulators)
	{
		Manipulators &m = manipulators_.append();
		m.translate = create<WidgetManipulatorTranslator>(transform, type, basis, mask);
		m.rotate = create<WidgetManipulatorRotator>(transform, type, basis, mask);
		m.scale = create<WidgetManipulatorScaler>(transform, type, basis, mask);
	}
	// change current manipulator
	else
	{
		Manipulators &m = manipulators_[frame_data_.num_manipulators];

		WidgetManipulatorPtr manipulator;
		if (type == Widget::WIDGET_MANIPULATOR_TRANSLATOR)
		{
			manipulator = m.translate;
			m.rotate->setHidden(true);
			m.scale->setHidden(true);
		}
		else if (type == Widget::WIDGET_MANIPULATOR_ROTATOR)
		{
			manipulator = m.rotate;
			m.translate->setHidden(true);
			m.scale->setHidden(true);
		}
		else if (type == Widget::WIDGET_MANIPULATOR_SCALER)
		{
			manipulator = m.scale;
			m.translate->setHidden(true);
			m.rotate->setHidden(true);
		}

		if (manipulator.isNull())
		{
			return changed;
		}

		manipulator->setBasis(basis);
		manipulator->setModelview(frame_data_.modelview);
		manipulator->setProjection(frame_data_.projection);
		manipulator->setMask(mask);

		bool type_changed = false;

		if (manipulator->isHidden())
		{
			type_changed = true;
			manipulator->setHidden(false);
		}

		// update position of the manipulator
		if (transform != manipulator->getTransform())
		{
			// check (user has been moving this manipulator?)
			if (!type_changed && manipulator->isFocusAxis())
			{
				changed = true;
				transform = manipulator->getTransform(); // return to user changed result
				frame_data_.changed = changed;
			}
			else
			{
				manipulator->setTransform(transform);
			}
		}
	}

	++frame_data_.num_manipulators;
	return changed;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
/// SampleEditorImmediate
////////////////////////////////////////////////////////////////////////////////////////////////////

void SampleEditorImmediate::init()
{
	points_.clear();
	points_.append(Vec3(-2, -2, 0));
	points_.append(Vec3(2, -2, 0));
	points_.append(Vec3(2, 2, 0));
	points_.append(Vec3(-2, 2, 0));
}

void SampleEditorImmediate::update()
{
	manipulators_immediate_.begin();
	label_immediate_.begin();

	// focus to polygon
	if (input_immediate_.isKeyDown(Input::KEY_F))
	{
		// some calculations...
		Vec3 center;
		for (auto &p : points_)
		{
			center += p;
		}
		center /= points_.size();

		Scalar bound = 0;
		for (int i = 0; i < points_.size(); ++i)
		{
			for (int j = 0; j < points_.size(); j++)
			{
				bound = Math::max(bound, distance(points_[i], points_[j]));
			}
		}

		// focus!
		if (auto viewport = ViewportManager::getActiveViewportWindow())
		{
			viewport->focusOnCenter(center, bound * 0.5f);
		}
	}


	// draw polygon
	Visualizer::setEnabled(true);
	for (int i = 0; i < points_.size(); ++i)
	{
		Visualizer::renderLine3D(points_[i - 1 < 0 ? points_.size() - 1 : i - 1], points_[i],
			vec4(0, 1, 0, 1));
		Visualizer::renderSolidSphere(0.1f, translate(points_[i]), vec4(1, 0, 0, 1));
	}

	// draw statistics
	label_immediate_.show("Points: %d", points_.size());
	for (int i = 0; i < points_.size(); ++i)
	{
		label_immediate_.show("%d) %.2f %.2f %.2f", i, points_[i].x, points_[i].y, points_[i].z);
	}

	// draw manipulators (Unigine::WidgetManipulatorTranslator instances)
	if (edit_mode_)
	{
		for (int i = 0; i < points_.size(); ++i)
		{
			// draw and move the point at the same time (imgui-like design)
			Mat4 t = translate(points_[i]);
			if (manipulators_immediate_.show(t))
			{
				points_[i] = t.getTranslate();
			}
		}
	}

	label_immediate_.end();
	manipulators_immediate_.end();
}

void SampleEditorImmediate::shutdown()
{
	label_immediate_.destroy();
	manipulators_immediate_.destroy();
	edit_mode_ = false;
	ObjectMode::setManipulatorsEnabled(true);
	UnigineEditor::ViewportManager::setEnabledRectangleSelection(true);
	UnigineEditor::ObjectMode::setManipulatorsEnabled(true);
	input_immediate_.releaseInput(Input::KEY_F);
}

void SampleEditorImmediate::updateGui()
{
	ImGui::TextWrapped(
		"This sample demonstrates how to work with the Editor and Engine's widgets in immediate "
		"mode (in ImGui-like style) using the EditorImmediate class.\n"
		"Programming in this mode is simple, but it has a number of limitations. It is recommended "
		"to use this mode for prototyping or simple plugins.\n"
		"\n"
		"At the center of the world there are four connected points that form a polygon.\n"
		"Click on \"Edit Mode\" to show manipulators for each point.\n"
		"In edit mode, you can press the F button to focus on the polygon.\n");

	if (ImGui::Checkbox("Edit Mode", &edit_mode_))
	{
		// InputImmediate::grabInput() disables Editor's hotkeys
		// and allow us to use InputImmediate::isKeyDown() method
		if (edit_mode_)
		{
			input_immediate_.grabInput(Input::KEY_F);
		}
		else
		{
			input_immediate_.releaseInput(Input::KEY_F);
		}

		ViewportManager::setEnabledRectangleSelection(!edit_mode_);
		ObjectMode::setManipulatorsEnabled(!edit_mode_);
		input_immediate_.setActive(edit_mode_);
	}
}

void SampleEditorImmediate::setEnabled(bool v)
{
	if (enabled_ == v)
	{
		return;
	}

	enabled_ = v;

	if (enabled_ == false)
	{
		label_immediate_.destroy();
		manipulators_immediate_.destroy();
		input_immediate_.setActive(false);
		UnigineEditor::ViewportManager::setEnabledRectangleSelection(true);
		UnigineEditor::ObjectMode::setManipulatorsEnabled(true);
	}
	else
	{
		UnigineEditor::ViewportManager::setEnabledRectangleSelection(!edit_mode_);
		UnigineEditor::ObjectMode::setManipulatorsEnabled(!edit_mode_);
		input_immediate_.setActive(edit_mode_);
	}
}
