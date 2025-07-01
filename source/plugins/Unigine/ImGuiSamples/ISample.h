#pragma once

class ISample
{
public:
	virtual ~ISample() = default;

	virtual const char *getName() = 0;
	virtual void setEnabled(bool v)= 0;
	virtual bool isEnabled() const = 0;

	// imgui logic
	virtual void updateGui() {}

	// system logic
	virtual void init() {}
	virtual void update() {}
	virtual void shutdown() {}

	// world logic
	virtual void worldInit() {}
	virtual void worldUpdate() {}
	virtual void worldShutdown() {}
};
