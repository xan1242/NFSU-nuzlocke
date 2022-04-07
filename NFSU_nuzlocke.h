#pragma once
#define NUZLOCKE_ALREADYSTARTED_WARNING_MSG "You've loaded a profile which had already started playing Underground Mode.\nWhile you can play Nuzlocke from this point on, it is highly recommended that you start a new game!"
#define NUZLOCKE_CARLIFE_MSG "You've lost all lives on your current car and must select a different one.\nYou cannot leave this screen until you select a car.\n\nThis message will only be shown once."
// intro stuff
#define NUZLOCKE_INTRO_MSG "Welcome to the Nuzlocke Challenge!\nPlay through the game as fast as possible with a limited number of lives!"
// rules
#define NUZLOCKE_INTRO_RULES_TREEHEAD "Rules"
#define NUZLOCKE_INTRO_RULES_BP1 "Each car has a certain amount of lives."
#define NUZLOCKE_INTRO_RULES_BP2 "Losing a race event in any way is considered a loss of life, this includes: losing a race, quitting prematurely and/or restarting a race"
#define NUZLOCKE_INTRO_RULES_BP3 "On the last life of a car you are not allowed to restart the race"
#define NUZLOCKE_INTRO_RULES_BP4 "If you lose all cars = game over"
#define NUZLOCKE_INTRO_RULES_BP5 "If you can't afford any car trade = game over"
#define NUZLOCKE_INTRO_RULES_BP6 "Depending on the selected difficulty, you may not be allowed to access the car trade menu and/or select the difficulty of a race"
#define NUZLOCKE_INTRO_RULES_NOTE "The challenge starts as soon as you enter Underground Mode."
// additional notes
#define NUZLOCKE_INTRO_ADDNOTE_TREEHEAD "Additional notes (please read at least once)"
#define NUZLOCKE_INTRO_ADDNOTE_BP1 "This addon restarts every time you load or make a new profile. It is highly recommended you start from a new game every time (and play from start to finish if you can)!"
#define NUZLOCKE_INTRO_ADDNOTE_BP2 "You can open the stats window anytime by pressing the PAGE DOWN key on the keyboard, here you can track all stats tracked by this addon (when it's in focus it locks the game input, be careful)"
#define NUZLOCKE_INTRO_ADDNOTE_BP3 "In the difficulty selection screen, you can enable an option to replace the traffic AI with racer's AI independent of the difficulty you select."
#define NUZLOCKE_INTRO_ADDNOTE_BP4 "This addon replaces the game's mouse cursor and uses the one from the OS instead (due to technical reasons)"
#define NUZLOCKE_INTRO_ADDNOTE_BP5 "The game is automatically saved any time you exit, restart or finish a race (and whenever the game does it by itself normally). Save files can be located in the 'NuzlockeSaves' directory next to the game executable. You can only have 1 saved Nuzlocke session per profile."
#define NUZLOCKE_INTRO_ADDNOTE_BP6 "And lastly (if you hadn't noticed yet) you should be able to fully control this UI with a controller."
#define NUZLOCKE_INTRO_ADDNOTE_BP7 "Thanks for playing!"

// load message
#define NUZLOCKE_LOAD_MSG "A saved Nuzlocke session for '%s' exists.\nWould you like to load it?\n\nWARNING: This session will be overwritten with a new one upon next save/autosave if you choose to not load it now."

// error message
#define NUZLOCKE_ERROR_MSG "An error has occured during handling of the Nuzlocke save file."

#define NUZLOCKE_REASON_NOCARS "Reason: No cars available"
#define NUZLOCKE_REASON_COMPLETE "Reason: Game complete!"
#define NUZLOCKE_REASON_FINANCIAL "Reason: Can't afford any car"

#define NUZLOCKE_HEADER_GAMEOVER "NUZLOCKE - GAME OVER"
#define NUZLOCKE_HEADER_INTRO "NUZLOCKE - Introduction"
#define NUZLOCKE_HEADER_LOADGAME "NUZLOCKE - Load Game"
#define NUZLOCKE_HEADER_PROFILEWARNING "NUZLOCKE - Profile Warning"
#define NUZLOCKE_HEADER_CARLIFE "NUZLOCKE - Lost Life"
#define NUZLOCKE_HEADER_DIFFICULTY "NUZLOCKE - Select Difficulty"
#define NUZLOCKE_HEADER_ERROR "NUZLOCKE - Error"

#define NUZLOCKE_UI_CLOSE_TXT "(A) Close"

#define NUZLOCKED_NUZ_DIFF_COUNT 5
#define NUZLOCKE_UI_NUZ_DIFF_EASY "Easy"
#define NUZLOCKE_UI_NUZ_DIFF_MEDIUM "Medium"
#define NUZLOCKE_UI_NUZ_DIFF_HARD "Nuzlocke"
#define NUZLOCKE_UI_NUZ_DIFF_ULTRAHARD "Ultra Nuzlocke"
#define NUZLOCKE_UI_NUZ_DIFF_CUSTOM "Custom"

//#define NUZLOCKE_UI_GAME_DIFF_UNLOCKED "Unlocked"
#define NUZLOCKE_UI_GAME_DIFF_EASY "Unlocked"
#define NUZLOCKE_UI_GAME_DIFF_MEDIUM "Medium"
#define NUZLOCKE_UI_GAME_DIFF_HARD "Hard"

#define NUZLOCKE_UI_CARTRADING_ALLOWED "Allowed"
#define NUZLOCKE_UI_CARTRADING_DISALLOWED "Not allowed"

#define NUZLOCKE_DIFF_EASY 0
#define NUZLOCKE_DIFF_EASY_LIVES 3
#define NUZLOCKE_DIFF_EASY_TRADING true
#define NUZLOCKE_DIFF_EASY_LOCKEDDIFF 0

#define NUZLOCKE_DIFF_MEDIUM 1
#define NUZLOCKE_DIFF_MEDIUM_LIVES 2
#define NUZLOCKE_DIFF_MEDIUM_TRADING true
#define NUZLOCKE_DIFF_MEDIUM_LOCKEDDIFF 0

#define NUZLOCKE_DIFF_HARD 2
#define NUZLOCKE_DIFF_HARD_LIVES 1
#define NUZLOCKE_DIFF_HARD_TRADING false
#define NUZLOCKE_DIFF_HARD_LOCKEDDIFF 1

#define NUZLOCKE_DIFF_ULTRAHARD 3
#define NUZLOCKE_DIFF_ULTRAHARD_LIVES 1
#define NUZLOCKE_DIFF_ULTRAHARD_TRADING false
#define NUZLOCKE_DIFF_ULTRAHARD_LOCKEDDIFF 2

#define NUZLOCKE_DIFF_CUSTOM 4
