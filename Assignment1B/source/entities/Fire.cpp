#include "../../include/entities/Fire.h"
#include <iostream>

Fire::Fire() : Entity(EntityType::FIRE), ttl(startTimeToLive)
{
}

Fire::~Fire()
{}

void Fire::update(Game* game, float elapsed)
{
	// XI.C First, update the position of the Fire object by calling the parent Entity::update() function.


	// XI.D Time to live (Fire::ttl member variable) needs to be reduced by 1 at every frame. If this gets
	//		to 0, the entity must be deleted (here, just setting the deleted flat to ture).
	

}
