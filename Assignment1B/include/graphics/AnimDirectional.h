//Adapted from SFML Game Development By Example (Raimondas Pupius)
#pragma once
#include "AnimBase.h"

class AnimDirectional : public AnimBase
{
protected:
	void frameStep();
	void cropSprite();
	void readIn(std::stringstream& st);
};

