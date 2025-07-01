#include "SampleSplineEditor.h"

#include <UnigineVisualizer.h>
#include <UnigineWorld.h>
#include <editor/UnigineObjectMode.h>
#include <editor/UnigineViewportManager.h>
#include <imgui/imgui.h>
#include <UnigineWorld.h>
#include <UnigineEngine.h>

using namespace Unigine;
using namespace Math;
using UnigineEditor::ShortcutManager;
using UnigineEditor::ShortcutContext;
using UnigineEditor::Shortcut;
using UnigineEditor::ViewportManager;
using UnigineEditor::Undo;

namespace
{
constexpr char *CONTEXT_SPLINE_EDITOR = "spline_editor";
constexpr char *SHORTCUT_SELECT = "spline_select";
constexpr char *SHORTCUT_ADD_OR_EXTEND = "spline_add_or_select";
constexpr char *SHORTCUT_DELETE = "spline_delete";
constexpr char *SHORTCUT_FOCUS = "spline_focus";
constexpr char *DESCRIPTION_TMPL_
	= "This sample shows how to use the Editor API if you are creating a complex "
		"plugin that uses Editor's Undo/Redo, Hotkeys, custom visualization...\n"
		"More code, but at the same time more flexibility.\n"
		"\n"
		"Hotkeys:\n"
		"  %s - Select point\n"
		"  %s - Create new point to the end of spline or add/remove point from selection\n"
		"  %s - Remove point\n"
		"  %s - Focus on selected point\n"
		"  %s - Undo\n"
		"  %s - Redo\n";

} // anonymous namespace

void SampleSplineEditor::init()
{
	context_ = ShortcutManager::createContext(CONTEXT_SPLINE_EDITOR, "Spline Editor",
		ShortcutContext::TYPE_SHARED, 2);

	sh_select_ = context_->createShortcut(SHORTCUT_SELECT);
	sh_select_->setTitle("Select point");
	sh_select_->setMouseButtonMask(Shortcut::MOUSE_MASK_LEFT);
	sh_select_->setTransparent(true);

	sh_add_or_extend_ = context_->createShortcut(SHORTCUT_ADD_OR_EXTEND);
	sh_add_or_extend_->setTitle("Add point or extend/exclude selection");
	sh_add_or_extend_->setMouseButtonMask(Shortcut::MOUSE_MASK_LEFT);
	sh_add_or_extend_->setModifierMask(Shortcut::MODIFIER_MASK_SHIFT);
	sh_add_or_extend_->setTransparent(true);

	sh_delete_ = context_->createShortcut(SHORTCUT_DELETE);
	sh_delete_->setTitle("Delete point");
	sh_delete_->setKey(Input::KEY_DELETE);

	sh_focus_ = context_->createShortcut(SHORTCUT_FOCUS);
	sh_focus_->setTitle("Focus the camera on the selected point(s)");
	sh_focus_->setKey(Input::KEY_F);

	canvas_ = WidgetCanvas::create();
}

void SampleSplineEditor::shutdown()
{
	clear();
	canvas_.deleteLater();

	ShortcutManager::removeContext(CONTEXT_SPLINE_EDITOR);

	edit_mode_ = false;
	UnigineEditor::ViewportManager::setEnabledRectangleSelection(true);
	UnigineEditor::ObjectMode::setManipulatorsEnabled(true);
}

void SampleSplineEditor::worldInit()
{
	points_.clear();
	points_.append(Vec3(-2, 0, 0));
	points_.append(Vec3(0, -2, 0));
	points_.append(Vec3(2, 0, 0));
}

void SampleSplineEditor::worldUpdate()
{
	if (edit_mode_)
	{
		update_selection();
		update_selected_point();
		canvas_update(); // draw handles
	}
	else
	{
		canvas_->clear(); // remove handles
	}

	draw_spline();
}

void SampleSplineEditor::worldShutdown()
{
	clear();
}

void SampleSplineEditor::updateGui()
{
	ImGui::TextWrapped("%s", description_text_.get());
	if (ImGui::Checkbox("Edit Mode", &edit_mode_))
	{
		UnigineEditor::ViewportManager::setEnabledRectangleSelection(!edit_mode_);
		UnigineEditor::ObjectMode::setManipulatorsEnabled(!edit_mode_);
		context_->setActive(edit_mode_);
	}
}

void SampleSplineEditor::setEnabled(bool v)
{
	if (enabled_ == v)
	{
		return;
	}

	enabled_ = v;

	UnigineEditor::ViewportManager::setEnabledRectangleSelection(!edit_mode_);
	UnigineEditor::ObjectMode::setManipulatorsEnabled(!edit_mode_);

	if (enabled_ == false)
	{
		context_->setActive(false);

		canvas_->clear();
		UnigineEditor::ViewportManager::setEnabledRectangleSelection(true);
		UnigineEditor::ObjectMode::setManipulatorsEnabled(true);

		manipulator_.deleteLater();
		description_text_.clear();
		connections_.disconnectAll();
	}
	else
	{
		context_->setActive(edit_mode_);
		update_description();
		ShortcutManager::getEventKeyboardLayoutChanged().connect(connections_, this,
			&SampleSplineEditor::update_description);
	}
}

void SampleSplineEditor::draw_spline(int subdiv)
{
	auto get_bezier_point = [](Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float k0) -> Vec3 {
		float k1 = 1.0f - k0;
		float k00 = k0 * k0;
		float k11 = k1 * k1;
		return p0 * (k11 * k1) + p1 * (3.0f * k11 * k0) + p2 * (3.0f * k00 * k1) + p3 * (k00 * k0);
	};

	auto get_auto_handles = [](Vec3 prev, Vec3 current, Vec3 next, Vec3 &left, Vec3 &right) {
		left = (prev - current) / 3.0f;
		right = (next - current) / 3.0f;
		left = (left - right) / 2.0f;
		right = -left;
	};

	Visualizer::setEnabled(true);
	for (int i = 0; i < points_.size(); ++i)
	{
		Vec3 prev_point = points_[max(0, i - 1)];
		Vec3 cur_point = points_[i];
		Vec3 next_point = points_[min(points_.size() - 1, i + 1)];
		Vec3 next_next_point = points_[min(points_.size() - 1, i + 2)];

		Vec3 tmp, handle_right, handle_left;
		get_auto_handles(prev_point, cur_point, next_point, tmp, handle_right);
		get_auto_handles(cur_point, next_point, next_next_point, handle_left, tmp);

		Vec3 last_p = cur_point;
		for (int z = 0; z <= subdiv; z++)
		{
			float t = float(z + 1) / (subdiv + 1);
			Vec3 p = get_bezier_point(cur_point, cur_point + handle_right, handle_left + next_point,
				next_point, t);

			// show piece of curve
			Visualizer::renderLine3D(last_p, p, vec4(1, 1, 1, 1));

			last_p = p;
		}
	}
}

void SampleSplineEditor::canvas_update()
{
	canvas_->clear();
	id_to_index_.clear();

	auto viewport = ViewportManager::getActiveViewportWindow();

	if (viewport == nullptr)
	{
		return;
	}

	GuiPtr gui = viewport->getGui();
	PlayerPtr player = viewport->getPlayer();
	if (!gui || !player)
	{
		return;
	}

	if (canvas_->getParentGui() != gui)
	{
		gui->addChild(canvas_, Gui::ALIGN_OVERLAP | Gui::ALIGN_EXPAND);
	}

	ivec2 screen_size = ivec2(gui->getWidth(), gui->getHeight());
	float ish = point_size_ * 0.5f;

	for (int i = 0; i < points_.size(); ++i)
	{
		int id = canvas_->addPolygon();
		int x, y;
		if (get_screen_position(x, y, points_[i], player, screen_size))
		{
			canvas_->addPolygonPoint(id, vec3(x - ish, y + ish, 0));
			canvas_->addPolygonPoint(id, vec3(x - ish, y - ish, 0));
			canvas_->addPolygonPoint(id, vec3(x + ish, y - ish, 0));
			canvas_->addPolygonPoint(id, vec3(x + ish, y + ish, 0));
			canvas_->addPolygonIndex(id, 0);
			canvas_->addPolygonIndex(id, 1);
			canvas_->addPolygonIndex(id, 2);
			canvas_->addPolygonIndex(id, 2);
			canvas_->addPolygonIndex(id, 3);
			canvas_->addPolygonIndex(id, 0);
			if (selected_.contains(i))
			{
				canvas_->setPolygonColor(id, vec4(1.0f, 1.0f, 1.0f, 1)); // white
			}
			else
			{
				canvas_->setPolygonColor(id, vec4(1.0f, 0.8f, 0.2f, 1)); // yellow
			}
			id_to_index_.append(id, i);
		}
	}
}

void SampleSplineEditor::update_selected_point()
{
	auto viewport = ViewportManager::getLastHoveredViewportWindow();

	if (selected_.empty() || viewport == nullptr)
	{
		manipulator_.deleteLater();
		return;
	}

	Mat4 transform = translate(points_[selected_.last()]);
	auto camera = viewport->getPlayer()->getCamera();

	if (manipulator_.isNull())
	{
		auto gui = viewport->getGui();
		manipulator_ = WidgetManipulatorTranslator::create(gui);

		manipulator_->setBasis(Unigine::Math::Mat4_identity);
		manipulator_->setModelview(camera->getModelview());
		manipulator_->setProjection(camera->getProjection());
		manipulator_->setTransform(transform);
		manipulator_->setRenderGui(gui);
		manipulator_->setSize(96);
		manipulator_->setMask(Unigine::WidgetManipulator::MASK_XYZ);
		gui->addChild(manipulator_, Gui::ALIGN_OVERLAP);
	}

	manipulator_->setModelview(camera->getModelview());
	manipulator_->setProjection(camera->getProjection());

	if (manipulator_->isFocusAxis())
	{
		const Vec3 delta = manipulator_->getTransform().getTranslate() - points_[selected_.last()];
		for (auto &i : selected_)
		{
			points_[i] += delta;
		}
	}
	else
	{
		manipulator_->setTransform(transform);
	}

	// controls
	if (sh_focus_->isDown())
	{
		focus_selected_points();
	}

	if (sh_delete_->isDown())
	{
		SaveStateToUndoStack state(*this);

		// inverse, from lower to higher
		quickSort(selected_.begin(), selected_.end(), [](int &l, int &r) -> int { return l < r; });

		for (int i = selected_.size() - 1; i >= 0; --i)
		{
			points_.remove(selected_[i]);
			selected_.removeFast(i);
		}
	}
}

void SampleSplineEditor::update_selection()
{
	if (canvas_.isNull() || (manipulator_ && manipulator_->isHoverAxis()))
	{
		return;
	}

	const int id = canvas_->getPolygonIntersection(canvas_->getMouseX(), canvas_->getMouseY());
	if (id == -1)
	{
		// clear selection if user clicks on empty space
		if (sh_select_->isDown())
		{
			SaveStateToUndoStack state(*this);
			selected_.clear();
		}
		else if (sh_add_or_extend_->isDown())
		{
			// create new point under mouse cursor!
			Vec3 point_pos;
			if (get_point_under_mouse_cursor(point_pos))
			{
				SaveStateToUndoStack state(*this);
				points_.append(point_pos);
				selected_.clear();
			}
		}
		return;
	}

	// highlight point under mouse cursor
	canvas_->setPolygonColor(id, vec4(1, 1, 1, 1)); // white

	if (sh_select_->isDown())
	{
		SaveStateToUndoStack state(*this);
		selected_.clear();
		selected_.append(id_to_index_[id]); // select point
	}
	else if (sh_add_or_extend_->isDown())
	{
		SaveStateToUndoStack state(*this);
		int index = id_to_index_[id];
		if (selected_.contains(index))
		{
			selected_.removeOne(index); // exclude selection
		}
		else
		{
			selected_.append(index); // extend selection
		}
	}
}

void SampleSplineEditor::focus_selected_points()
{
	if (selected_.empty())
	{
		return;
	}

	Vec3 center;
	for (auto &p : selected_)
	{
		center += points_[p];
	}
	center /= selected_.size();

	Scalar bound = 0;
	for (int i = 0; i < selected_.size(); ++i)
	{
		for (int j = 0; j < selected_.size(); j++)
		{
			bound = Math::max(bound, distance(points_[selected_[i]], points_[selected_[j]]));
		}
	}

	if (auto viewport = ViewportManager::getActiveViewportWindow())
	{
		viewport->focusOnCenter(center, Math::max(Scalar(1.0f), bound * 0.5f));
	}
}

// return true if world_point in front of the player, otherwise false
bool SampleSplineEditor::get_screen_position(int &x, int &y, const Vec3 &world_point,
	const PlayerPtr &player, const ivec2 &screen_size)
{
	const float width = itof(screen_size.x);
	const float height = itof(screen_size.y);

	mat4 projection = player->getProjection();
	const mat4 modelview = mat4(player->getCamera()->getModelview());
	projection.m00 *= height / width;

	Vec4 p = projection * Vec4(modelview * Vec4(world_point, 1));
	if (p.w > 0)
	{
		// in front of camera
		x = ftoi(static_cast<float>(width * (0.5f + p.x * 0.5f / p.w)));
		y = ftoi(static_cast<float>(height - height * (0.5f + p.y * 0.5f / p.w)));
		return true;
	}
	else
	{
		// behind camera
		x = ftoi(width - static_cast<float>(width * (0.5f + p.x * 0.5f / p.w)));
		y = ftoi(height - static_cast<float>(height - height * (0.5f + p.y * 0.5f / p.w)));
		return false;
	}
}

bool SampleSplineEditor::get_point_under_mouse_cursor(Unigine::Math::Vec3 &out_point)
{
	auto viewport = ViewportManager::getActiveViewportWindow();
	if (viewport == nullptr)
	{
		return false;
	}

	PlayerPtr player = viewport->getPlayer();
	if (player == nullptr)
	{
		return false;
	}

	const ivec2 win_size = viewport->getSize();
	const ivec2 mouse_pos = viewport->getMousePos();
	const Vec3 player_pos = player->getWorldPosition();
	const Vec3 player_dir = Vec3(
		player->getDirectionFromScreen(mouse_pos.x, mouse_pos.y, 0, 0, win_size.x, win_size.y));

	WorldIntersectionPtr intersection = WorldIntersection::create();
	if (World::getIntersection(player_pos + player_dir * player->getZNear(),
			player_pos + player_dir * player->getZFar(), ~0, intersection))
	{
		out_point = intersection->getPoint();
		return true;
	}
	else
	{
		return false;
	}
}

void SampleSplineEditor::clear()
{
	id_to_index_.clear();
	canvas_->clear();

	selected_.clear();
	points_.clear();

	manipulator_.deleteLater();
}

void SampleSplineEditor::update_description()
{
	// Get undo/redo shortcuts of editor
	auto global_context = ShortcutManager::getContext("global");
	auto sh_undo = global_context->getShortcut("undo");
	auto sh_redo = global_context->getShortcut("redo");

	description_text_ = String::format(DESCRIPTION_TMPL_, sh_select_->toString().get(),
		sh_add_or_extend_->toString().get(), sh_delete_->toString().get(),
		sh_focus_->toString().get(), sh_undo->toString().get(), sh_redo->toString().get())
							.get();
}

CommandSaveState::CommandSaveState(SampleSplineEditor &editor, const Vector<Vec3> &prev_points,
	const Vector<int> &prev_selected)
	: editor_(editor)
	, prev_points_(prev_points)
	, prev_selected_(prev_selected)
	, now_points_(editor_.getPoints())
	, now_selected_(editor_.getSelected())
{}

void CommandSaveState::apply()
{
	editor_.setPoints(now_points_);
	editor_.setSelected(now_selected_);
}

void CommandSaveState::undo()
{
	editor_.setPoints(prev_points_);
	editor_.setSelected(prev_selected_);
}

void CommandSaveState::redo()
{
	apply();
}

SaveStateToUndoStack::SaveStateToUndoStack(SampleSplineEditor &editor)
	: editor_(editor)
	, prev_points_(editor_.getPoints())
	, prev_selected_(editor_.getSelected())
{}

SaveStateToUndoStack::~SaveStateToUndoStack()
{
	// create action to undo stack
	Undo::instance()->push(new CommandSaveState(editor_, prev_points_, prev_selected_));
}
