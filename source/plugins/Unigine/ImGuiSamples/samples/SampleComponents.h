#pragma once
#include "ISample.h"

class SampleComponents : public ISample
{
public:
	const char *getName() override { return "Components"; }
	void setEnabled(bool v) override { enabled_ = v; }
	bool isEnabled() const override { return enabled_; }
	void updateGui() override;

private:
	bool enabled_{false};
};
