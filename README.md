
# PROTECTOR

![Title Screen](/screens/title.png?raw=true "Title Screen")

A Ludum Dare 46 game jam compo entry. April 2020.

**THEME :** *KEEP IT ALIVE*

**Platform :** Nintendo Entertainment System \[NES\]

**Language : C** *(ASM source in this repo were either generated (sounds effects), or are library required for building the project.)*

![Box Art](/screens/boxart.png?raw=true "Box Art")

### Description

This a 1-player Arcade action game.
You are a hero, with your antivirus gun, you must destroy the incoming viruses, survive and protect the nurse in an hospital.

![Game](/screens/gameplay.png?raw=true "Game")

### Controls

* Hold A to strafe
* Press B to shoot
* Use D-PAD to move around
* Press SELECT to toggle the music if it annoys you
* Press START to pause the game

### How to run

Download .nes ROM. Open it in your favorite NES emulator. 
I recommend to use [Mesen](https://mesen.ca/) for an optimal experience.

If you do not want to install an emulator, you can just go to https://jsnes.org/, then drag and drop the .nes rom on the webpage to play.

### How was it made

**Libraries :**

* Shiru's neslib
* Shiru's famitone2
[Library are included in this project/repo to allow easy compilation]

**Tools used :**

* Visual Studio Code (Code editor)
* Shiru's NES Screen Tools (CHR graphic bank editor, and nametable map tool, NES palette editor)
* CA65 compiler (Compiler)
* Famitracker  (Sound & Music)
* Mesen (Emulator & Debug tools)
* Shiru's NES Space Checker (Check generated ROM)

### Special Thanks

I'd like to thanks :

* Shiru for his invaluable NES development tutorials, tools, examples and learning resources
* All the developers of the tools i used. (Mesen, Famitracker, Visual Studio Code, CA65)
* The LDJAM event organizers
* Anyone that is going to play and rate this game during the competion
* ...
* AND all the people fighting the pandemic while i'm at home making silly games !

### How to build

Download CC65 from [here](ftp://ftp.musoftware.de/pub/uz/cc65/)
Unpack it somewhere and add bin folder to path.

Run _compile.bat to build protector.

Important: This will only compile with CC65 **v2.13.3.**