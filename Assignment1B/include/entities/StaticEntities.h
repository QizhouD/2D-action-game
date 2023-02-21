#pragma once
#include "Entity.h"


class Potion : public Entity
{
public:
	Potion() : Entity(EntityType::POTION) {}
	~Potion() {}

	void init(const std::string& textureFile, float scale) override
	{
		// III.C (1/2) Call the init() function in Entity to initalize this object

		// VIII.C (1/2) Set the top left and bottom right corners of the bounding box for this entity.
	}

	virtual void update(Game* game, float elapsed = 1.0f) override { }

	int getHealth() const { return potionHealth; }

protected:
	const int potionHealth = 10;
};


class Log : public Entity
{
public:
	Log() : Entity(EntityType::LOG) {}
	~Log() {}

	void init(const std::string& textureFile, float scale) override
	{
		// III.C (2/2) Call the init() function in Entity to initalize this object

		// VIII.C (2/2) Set the top left and bottom right corners of the bounding box for this entity.
	}

	virtual void update(Game* game, float elapsed = 1.0f) override {}

	int getWood() const { return woodAdded; }

protected:
	const int woodAdded = 15;
};