#include "../../include/graphics/SpriteSheet.h"
#include "../../include/graphics/AnimDirectional.h"
#include <fstream>
#include <stdexcept>

SpriteSheet::SpriteSheet() :
	curAnimation(nullptr),
	spriteScale(1.f, 1.f),
	direction(Direction::Right)
{
}

SpriteSheet::~SpriteSheet()
{
}

void SpriteSheet::releaseSheet()
{
	curAnimation = nullptr;
	while (animations.begin() != animations.end())
	{
		delete animations.begin()->second;
		animations.erase(animations.begin());
	}
}

void SpriteSheet::setSpriteSize(const sf::Vector2i& size)
{
	spriteSize = size;
}


void SpriteSheet::setSpriteScale (const sf::Vector2f& scale)
{
	spriteScale = scale;
	sprite.setScale(spriteScale);
}

void SpriteSheet::setSpritePosition(const sf::Vector2f& pos)
{
	sprite.setPosition(pos);
}

void SpriteSheet::setSpriteDirection(const Direction& dir)
{
	if (dir == direction)
		return;
	direction = dir;
	if (curAnimation != nullptr)
		curAnimation->cropSprite();
}

void SpriteSheet::cropSprite(const sf::IntRect& rect)
{
	sprite.setTextureRect(rect);
}

bool SpriteSheet::loadSheet(const std::string& file)
{
	std::ifstream sheet;
	sheet.open(file);
	if (sheet.is_open())
	{
		releaseSheet();
		std::string line;
		while (std::getline(sheet, line))
		{
			if (line[0] == '#') continue;	//Commented line, ignore.

			std::stringstream keystream(line);
			std::string type;

			keystream >> type;
			if (type == "Texture")
			{
				std::string textureFile;
				keystream >> textureFile;

				if (!texture.loadFromFile(textureFile))  // This would be better in a resources manager.
					throw std::runtime_error("Texture file not found: " + textureFile);

				sprite.setTexture(texture);

			}
			else if (type == "Size")
			{
				keystream >> spriteSize.x >> spriteSize.y;
				setSpriteSize(spriteSize);
			}
			else if(type == "Scale")
			{
				keystream >> spriteScale.x >> spriteScale.y;
				setSpriteScale(spriteScale);
			}
			else if (type == "AnimationType")
			{
				keystream >> animType;
			}
			else if (type == "Animation")
			{
				std::string name;
				keystream >> name;

				if (animations.find(name) != animations.end())
				{
					throw std::runtime_error("Duplicated animation: " + name + " in sprite sheet " + file);
				}
				else
				{
					AnimBase* anim = nullptr;
					if (animType == "Directional")
						anim = new AnimDirectional();
					else
						throw std::runtime_error("Unknown animation type: " + animType + " in sprite sheet " + file);
				
					keystream >> *anim;
					anim->setSpriteSheet(this);
					anim->setName(name);
					anim->reset();
					animations.emplace(name, anim);
				}
			}

		}
		sheet.close();
		return true;
	}

	throw std::runtime_error("ERROR: failed loading spritesheet " + file);

	return false;
}



AnimBase* SpriteSheet::getCurrentAnim() const 
{
	return curAnimation;
}

bool SpriteSheet::setAnimation(const std::string& name, bool play, bool loop)
{
	auto itr = animations.find(name);
	if (itr == animations.end()) return false;
	if (itr->second == curAnimation) return false;
	if (curAnimation)
		curAnimation->stop();

	curAnimation = itr->second;
	curAnimation->setLooping(loop);
	if (play)
		curAnimation->play();
	curAnimation->cropSprite();
	return true;
}

void SpriteSheet::update(float elapsedTime)
{
	curAnimation->update(elapsedTime);
}

void SpriteSheet::draw(sf::RenderWindow* window)
{
	window->draw(sprite);
}

