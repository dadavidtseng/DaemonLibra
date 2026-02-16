# SD1 Assignment 8: Libra Epilogue

## C34 Yu-Wei Tseng

Implementation of Libra, a semi-procedural action-arcade-RPG shooter.

## How to Use

### Keyboard

- **F1:** Toggles the debug render for entities.
- **F3:** Toggles noclip developer cheat, allowing the player to move through solid tiles.
- **F4:** Toggles the debug camera, which shows the entire current Map onscreen.
- **F6:** Toggles the debug heat maps, which shows different kind of heat maps (four in this version).
- **F8:** Does a hard restart of the game, deleting and re-newing the Game instance.
- **F9:** Switch to the next map in the game. If it is the last map, the player enters the game win scene and returns to
  attract mode.
- **WASD:** To drive forward & turn toward direction.
- **IJKL:** To aim turret toward direction.
- **P:** To start new game, or toggle pause in-game.
- **ESC:** To pause & leave game, and again to quit.
- **T:** Slows game simulation time to 1/10 the normal rate.
- **Y:** Speeds game simulation time up to 4x the normal rate.
- **O:** Step single update and pause.
- **`:** Toggles the developer console
- **ENTER:** Execute an event, right now only event "SHOOT".

### Xbox controller

- **XBOX_BUTTON_DPAD_UP:** Toggles the debug render for entities.
- **XBOX_BUTTON_DPAD_DOWN:** Toggles noclip developer cheat, allowing the player to move through solid tiles.
- **XBOX_BUTTON_LSHOULDER:** Toggles the debug camera, which shows the entire current Map onscreen.
- **XBOX_BUTTON_DPAD_RIGHT:** Does a hard restart of the game, deleting and re-newing the Game instance.
- **XBOX_LEFT_TRIGGER:** To drive forward & turn toward direction.
- **XBOX_RIGHT_TRIGGER:** To aim turret toward direction.
- **XBOX_BUTTON_START:** To start new game, or toggle pause in-game.
- **XBOX_BUTTON_BACK:** To pause & leave game, and again to quit.
- **XBOX_BUTTON_RTHUMB:** Slows game simulation time to 1/10 the normal rate.
- **XBOX_BUTTON_LTHUMB:** Speeds game simulation time up to 4x the normal rate.
- **XBOX_BUTTON_RSHOULDER:** Step single update and pause.

## Known Issues

- N/A

## Deep Learning

I really like how the XML data drives the entire project.
I look forward to implementing a more advanced event system and developer console.
Additionally, I am planning to implement various faster pathfinding algorithms,
as well as address some of the extreme edge cases that might occur in this project.
Finally, I am definitely going to create more diverse enemies to add depth to the project.




