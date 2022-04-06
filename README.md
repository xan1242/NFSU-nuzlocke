# NFS Underground - Nuzlocke Challenge

Can you beat the game with 1 life per car?

Are you ready for the Nuzlocke Challenge in NFS Underground?

Then this mod is for you!

## What is this exactly?

This is a mod for NFS Underground which sets the rules of the Nuzlocke Challenge (or multiple lives) and applies them directly to the game code. 

It minimally and seamlessly transforms the game into an arcade-type challenge with lives, timers and points.

## OK, but what exactly is this so-called "Nuzlocke Challenge"?

If you aren't familiar with the term "Nuzlocke", [then you may want to watch this video from KuruHS as an example](https://www.youtube.com/watch?v=AomBjtyJKWs).

This challenge had been introduced in other games, not just Need for Speed.

For those that don't want to watch or can't, TLDW: 1 life per car. You lose a race, you lose the car permanently.

## How does this work?

If you look at the spaghetti-ridden code, you'll notice that I've hooked many different places of the game to enforce the rules.

To sum it up - whenever there's an event of interest in the game where we can enforce a rule, this plugin hooks into that part of the code and forces it to do Nuzlocke stuff (such as car locking, difficulty setting locking, etc.).

## Installation

- As with any other ASI mod, extract and merge the 'scripts' folder with the one in your game root directory.

- If you do not have the Ultimate ASI Loader already installed, install it to your game

- If you do not already have Visual C++ 2022 x86 redistributable installed, install it

## Technical info

- There are 2 timers: PLAY time and RACE time

- PLAY time: it is the total playing time in Underground Mode, timed with 1ms resolution, including game loading and everything (it is currently measured even in Main Menu until you enter a different mode)

- RACE time: actual gameplay time out of menus and in the game. This timer is 100% accurate and is ticked with the game's own timer used for measuring lap/race time. Whenever the game time slows down (e.g. jump camera), this timer slows down too. This timer uses 4ms resolution.

- Each car has its own race timer stored and is displayed as "Race:" in the ingame HUD

- Total race time is accumulated race time of each car and is displayed in "Total:" in the ingame HUD

- Mouse reading was replaced with the Win32 API because of imgui integration

- Optional MiniWorld segment increase was included, but can also be disabled if you use HD Reflections from Aero_

- The input handoff between imgui and the game is very fast, so if you hold a button for a long time after closing a dialog, it'll actually trigger the ingame action.

# Credits

- [ocornut (omar)](https://github.com/ocornut) & others - for the amazing [Dear ImGui](https://github.com/ocornut/imgui)

- [ThirteenAG (Sergey P.)](https://github.com/ThirteenAG) - Ultimate ASI Loader

- [thelink2012 (Denilson das Mercês Amorim)](https://github.com/thelink2012) - injector & IniReader


