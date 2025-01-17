# UE4_FPSGame_Learning
A stepping stone project that was used as my first actual game inside UE4, The game is not pretty nor is much over what ever you would get if you followed the course (listed below). Although this was a project that teached me the basics of UE4 with some simple networking through Replication.

# Learning source used
I used the following course to get a grasp on the basic features of UE4.  
[Unreal Engine 4 Mastery: Create Multiplayer Games with C++](https://www.udemy.com/unrealengine-cpp)

# Additions
I have made a few changes / additions to from the base lessons, though not all are listed here are the main ones:
- Implemented a build counter using the [UE4 Build system, FPSGameEditor.Target.cs](/ue4_mastery_first_fpsgame/Source/FPSGameEditor.Target.cs).
- Fixed projectile collisions with the shooter (this becomes a issue when networking is involved as the player is not always synced up from the projectile origin causing an overlap).
- Added build information into the GameMode then later moved into the GameState to work on all clients, these are variables that can be accessed through Blueprints to create a UI element. 
This information is stored inside [GameVersion.generated.h](/ue4_mastery_first_fpsgame/Source/FPSGame/Public/GameVersion.generated.h) 
but can be changed through editing [FPSGameEditor.Target.cs](/ue4_mastery_first_fpsgame/Source/FPSGameEditor.Target.cs).
- Added a pulsating material for the mission objective extraction zone.

# Screenshots
### Ingame
![Ingame](/docs/resources/ue4_mastery_first_fpsgame/ue4_mastery_first_fpsgame.ingame.PNG "Ingame")

### Level design
![Level design](/docs/resources/ue4_mastery_first_fpsgame/ue4_mastery_first_fpsgame.level_design.PNG "Level design")

### UIVersion Blueprint
![Version Blueprint](/docs/resources/ue4_mastery_first_fpsgame/ue4_mastery_first_fpsgame.version_info_example.PNG "Blueprint for showing Version on UI")

### Pulsating Material with Properties
![Pulsating Material with Properties](/docs/resources/ue4_mastery_first_fpsgame/ue4_mastery_first_fpsgame.customizable_pulsing_material.PNG "Pulsating Material with Properties")
