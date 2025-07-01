#include "SampleComponents.h"

#include <UnigineComponentSystem.h>
#include <UnigineGame.h>
#include <imgui/imgui.h>

using namespace Unigine;
using namespace Math;

/////////////////////////////////////////////////////////
// Component
/////////////////////////////////////////////////////////

class NodeRotation : public ComponentBase
{
public:
	COMPONENT_DEFINE(NodeRotation, ComponentBase);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Float, speed_x, 0, "Rotation Speed X",
		"Rotation around the X axis (in degrees per second)", "", "min=-180;max=180");
	PROP_PARAM(Float, speed_y, 0, "Rotation Speed Y",
		"Rotation around the Y axis (in degrees per second)", "", "min=-180;max=180");
	PROP_PARAM(Float, speed_z, 45, "Rotation Speed Z",
		"Rotation around the Z axis (in degrees per second)", "", "min=-180;max=180");

private:
	void update()
	{
		vec3 s = vec3(speed_x, speed_y, speed_z) * Game::getIFps();
		node->setRotation(node->getRotation() * quat(s.x, s.y, s.z));
	}
};

REGISTER_COMPONENT(NodeRotation)

/////////////////////////////////////////////////////////
// TabComponents
/////////////////////////////////////////////////////////
void SampleComponents::updateGui()
{
	ImGui::TextWrapped(
		"Click on \"Initialize the ComponentSystem\" to run the C++ Component System.\n"
		"It will automatically create the \"NodeRotation\" component (property).\n"
		"Next, assign it to any node and it will immediately start rotating (right in the "
		"Editor!).");

	if (ImGui::Button("Initialize the ComponentSystem"))
	{
		ComponentSystem::get()->initialize();
	}
}
