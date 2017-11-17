#pragma once

#include <Commands/Command.h>

class Forward: public Command {
public:
	Forward();
	void Initialize() override;
	void Execute() override;
	bool IsFinished() override;
	void End() override;
	void Interrupted() override;
};