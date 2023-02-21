#include "../graphics/Window.h"
#include "../core/Board.h"
#include "../entities/Player.h"

class InputHandler;

class Game
{
public:

	const int spriteWH = 50;
	const float tileScale = 2.0f;
	const float itemScale = 1.0f;

	Game();
	~Game();

	void init(std::vector<std::string> lines);
	void addEntity(std::shared_ptr<Entity> newEntity);

	void buildBoard(size_t width, size_t height);
	void initWindow(size_t width, size_t height);

	void handleInput();
	void update(float elapsed);
	void render(float elapsed);
	Window* getWindow() { return &window; }

	sf::Time getElapsed() const;
	void setFPS(int FPS);
	void togglePause() { paused = !paused; }
	bool isPaused() const { return paused; }

	//  IV.A (2/2) Write a function that returns the shared pointer of the player of the game.


	EntityID getIDCounter();
	std::shared_ptr<Entity> getEntity(unsigned int idx);

	template <typename T>
	std::shared_ptr<T> buildEntityAt(const std::string& filename, int col, int row);


private:

	Window window;
	bool paused;
	sf::Clock gameClock;
	sf::Time elapsed;

	// II.A Declare a unique pointer of type Board 


	// III.D Declare a vector from the standard template library that 
	//       contains shared pointers to Entity classes. Recommended name: entities.


	// III.E Declare a variable of type EntityID (which is declared in Entity.h). This variable will
	//       be incremented by one every time an entity is added to the game.


	
	// IV.A (1/2) Declare a pointer to a player object. The pointer must be shared pointer from the standard library.


	// V.A Declare a unique pointer to an Input Handler object for this class.


};

