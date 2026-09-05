#pragma once
#include <memory>
#include "Components.h"

class Game;
class PlayerInputHandler;

// Base class for input components.
class InputComponent: public Component {
public:
    ComponentID getID() const override {
        return ComponentID::INPUT;
    }

    virtual ~InputComponent() = default;
    virtual void update(Game& game) = 0;
};

// Player-specific input component that uses PlayerInputHandler.
class PlayerInputComponent : public InputComponent {
public:
    PlayerInputComponent();
    // Defined in the .cpp where PlayerInputHandler is a complete type
    // (required for std::unique_ptr to a forward-declared class).
    ~PlayerInputComponent() override;
    virtual void update(Game& game) override;

private:
    std::unique_ptr<PlayerInputHandler> inputHandler;
};

