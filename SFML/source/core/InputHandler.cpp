#include "../../include/core/InputHandler.h"
#include "../../include/core/Command.h"
#include "../../include/graphics/Window.h"
#include <SFML/Window/Keyboard.hpp>
#include <iostream>
#include <vector> 

InputHandler::InputHandler()
{
    pauseCommand = std::make_shared<PauseCommand>();  // Initialize PauseCommand
}

std::shared_ptr<Command> InputHandler::handleInput(const Window& window)
{
    // Edge-triggered: one toggle per key press, however long the key is held.
    // Gamepad Start (button 7) pauses too.
    if (window.wasKeyPressed(sf::Keyboard::Escape) || window.wasJoyButtonPressed(7))
    {
        return pauseCommand;
    }
    return nullptr;
}

PlayerInputHandler::PlayerInputHandler()
{
    moveRightCommand = std::make_shared<MoveRightCommand>();
    moveLeftCommand = std::make_shared<MoveLeftCommand>();
    moveUpCommand = std::make_shared<MoveUpCommand>();
    moveDownCommand = std::make_shared<MoveDownCommand>();
    attackCommand = std::make_shared<AttackCommand>();
    shoutCommand = std::make_shared<ShoutCommand>();

    inputMode = InputMode::WASD; // Default to WASD
}

std::vector<std::shared_ptr<Command>>& PlayerInputHandler::handleInput(const Window& window)
{
    commandQueue.clear();

    // Toggle input mode with Tab (edge-triggered). Enter is reserved for menus.
    if (window.wasKeyPressed(sf::Keyboard::Tab)) {
        toggleInputMode();
    }

    if (inputMode == InputMode::WASD) {
        if (window.isKeyDown(sf::Keyboard::W)) commandQueue.push_back(moveUpCommand);
        if (window.isKeyDown(sf::Keyboard::A)) commandQueue.push_back(moveLeftCommand);
        if (window.isKeyDown(sf::Keyboard::S)) commandQueue.push_back(moveDownCommand);
        if (window.isKeyDown(sf::Keyboard::D)) commandQueue.push_back(moveRightCommand);
    }
    else if (inputMode == InputMode::ARROWS) {
        if (window.isKeyDown(sf::Keyboard::Up)) commandQueue.push_back(moveUpCommand);
        if (window.isKeyDown(sf::Keyboard::Left)) commandQueue.push_back(moveLeftCommand);
        if (window.isKeyDown(sf::Keyboard::Down)) commandQueue.push_back(moveDownCommand);
        if (window.isKeyDown(sf::Keyboard::Right)) commandQueue.push_back(moveRightCommand);
    }

    // Commands common to both modes
    if (window.isKeyDown(sf::Keyboard::Space)) commandQueue.push_back(attackCommand);
    if (window.isKeyDown(sf::Keyboard::LShift)) commandQueue.push_back(shoutCommand);

    // Gamepad (XInput-style layout): left stick / d-pad move, A attacks, B or X shouts.
    if (window.isJoystickConnected()) {
        const sf::Vector2f joy = window.getJoyDirection();
        const float threshold = 0.4f;
        if (joy.y < -threshold) commandQueue.push_back(moveUpCommand);
        if (joy.y >  threshold) commandQueue.push_back(moveDownCommand);
        if (joy.x < -threshold) commandQueue.push_back(moveLeftCommand);
        if (joy.x >  threshold) commandQueue.push_back(moveRightCommand);
        if (window.isJoyButtonDown(0)) commandQueue.push_back(attackCommand);
        if (window.isJoyButtonDown(1) || window.isJoyButtonDown(2)) commandQueue.push_back(shoutCommand);
    }

    return commandQueue;
}

// Toggle between WASD and Arrow input modes
void PlayerInputHandler::toggleInputMode() {
    if (inputMode == InputMode::WASD) {
        inputMode = InputMode::ARROWS;
        std::cout << "[InputHandler] Switched to ARROWS mode\n";
    }
    else {
        inputMode = InputMode::WASD;
        std::cout << "[InputHandler] Switched to WASD mode\n";
    }
}

InputMode PlayerInputHandler::getInputMode() const {
    return inputMode;
}