#pragma once
#include "Entity.h"

class Fire : public Entity
{
public:
	const int startTimeToLive = 150; //frames

	Fire();
	~Fire();

	virtual void update(Game* game, float elapsed = 1.0f) override;

	int getTTL() const { return ttl; }

private:

	int ttl;

};

