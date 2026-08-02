Somatic Camera FO4 — Pseudo First-Person Camera for Fallout 4
Version: Beta V2 - 
Compatible with: Fallout 4 (v1.11.221.0)
(If you have a different version of the game, please test pseudo-camera and let me know if it works!)

Description:
SomaticCameraFO4 is a beta Fallout 4 mod that adds a pseudo first-person (pseudo FPP) camera system, allowing the player to experience the game world from a third-person perspective with a camera positioned at the character's head height — closely mimicking a true first-person view while retaining third-person situational awareness.
The mod is toggled with the F4 key (default toggle key, configurable in the INI file).
How It Works:
The mod hooks into Fallout 4's camera system via F4SE (Fallout 4 Script Extender) and uses vtable hooks on PlayerCamera::Updaten and NiAVObject scene-graph updates. When activated, the mod pushes a k3rdPerson camera state onto the engine's camera stack and continuously repositions the camera root to follow the player's head node, applying user-configurable vertical and forward offsets. The actual camera rotation (mouse look) is left entirely to the engine, so the pseudo FPP experience feels natural and responsive.
The camera offsets are fully adjustable in the mod's INI configuration file:
fHeightOffset — vertical offset applied to the camera position 
fForwardOffset — forward offset along the camera's view direction
Both offsets can be freely tweaked to suit personal preference. The default values are set to what works well for my own playstyle, but they are entirely optional and can be adjusted to taste.
The mod also includes:
ADS (Aim Down Sights) — a Starfield-style physical-button aiming system that temporarily switches the camera to true first-person when the player aims, then smoothly returns to the pseudo FPP rig when aiming stops.
Furniture & power armor transition support — the pseudo camera gracefully handles special camera states like furniture sitting and power armor transitions.
Pip-Boy & menu compatibility — the mod detects blocking menus (Pip-Boy, terminals, dialogue, crafting, etc.) and temporarily disables the pseudo rig so the engine can handle menu cameras normally.
What Inspired It:
This mod was inspired by  Improved Camera for Skyrim and  SomaticCameraSF  and the general concept of pseudo first-person camera rigs found in various third-person games. The Starfield implementation served as the primary reference architecture, including its approach to camera state management, scene-graph hooking, and ADS handling. The FO4 version was built from scratch to adapt these concepts to Fallout 4's different camera system and engine architecture.
What Was Used to Build It:
F4SE (ianpatt) — the Fallout 4 Script Extender, used for plugin loading and API access
CommonLibF4 (Libxe)— the common library for Fallout 4 modding, providing RE (RedEngine) bindings, REL (relocation) utilities, and plugin infrastructure. This file uses CommonLibF4, which is licensed under the GNU GPLv3 
MinHook (TsudaKageyu) — used for vtable and function hooking
github SomaticCameraFO4 
- Inspired by Improved Camera for Skyrim - ArranzCNL and TwistedModding
Requirements:
Fallout 4  (v1.11.221.0)
F4SE
Address Library 
