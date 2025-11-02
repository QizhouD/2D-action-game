This project demonstrates a complete 2D game built with flexible ECS architecture options, implementing design patterns such as Command, Observer, and Service Locator. Key features include:
Adaptive game loop with target 60FPS rendering
Dual input mode support (WASD/Arrow keys) with toggle functionality
Entity collision detection with type-specific callback handling
Audio management via Service Locator pattern
Runtime entity spawning, destruction, and TTL (Time-To-Live) handling
Level loading from text-based map files
Real-time FPS monitoring and GUI status display
Component-based entity system supporting players, pickups, and projectiles
The engine architecture allows seamless switching between ECS implementations (Big Array, Archetypes, Packed Arrays) to compare performance characteristics while maintaining consistent gameplay functionality. Core systems include input processing, movement, collision detection, gameplay logic, and rendering, all designed for modularity and extensibility
