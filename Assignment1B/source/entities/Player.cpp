#include "../../include/entities/Player.h"
#include "../../include/graphics/AnimBase.h"
#include "../../include/entities/Fire.h"
#include "../../include/core/Game.h"
#include <iostream>


Player::Player() : Entity(EntityType::PLAYER), attacking(false), shouting(false), health(60), wood(0), shootCooldown(0)
{
	speed = playerSpeed;

	// VI.B: Create the unique pointer to the PlayerInputHandler object


}

Player::~Player() {}

void Player::update(Game* game, float elapsed)
{

		// VI.G Modify the code below to add the functionality to play the appropriate animations 
		//      and set the appropriate directions for movement depending on the  value of the
		//      velocity vector for moving up, down and left.


		// VI.F (1/2) If the X component of the velocity vector is positive, we're moving to the right.
		//            Set the animation of the spritesheet to "Walk". Mind the parameters required for the
		//			  animation: if it should start playing and if it should loop.
		//			  Additionally, you must also set the sprite direction (to Direction::Right) of the spritesheet.
		

		// VI.F (2/2) If the player is not moving, we must get back to playing the "Idle" animation.
		

	
	// IV.D (1/2) Call the function update in the base class to do the general update stuff that is common to all entities.

	// XI.B (2/2):  Reduce the shoot cooldown counter by the elapsed time at every frame. 
	//              Only do this if shoot cooldown is > 0 (can you guess why?)

	// XI.A: Create an Fire entity object (using Player::createFire()) and add it to the game (using Game::addEntity).
	//       Then, remove the shooting cost (Player::shootingCost) from the wood member variable of this class
	//       Finally, wrap the functionality below in an IF statement, so we only spawn fire when:
	//            1) We are playing the shouting animation
	//			  2) The animation is in one of the "in action" frames.
	//			  3) We have enough wood "ammunition" (variable wood and shootingCost)

		// XI.B (1/2): Set the variable shootCooldown to the cooldown time (defined in shootCooldownTime).
		//        Add another condition to the shooting IF statement that only allows shoowing if shootCooldown <= 0.

	
	// VII.B: If we are attacking but the current animation is no longer playing, set the attacking flag to false.
	//        The same needs to be done for "shouting".

}


void Player::handleInput(Game& game)
{
	// VI.E Set the velocity of this player to (0, 0)


	// VI.C: Call the fucntion that handles the input for the player and retrieve the command returned in a variable.
	//       Then, call the "execute" method of the returned object to run this command.


	// VII.A Modify the code ABOVE so, instead of calling "execute" in a command pointer, iterates through
	//       the vector of commands and executes them all.
}

std::shared_ptr<Fire> Player::createFire() const
{
	auto fireEntity = std::make_shared<Fire>();		

	Vector2f pos { position.x + getTextureSize().x * 0.5f,  position.y + getTextureSize().y * 0.5f };
	fireEntity->init("img/fire.png", 1.0f);
	fireEntity->setPosition(pos.x, pos.y);
	Vector2f vel(fireSpeed, 0.f);
	if (spriteSheet.getSpriteDirection() == Direction::Left) vel.x = vel.x * -1.0f;
	fireEntity->setVelocity(vel);

	return fireEntity;
}

void Player::addHealth(int h)
{
	health += h;
	if (health > maxHealth) health = maxHealth;
	if (health < 0) health = 0;
}

void Player::addWood(int w)
{
	wood += w;
	if (wood > maxWood) wood = maxWood;
	if (wood < 0) wood = 0;
}


void Player::positionSprite(int row, int col, int spriteWH, float tileScale)
{
	sf::Vector2f scaleV2f = getSpriteScale();
	sf::Vector2i textureSize = getTextureSize();

	float x = col * spriteWH * tileScale;
	float y = (row)*spriteWH * tileScale;
	float spriteSizeY = scaleV2f.y * textureSize.y;
	float cntrFactorY = ((spriteWH * tileScale) - spriteSizeY);	// to align to lower side of the tile.
	float cntrFactorX = cntrFactorY * 0.5f;						//to center horizontally

	setPosition(x + cntrFactorX, y + cntrFactorY);
	setVelocity({ 0.0f, 0.0f });
}
