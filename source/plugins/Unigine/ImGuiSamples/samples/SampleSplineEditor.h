#pragma once
#include "ISample.h"

#include <UnigineMathLib.h>
#include <UniginePlayers.h>
#include <UnigineVector.h>
#include <UnigineWidgets.h>
#include <editor/UnigineShortcutManager.h>
#include <editor/UnigineUndo.h>

class SampleSplineEditor final : public ISample
{
public:
	const char *getName() override { return "Spline Editor"; }

	void init() override;
	void shutdown() override;

	void worldInit() override;
	void worldUpdate() override;
	void worldShutdown() override;

	void updateGui() override;

	void setEnabled(bool v) override;
	bool isEnabled() const override { return enabled_; }

	const Unigine::Vector<Unigine::Math::Vec3> &getPoints() const { return points_; }
	void setPoints(const Unigine::Vector<Unigine::Math::Vec3> &v) { points_ = v; }

	const Unigine::Vector<int> &getSelected() const { return selected_; }
	void setSelected(const Unigine::Vector<int> &v) { selected_ = v; }

private:
	void draw_spline(int subdiv = 10);
	void canvas_update();
	void update_selected_point();
	void update_selection();
	void focus_selected_points();
	bool get_screen_position(int &x, int &y, const Unigine::Math::Vec3 &world_point,
		const Unigine::PlayerPtr &player, const Unigine::Math::ivec2 &screen_size);
	bool get_point_under_mouse_cursor(Unigine::Math::Vec3 &out_point);
	void clear();
	void update_description();

	bool edit_mode_{false};
	bool enabled_{false};

	Unigine::String description_text_;

	Unigine::Vector<Unigine::Math::Vec3> points_;
	Unigine::HashMap<int, int> id_to_index_;
	Unigine::Vector<int> selected_; // indices

	Unigine::WidgetCanvasPtr canvas_;
	float point_size_{20};
	Unigine::WidgetManipulatorTranslatorPtr manipulator_;

	UnigineEditor::ShortcutContextPtr context_;
	UnigineEditor::ShortcutPtr sh_select_;
	UnigineEditor::ShortcutPtr sh_add_or_extend_;
	UnigineEditor::ShortcutPtr sh_delete_;
	UnigineEditor::ShortcutPtr sh_focus_;

	Unigine::EventConnections connections_;
};

// Note: the undo editor stack is cleared when the world is reloaded.
class CommandSaveState : public UnigineEditor::Action
{
public:
	CommandSaveState(SampleSplineEditor &editor,
		const Unigine::Vector<Unigine::Math::Vec3> &prev_points,
		const Unigine::Vector<int> &prev_selected);

	void apply() override;
	void undo() override;
	void redo() override;

private:
	SampleSplineEditor &editor_;
	Unigine::Vector<Unigine::Math::Vec3> prev_points_;
	Unigine::Vector<int> prev_selected_;
	Unigine::Vector<Unigine::Math::Vec3> now_points_;
	Unigine::Vector<int> now_selected_;
};

class SaveStateToUndoStack
{
public:
	SaveStateToUndoStack(SampleSplineEditor &editor);
	~SaveStateToUndoStack();

private:
	SampleSplineEditor &editor_;
	Unigine::VectorStack<Unigine::Math::Vec3> prev_points_;
	Unigine::VectorStack<int> prev_selected_;
};
