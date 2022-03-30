// NFS Underground - Nuzlocke Challenge
// by Xan/Tenjoin

// TODO: add session saving - this game is prone to crashing regardless of this mod
// TODO: add a points system of some sort -- done?? using style points for now... but I think we should track the acculumated points during the runtime of Nuzlocke
// TODO: add an "extra life" system maybe
// TODO: session saving maybe?

#include "NFSU_nuzlocke.h"
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <timeapi.h>
#include "includes\injector\injector.hpp"
#include "inireader\IniReader.h"

//////////////////////////////////////////////////////////////////
// Dear ImGui declarations
//////////////////////////////////////////////////////////////////
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>

#define GAME_HWND_ADDR 0x00736380
#define GAME_D3DDEVICE_ADDR 0x0073636C

static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
bool bInitedImgui = false;
bool bBlockedGameInput = false;
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// DIALOG BOOLS
bool bShowIntroMessage = false;
bool bShowAlreadyStartedWarning = false;
bool bShowStatsWindow = false;
bool bShowGameOverScreen = false;
bool bShowCarLifeHint = false;
bool bShowDifficultySelector = false;

// Show-once bools
bool bShownGameOverOnce = false;
bool bShownLifeOverOnce = false;
// permanent ones -- CONFIG
bool bSkipIntroMessage = false;
bool bSkipAlreadyStartedWarning = false;

// HUD hide stuff -- CONFIG
bool bHideFEHUD = false;
bool bHideIGHUD = false;

ImFont* lcd_font;
char hud_disp_string[1024];

// Win32 mouse API stuff
RECT windowRect;
bool bMousePressedDown = false;
bool bMousePressedDownOldState = false;
bool bConfineMouse = false;
int TimeSinceLastMouseMovement = 0;

#define MOUSEHIDE_TIME 5000
#define FEMOUSECURSOR_X_ADDR 0x0070649C
#define FEMOUSECURSOR_Y_ADDR 0x007064A0
#define FEMOUSECURSOR_BUTTONPRESS_ADDR 0x007064B0
#define FEMOUSECURSOR_BUTTONPRESS2_ADDR 0x007064B1
#define FEMOUSECURSOR_BUTTONPRESS3_ADDR 0x007064B2
#define FEMOUSECURSOR_CARORBIT_X_ADDR 0x007064A4
#define FEMOUSECURSOR_CARORBIT_Y_ADDR 0x007064A8
#define FEMOUSECURSOR_CARORBIT_Z_ADDR 0x007064AC

#define FEHUD_FONT_SCALE 1.5
#define IGHUD_FONT_SCALE 1.5

//////////////////////////////////////////////////////////////////
// Dear ImGui declarations end
//////////////////////////////////////////////////////////////////

// Game-related addresses
#define CAREER_CAR_TYPE_HASH_POINTER 0x75EF00
#define FECAREERMANAGER_POINTER 0x75EEF8
#define THEGARAGEMANAGER_ADDR 0x00758AD4
#define CARTYPEINFOARRAY_ADDRESS 0x00734588
#define GAMEFLOWMANAGER_STATUS_ADDR 0x0077A920
#define GAMEFLOWMANAGER_OBJ_ADDR 0x0077B240
#define CURRENTWORLD_OBJ_ADDR 0x007361F8
#define GAMEMODE_ADDR 0x00777B4C
#define RACETYPE_ADDR 0x00777CC8
#define FEDATABASE_ADDR 0x00748F70
#define CFENG_PINSTANCE_ADDR 0x0073578C
#define PLAYERMONEY_ADDR 0x0076026C
#define GAMEPAUSED_ADDR 0x007041C4
// currentrace + 0x14 = timer
#define CURRENTRACE_POINTER_ADDR 0x0073619C
#define PLAYER_POINTER 0x007361BC
#define CAREER_STATUS_ADDR 0x0075F2B5
char* TradeCarScreen = "MU_UG_TradeCareerCar.fng";

unsigned int NumberOfCars = 35; // this is a fixed value in the executable, change only if you manage to increase the car count in the game

// difficulty-related stuff
unsigned int NuzlockeDifficulty = 0; // 0 = easy, 1 = medium, 2 = hard/nuzlocke, 3 = ultra hard/ultra nuzlocke, 4 = custom
unsigned int NumberOfLives = 1; // starting number of lives
bool bAllowTradingCarMidGame = false; // option to allow/disallow car changing mid-game
unsigned int LockedGameDifficulty = 0; // 0 = unlocked, 1 = medium, 2 = hard
// VARIABLE FOR FUN - force traffic cars into race paths (independent of difficulty)
bool bTrafficRacers = false;

// miscellaneous
#define MINIWORLD_NUMVISIBLESEGMENTS_MAIN_ADDR 0x006FB048
#define MINIWORLD_NUMVISIBLESEGMENTS_REF_ADDR 0x006FB04C
#define MINIWORLD_NUMVISIBLESEGMENTS_ENV_ADDR 0x006FB050
bool bIncreaseMiniworldSegments = true;

unsigned int CustomNumberOfLives = 2;
unsigned int CustomLockedGameDifficulty = 0;
bool bCustomAllowTrading = false;

const char CarTradingStatusNames[2][16] = { NUZLOCKE_UI_CARTRADING_DISALLOWED, NUZLOCKE_UI_CARTRADING_ALLOWED };
const char NuzDifficultyNames[NUZLOCKED_NUZ_DIFF_COUNT][16] = { NUZLOCKE_UI_NUZ_DIFF_EASY, NUZLOCKE_UI_NUZ_DIFF_MEDIUM, NUZLOCKE_UI_NUZ_DIFF_HARD, NUZLOCKE_UI_NUZ_DIFF_ULTRAHARD , NUZLOCKE_UI_NUZ_DIFF_CUSTOM };
const char GameDifficultyNames[3][16] = { NUZLOCKE_UI_GAME_DIFF_EASY, NUZLOCKE_UI_GAME_DIFF_MEDIUM, NUZLOCKE_UI_GAME_DIFF_HARD};

unsigned int CareerCarHash = 0; // currently selected car in career mode (not initialized until DDay is completed)
unsigned int GameFlowStatus = 0;
unsigned int Money = 0;
unsigned int StylePoints = 0;
unsigned int TimeSinceModeSwitch = 0;
unsigned int TimeBaseSinceModeSwitch = 0;
unsigned int OldGameMode = 0;
unsigned int LastCarTime = 0;
unsigned int LastTotalTime = 0;
unsigned int LastTotalPlayTime = 0;

unsigned int GameMode = 0; // 1 = career mode, others are quickrace or online modes
unsigned int RaceType = 0; // quick race - race type, currently unused

bool bGameIsOver = false;
bool bCantAffordAnyCar = false; // if player lost lives on current car and can't afford any = game over
bool bAllCarsLost = false; // if player got all their cars on 0 lives
bool bGameComplete = false; // if player beats UG mode with Nuzlocke
bool bGameStarted = false; // should start right after entering career mode
bool bProfileStartedCareer = false;
bool bRaceFinished = false;
bool bShowingRaceOver = false;
bool bMarkedStatusAlready = false;

unsigned int UnlockedCarCount = 0; // we MUST keep track of this -- if the player has 1 car unlocked, trade menu CANNOT open
unsigned int GameUnlockedCarCount = 0; // unlocked cars by the game progression itself

// some extra stats
unsigned int TotalLivesLost = 0;
unsigned int TotalLosses = 0;
unsigned int TotalWins = 0;
unsigned int TotalTimeSpentRacing = 0;
unsigned int TotalTimePlaying = 0;
unsigned int TotalEventsPlayed = 0;

enum CarUsageType
{
	CAR_USAGE_TYPE_RACING = 0,
	CAR_USAGE_TYPE_COP = 1,
	CAR_USAGE_TYPE_TRAFFIC = 2,
	CAR_USAGE_TYPE_WHEELS = 3,
	CAR_USAGE_TYPE_UNIVERSAL = 4,
};

struct NuzlockeStruct
{
	char* CarName;
	unsigned int CarNameHash;
	CarUsageType UsageType;
	unsigned int Lives;
	unsigned int Wins;
	unsigned int Losses;
	unsigned int TimeSpentRacing;
	unsigned int TimeOfDeathRT;
	unsigned int TimeOfDeathPT;
	bool bUnlocked;
}*NuzCars, DDayCar;

// DDay car name
char* DDayCarName = "D-Day";

struct CarTypeInfo
{
	char CarTypeName[32];
	char BaseModelName[32];
	char GeometryFilename[32];
	char GeometryFilenameComp[32];
	unsigned char padding[64];
	char ManufacturerName[16];
	unsigned int CarTypeNameHash;
	unsigned char RestOfData[0xB7C];
	int Type;
	CarUsageType UsageType;
	unsigned char RestOfData2[0x38];
}*CarTypeInfoArray;

//////////////////////////////////////////////////////////////////
// Function pointers
//////////////////////////////////////////////////////////////////
//bool(__fastcall* IsCarUnlocked)(unsigned int arg1, unsigned int arg2) = (bool(__fastcall*)(unsigned int, unsigned int))0x005A1630;
// FERaceEvent namespace
bool(__stdcall* HasEventBeenWon)(unsigned int arg1, unsigned int arg2) = (bool(__stdcall*)(unsigned int, unsigned int))0x005A2F10;
bool(__thiscall* IsThereANextRace)(void* dis) = (bool(__thiscall*)(void*))0x005A2C00;
// PauseMenu namespace
void(__stdcall* DoQuitRace)(int unk) = (void(__stdcall*)(int unk))0x004CDA50;
// UndergroundTradeCarScreen namespace
void(__stdcall*BuildTradeableList)(int unk) = (void(__stdcall*)(int))0x004C3240;
// PostRaceMenuScreen namespace
unsigned int(__stdcall*PostRaceMenuScreen_Setup)(unsigned int PostRaceMenuScreen, unsigned int PostRaceMenuSetupParams) = (unsigned int(__stdcall*)(unsigned int, unsigned int))0x004969E0;
// FE stuff
void(__thiscall* _FEngSetColor)(void* FEObject, unsigned int color) = (void(__thiscall*)(void*, unsigned int))0x004F75B0;
unsigned int(*FEngGetCurrentButton)(char* pkgname) = (unsigned int(*)(char*))0x004F64D0;
// UndergroundMenuScreen
void(__thiscall* UndergroundMenuScreen_NotificationMessage)(void* UndergroundMenuScreen, unsigned int msg, void* FEObject, unsigned int unk2, unsigned int unk3) = (void(__thiscall*)(void*, unsigned int, void*, unsigned int, unsigned int))0x004EB1E0;
// UndergroundTradeCarScreen
unsigned int(__stdcall*UndergroundTradeCarScreen_Constructor)(void* Object, void* ScreenConstructorData) = (unsigned int(__stdcall*)(void*, void*))0x004C2ED0;
//cFEng
unsigned int(__stdcall* cFEng_GetMouseInfo)(unsigned int FEMouseInfo) = (unsigned int(__stdcall*)(unsigned int))0x004F6080;
// GameFlowManager
bool(__thiscall* GameFlowManager_IsPaused)(void* dis) = (bool(__thiscall*)(void*))0x0043A2E0;
// UndergroundBriefScreen
void(__thiscall* UndergroundBriefScreen_NotificationMessage)(void* UndergroundBriefScreen, unsigned int msg, void* FEObject, unsigned int unk2, unsigned int unk3) = (void(__thiscall*)(void*, unsigned int, void*, unsigned int, unsigned int))0x004C5FA0;
void(__thiscall* UndergroundBriefScreen_LaunchCurrentEvent)(void* UndergroundBriefScreen, void* FEObject) = (void(__thiscall*)(void*, void*))0x004C5DF0;


void(*sub_546780)() = (void(*)())0x546780;
void(*sub_4DFD70)() = (void(*)())0x4DFD70;
void(*sub_40A4E0)() = (void(*)())0x0040A4E0;
void(*GenerateJoyEvents)() = (void(*)())0x00573CB0;
void(*game_crt_free)(void* block) = (void(*)(void*))0x00671102;


unsigned int GameWndProcAddr = 0;
LRESULT(WINAPI* GameWndProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//////////////////////////////////////////////////////////////////
// Function pointers end
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Inireader
//////////////////////////////////////////////////////////////////

void UpdateIniFile()
{
	CIniReader inireader("");

	inireader.WriteInteger("Nuzlocke", "SkipIntroMessage", bSkipIntroMessage);
	inireader.WriteInteger("Nuzlocke", "SkipAlreadyStartedWarning", bSkipAlreadyStartedWarning);
	inireader.WriteInteger("Nuzlocke", "HideFEHUD", bHideFEHUD);
	inireader.WriteInteger("Nuzlocke", "HideIGHUD", bHideIGHUD);

	inireader.WriteInteger("CustomGame", "NumberOfLives", CustomNumberOfLives);
	inireader.WriteInteger("CustomGame", "AllowTrading", bCustomAllowTrading);
	inireader.WriteInteger("CustomGame", "LockedGameDifficulty", CustomLockedGameDifficulty);
}

void ReadIniFile()
{
	CIniReader inireader("");

	bSkipIntroMessage = inireader.ReadInteger("Nuzlocke", "SkipIntroMessage", 0);
	bSkipAlreadyStartedWarning = inireader.ReadInteger("Nuzlocke", "SkipAlreadyStartedWarning", 0);
	bHideFEHUD = inireader.ReadInteger("Nuzlocke", "HideFEHUD", 0);
	bHideIGHUD = inireader.ReadInteger("Nuzlocke", "HideIGHUD", 0);
	NumberOfCars = inireader.ReadInteger("Nuzlocke", "NumberOfCars", 35);
	bConfineMouse = inireader.ReadInteger("Nuzlocke", "ConfineMouse", 0);

	CustomNumberOfLives = inireader.ReadInteger("CustomGame", "NumberOfLives", 2);
	bCustomAllowTrading = inireader.ReadInteger("CustomGame", "AllowTrading", 1);
	CustomLockedGameDifficulty = inireader.ReadInteger("CustomGame", "LockedGameDifficulty", 0);

	bIncreaseMiniworldSegments = inireader.ReadInteger("Misc", "IncreaseMiniworldSegments", 1);
}

//////////////////////////////////////////////////////////////////
// Inireader end
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Custom FE functions
//////////////////////////////////////////////////////////////////

// because the game uses __fastcall convention, we gotta do some asm magic/wrapping to make it work with __stdcall/__cdecl
unsigned int FEHashUpper_Addr = 0x004FD230;
unsigned int FEPkgMgr_FindPackage_Addr = 0x004F65D0;
unsigned int FEPackageManager_FindPackage_Addr = 0x004F3F90;
unsigned int FEngFindObject_Addr = 0x004FFB70;
unsigned int FEngSetButtonState_Addr = 0x004F5F80;
unsigned int FEngSwitchPackages_Addr = 0x004F6360;
unsigned int FEngSendMessageToPackage_Addr = 0x004C96C0;
unsigned int LaunchPartsBrowser_Addr = 0x00504990;
unsigned int uUserStartedCareer_Addr = 0x004B2AA0;

unsigned int __stdcall FEHashUpper(const char* str)
{
	unsigned int result = 0;
	_asm
	{
		mov edx, str
		call FEHashUpper_Addr
		mov result, eax
	}
	return result;
}

unsigned int __stdcall FEPkgMgr_FindPackage(const char* pkgname)
{
	unsigned int result = 0;
	_asm
	{
		mov eax, pkgname
		call FEPkgMgr_FindPackage_Addr
		mov result, eax
	}
	return result;
}

unsigned int __stdcall FEPackageManager_FindPackage(const char* pkgname)
{
	unsigned int result = 0;
	_asm
	{
		push 0x00746104
		mov eax, pkgname
		call FEPackageManager_FindPackage_Addr
		mov result, eax
		//add esp, 4
	}
	return result;
}

unsigned int __stdcall FEngFindObject(const char* pkg, int hash)
{
	unsigned int result = 0;
	_asm
	{
		mov ecx, hash
		mov edx, pkg
		call FEngFindObject_Addr
		mov result, eax
	}
	return result;
}

int FEngFindString(const char* pkgname, int objhash)
{
	int result; // r3

	result = FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), objhash);
	if (!result || *(unsigned int*)(result + 24) != 2)
		result = 0;
	return result;
}

void __stdcall FEngSetButtonState(const char* pkgname, unsigned int hash, unsigned int state)
{
	int cfeng_instance = *(int*)CFENG_PINSTANCE_ADDR;
	_asm
	{
		push state
		mov edi, hash
		mov eax, pkgname
		push cfeng_instance
		call FEngSetButtonState_Addr
		//add esp, 4
	}
}

unsigned int FEngSetInvisible_Addr = 0x000495F70;
void __stdcall FEngSetInvisible(const char* pkgname, unsigned int hash)
{
	_asm
	{
		mov edi, hash
		mov esi, pkgname
		call FEngSetInvisible_Addr
	}
}

unsigned int FEngSetVisible_Addr = 0x00495FC0;
void __stdcall FEngSetVisible(const char* pkgname, unsigned int hash)
{
	_asm
	{
		mov edi, hash
		mov esi, pkgname
		call FEngSetVisible_Addr
	}
}
#pragma runtime_checks( "", off )
unsigned int FEngSetCurrentButton_Addr = 0x004F5ED0;
void __stdcall FEngSetCurrentButton(const char* pkgname, unsigned int button_hash)
{
	_asm
	{
		mov eax, ds:CFENG_PINSTANCE_ADDR
		push eax
		mov ecx, button_hash
		mov eax, pkgname
		call FEngSetCurrentButton_Addr
	}
}
#pragma runtime_checks( "", restore )

void __stdcall FEngSwitchPackages(char* src, char* dest)
{
	_asm
	{
		push src
		mov edi, dest
		call FEngSwitchPackages_Addr
		add esp, 4
	}
}

void __stdcall FEngSendMessageToPackage(unsigned int msg, char* dest)
{
	_asm
	{
		push msg
		mov eax, dest
		call FEngSendMessageToPackage_Addr
		add esp, 4
	}
}

void __stdcall LaunchPartsBrowser(int eBrowserModes, void* FECarConfig, const char* src_pkg, const char* dest_pkg, int somebool)
{
	_asm
	{
		push somebool
		push dest_pkg
		mov ebx, src_pkg
		mov eax, FECarConfig
		push eBrowserModes
		call LaunchPartsBrowser_Addr
		add esp, 0xC
	}
}

bool __stdcall bUserStartedCareer()
{
	bool result = false;
	_asm
	{
		mov edx, 0x75EEF8
		xor ecx, ecx
		call uUserStartedCareer_Addr
		mov result, al
	}
	return result;
}

unsigned int sub_4C2D20 = 0x4C2D20;
void __stdcall UpdateCarPricing(unsigned int unk1, unsigned int unk2)
{
	_asm
	{
		mov ebx, unk1
		push unk2
		call sub_4C2D20
	}
}

unsigned int IsCarUnlocked_Addr = 0x005A1630;
bool __stdcall IsCarUnlocked(unsigned int hash, unsigned int careermanager)
{
	bool result_checker = false;
	_asm
	{
		mov edi, hash
		mov ebx, careermanager
		call IsCarUnlocked_Addr
		mov result_checker, al
	}
	return result_checker;
}

unsigned int UndergroundBriefScreen_Redraw_Addr = 0x004C5680;
void __stdcall UndergroundBriefScreen_Redraw(void* UndergroundBriefScreen)
{
	_asm
	{
		mov eax, UndergroundBriefScreen
		call UndergroundBriefScreen_Redraw_Addr
	}
}

// super hacky way of handling this, because the game doesn't actually use a __thiscall for this, but by sheer luck it passes the object through the ECX register
// RTC disabled for this function to stop the runtime from complaining about misaligned stack after the function call
#pragma runtime_checks( "", off )
void __stdcall FEngSetColor(void* FEObject, unsigned int color)
{
	_FEngSetColor(FEObject, color);
	_asm add esp, 4
}
#pragma runtime_checks( "", restore )

void __stdcall FE_SetColor_Str(const char* option_name, const char* pkgname, unsigned int color)
{
	FEngSetColor((void*)FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), FEHashUpper(option_name)), color);
}

void __stdcall FE_SetColor_Hash(unsigned int FEHash, const char* pkgname, unsigned int color)
{
	FEngSetColor((void*)FEngFindObject((char*)FEPkgMgr_FindPackage(pkgname), FEHash), color);
}

void __stdcall FE_SetButtonState_Str(const char* option_name, const char* pkgname, bool state)
{
	FEngSetButtonState(pkgname, FEHashUpper(option_name), state);
}

//////////////////////////////////////////////////////////////////
// Custom FE functions end
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Nuzlocke logic
//////////////////////////////////////////////////////////////////
unsigned int FindCarIndexByHash(unsigned int CarTypeNameHash)
{
	for (int i = 0; i < NumberOfCars; i++)
	{
		if (CarTypeNameHash == NuzCars[i].CarNameHash)
			return i;
	}
	return 0;
}

void UpdateCarUnlockStatus()
{
	CareerCarHash = *(int*)CAREER_CAR_TYPE_HASH_POINTER;
	unsigned int carcount = 0;
	unsigned int gamecarcount = 0;
	
	for (int i = 0; i < NumberOfCars; i++)
	{
		unsigned int Arg1_checker = NuzCars[i].CarNameHash;
		unsigned int Arg2_checker = FECAREERMANAGER_POINTER;

		NuzCars[i].bUnlocked = IsCarUnlocked(NuzCars[i].CarNameHash, FECAREERMANAGER_POINTER);

		if (NuzCars[i].bUnlocked && NuzCars[i].Lives && (NuzCars[i].UsageType == CAR_USAGE_TYPE_RACING))
			carcount++;
		if (NuzCars[i].bUnlocked && (NuzCars[i].UsageType == CAR_USAGE_TYPE_RACING))
			gamecarcount++;
	}
	UnlockedCarCount = carcount;
	GameUnlockedCarCount = gamecarcount;
}

bool bCheckIfAllCarsLocked()
{

	if (!bProfileStartedCareer)
	{
		if (DDayCar.Lives)
			return false;
		return true;
	}

	for (int i = 0; i < NumberOfCars; i++)
	{
		NuzCars[i].bUnlocked = IsCarUnlocked(NuzCars[i].CarNameHash, FECAREERMANAGER_POINTER);

		if (NuzCars[i].bUnlocked && NuzCars[i].Lives && (NuzCars[i].UsageType == CAR_USAGE_TYPE_RACING))
			return false;
	}
	return true;
}

unsigned int Arg1 = 0;
unsigned int Arg2 = 0;
bool PrevResult = false;
bool __stdcall IsCarUnlockedHook_Code()
{
	if (UnlockedCarCount <= 1) // if we have only 1 car unlocked, we MUST keep the current one unlocked so we can access the trade menu
	{
		if (Arg1 == CareerCarHash)
			return true;
	}

	if (NuzCars[FindCarIndexByHash(Arg1)].Lives <= 0)
		return false;
	return PrevResult;
}

void __declspec(naked) IsCarUnlockedHook()
{
	_asm
	{
		// take args and pass on the code normally
		mov Arg1, edi
		mov Arg2, ebx
		call IsCarUnlocked_Addr
		// save the result
		mov PrevResult, al
		// modify the result
		jmp IsCarUnlockedHook_Code
	}
}

void __stdcall BuildTradeableList_Hook(int unk)
{
	UpdateCarUnlockStatus();
	return BuildTradeableList(unk);
}

int DisableTradeMenu_Exit_True = 0x00504EBB;
int DisableTradeMenu_Exit_False = 0x00504E8C;
void __declspec(naked) DisableTradeMenu_Cave()
{
	if (UnlockedCarCount <= 1 || !bAllowTradingCarMidGame) // disable the "Trade" menu option if there's 1 or less cars unlocked
		_asm jmp DisableTradeMenu_Exit_True 
	_asm
	{
		mov edx, 0x006C1614
		or ecx, 0xFFFFFFFF
		jmp DisableTradeMenu_Exit_False
	}
}

void CheckAllCarPrices(unsigned int TradeCarObj)
{
	bool bLastOne = false;
	bool bCantAfford = true;

	float PriceDiff = 0.0;
	int IntPriceDiff = 0;
	unsigned int CarArg1 = *(unsigned int*)(TradeCarObj + 0x40);
	unsigned int CarArg2 = TradeCarObj + 0x90;
	unsigned int NextCarArg1 = 0;
	unsigned int LastCarArg1 = *(unsigned int*)(TradeCarObj + 0x44);

	do
	{
		UpdateCarPricing(CarArg1, CarArg2);
		PriceDiff = *(float*)(CarArg1 + 0xE64);
		NextCarArg1 = *(unsigned int*)(CarArg1);

		// if any price difference is a rebate (larger than 0) then the player obviously CAN afford a car and we immediately stop checking
		if (PriceDiff > 0)
		{
			bCantAfford = false;
			break;
		}

		// if any price difference is a cost, check if the player can afford it, and if so, stop checking immediately
		if (PriceDiff < 0)
		{
			IntPriceDiff = (int)floor(fabs(PriceDiff)); // game floors the value, so we do it too
			if (IntPriceDiff < Money)
			{
				bCantAfford = false;
				break;
			}
		}

		if (CarArg1 == LastCarArg1)
			bLastOne = true;

		CarArg1 = NextCarArg1;
	} while (!bLastOne);

	if (bCantAfford)
		bCantAffordAnyCar = true;
}

unsigned int __stdcall UndergroundTradeCarScreen_Constructor_Hook(void* Object, void* ScreenConstructorData)
{
	unsigned int result = UndergroundTradeCarScreen_Constructor(Object, ScreenConstructorData);

	// in case accessing this screen is allowed, we don't want to trigger the game over condition
	if (NuzCars[FindCarIndexByHash(CareerCarHash)].Lives <= 0)
		CheckAllCarPrices((unsigned int)Object);

	return result;
}

bool __stdcall HasEventBeenWon_hook(unsigned int arg1, unsigned int arg2)
{
	bool result = HasEventBeenWon(arg1, arg2);
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if ((GameMode == 1) && !IsThereANextRace((void*)arg1))
	{
		bMarkedStatusAlready = true;
		if (!result)
		{
			(*car).Lives--;
			(*car).Losses++;

			TotalLivesLost++;
			TotalLosses++;

			if ((*car).Lives <= 0)
			{
				// mark the time of death
				(*car).TimeOfDeathRT = TotalTimeSpentRacing;
				(*car).TimeOfDeathPT = TotalTimePlaying;
			}
		}

		if (result)
		{
			(*car).Wins++;
			TotalWins++;
		}
		TotalEventsPlayed++;
	}
	return result;
}

// any type of restarting is considered a loss EXCEPT if player had already won the event
bool __stdcall NotifyRestart_hook(unsigned int arg1, unsigned int arg2)
{
	bool result = HasEventBeenWon(arg1, arg2);
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if (GameMode == 1 && !result)
	{
		(*car).Lives--;
		(*car).Losses++;

		TotalLivesLost++;
		TotalLosses++;

		bMarkedStatusAlready = false;
		TotalEventsPlayed++;
	}

	bRaceFinished = false;

	return result;
}

// race quitting from PauseMenu = loss (EXCEPT if player had already won the event)
void __stdcall PauseMenu_DoQuitRace_Hook(int unk)
{
	int FERaceEvent = *(int*)(FEDATABASE_ADDR + 0x1E838);
	bool event_won = false;
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if (FERaceEvent)
		event_won = HasEventBeenWon(FERaceEvent, 0);

	if (GameMode == 1 && !event_won)
	{
		(*car).Lives--;
		(*car).Losses++;

		TotalLivesLost++;
		TotalLosses++;
		bMarkedStatusAlready = false;
		TotalEventsPlayed++;
	}

	bRaceFinished = false;

	return DoQuitRace(unk);
}

bool __stdcall PostRace_DoQuitRace_hook(unsigned int arg1, unsigned int arg2)
{
	bool result = HasEventBeenWon(arg1, arg2);
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if (GameMode == 1 && !result && !bMarkedStatusAlready) // to avoid double reduction, we must track if the race was finished previously
	{
		(*car).Lives--;
		(*car).Losses++;

		TotalLivesLost++;
		TotalLosses++;
		TotalEventsPlayed++;
	}

	if (GameMode == 1 && bMarkedStatusAlready)
		bMarkedStatusAlready = false;

	bRaceFinished = false;

	return result;
}

#pragma runtime_checks( "", off )
// if player is on last life, "Restart" is disabled in both PauseMenu and PostRace menus
// Hook for pause menus
void __stdcall FEngSetButtonState_Hook(unsigned int cfeng, unsigned int state)
{
	char* pkgname = 0;
	unsigned int hash = 0;
	_asm mov pkgname, eax
	_asm mov hash, edi

	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if ((*car).Lives <= 1 && (GameMode == 1))
	{
		FE_SetColor_Hash(0x1AEC7D0A, pkgname, 0xFF404040);
		return;
	}

	return FEngSetButtonState(pkgname, hash, state);

}
#pragma runtime_checks( "", restore )


unsigned int __stdcall PostRaceMenuScreen_Setup_Hook(unsigned int PostRaceMenuScreen, unsigned int PostRaceMenuSetupParams)
{
	char* pkgname = *(char**)(PostRaceMenuScreen + 0xC);
	unsigned int result = 0;
	unsigned int NumButtons = *(unsigned int*)PostRaceMenuSetupParams;
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if (((*car).Lives <= 1) && (GameMode == 1))
	{
		// specifically hunting 3 and 4 (and decreasing it) because
		// 1. there apparently is a case where it can have less than 3 buttons in post race screen (never seen it happen)
		// 2. this menu always picks the top option, so even IF we disable it, it gets selected, so this is a workaround
		// 3. Button 3 and 4 are always "Restart Race" and "Restart Tournament" respectively
		if ((NumButtons == 3) || (NumButtons == 4))
			*(unsigned int*)PostRaceMenuSetupParams = 2;

		result = PostRaceMenuScreen_Setup(PostRaceMenuScreen, PostRaceMenuSetupParams);
		FE_SetButtonState_Str("Button3", pkgname, 0);
		FE_SetButtonState_Str("Button4", pkgname, 0);
		FE_SetButtonState_Str("Button_3", pkgname, 0);
		FE_SetButtonState_Str("Button_4", pkgname, 0);

		FE_SetColor_Str("Button3", pkgname, 0xFF404040);
		FE_SetColor_Str("Button4", pkgname, 0xFF404040);
		//FE_SetColor_Str("Button_3", pkgname, 0xFF404040);
		//FE_SetColor_Str("Button_4", pkgname, 0xFF404040);
	}
	else
		result = PostRaceMenuScreen_Setup(PostRaceMenuScreen, PostRaceMenuSetupParams);

	return result;
}

#pragma runtime_checks( "", off )
void __stdcall UndergroundMenuScreen_NotificationMessage_Hook(unsigned int unk1, void* FEObject, unsigned int unk2, unsigned int unk3)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx

	UpdateCarUnlockStatus();

	if ((NuzCars[FindCarIndexByHash(CareerCarHash)].Lives <= 0) && (GameMode == 1) && !bGameIsOver)
	{
		// FORCE SWITCH TO TRADE CAR SCREEN
		FEngSendMessageToPackage(0x6C6603DF, "GarageMain.fng");
		LaunchPartsBrowser(1, (void*)0x75EEF8, *(char**)(thethis + 0xC), TradeCarScreen, 0);

		if (!bShownLifeOverOnce)
		{
			ImGui::OpenPopup(NUZLOCKE_HEADER_CARLIFE);
			bShowCarLifeHint = true;
			bShownLifeOverOnce = true;
		}

		return;
	}

	return UndergroundMenuScreen_NotificationMessage((void*)thethis, unk1, FEObject, unk2, unk3);
}
#pragma runtime_checks( "", restore )

char* ChooseCareerCar_CurrFNGName = NULL;
unsigned int ChooseCareerCar_EAX = 0;
unsigned int sub_4F0800 = 0x4F0800;
void __declspec(naked) ChooseCareerCar_Cave()
{
	_asm mov ChooseCareerCar_EAX, eax
	_asm mov ecx, [esi+8]
	_asm mov ChooseCareerCar_CurrFNGName, ecx

	bProfileStartedCareer = true;
	UpdateCarUnlockStatus();

	_asm
	{
		mov ecx, ChooseCareerCar_CurrFNGName
		mov eax, ChooseCareerCar_EAX
		push eax
		push ecx
		mov edi, 0x006C27C4
		call sub_4F0800
		pop edi
		pop esi
		retn 0x10
	}
}

// in_time is 4ms based, so 1*in_time = 4ms
// time spent racing
void __stdcall TimeCurrentCar(unsigned int in_time)
{
	bShowingRaceOver = false;
	int pointer = *(int*)PLAYER_POINTER;
	if (pointer)
	{
		pointer = *(int*)(pointer + 0x95C);
		if (pointer)
		{
			pointer = *(int*)(pointer + 0x30);
			if (pointer)
			{
				pointer = *(int*)(pointer + 0x18);
				if (pointer != 0)
					bShowingRaceOver = true;
			}
		}
	}


	NuzlockeStruct* car;
	
	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	if (!bShowingRaceOver)
	{
		(*car).TimeSpentRacing = in_time + LastCarTime;
		TotalTimeSpentRacing = in_time + LastTotalTime;
	}
}

// time spent in UG mode basically
void TimeTotalPlaytime()
{
	if (GameMode != OldGameMode)
	{
		// on turning to UG mode
		if (GameMode == 1)
		{
			// trigger one-time timebase start here
			TimeBaseSinceModeSwitch = timeGetTime();
			LastTotalPlayTime = TotalTimePlaying;
		}
		OldGameMode = GameMode;
	}

	if (GameMode == 1)
	{
		TimeSinceModeSwitch = timeGetTime() - TimeBaseSinceModeSwitch;
		TotalTimePlaying = TimeSinceModeSwitch + LastTotalPlayTime;
	}
}

#pragma runtime_checks( "", off )
// hook in RaceCoordinator::TheRaceHasFinished
bool GameFlowManager_IsPaused_Hook()
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx

	if (GameMode == 1)
	{
		bRaceFinished = true;
	}

	return GameFlowManager_IsPaused((void*)thethis);
}
#pragma runtime_checks( "", restore )

#pragma runtime_checks( "", off )
void __stdcall UndergroundBriefScreen_NotificationMessage_Hook(unsigned int msg, void* FEObject, unsigned int unk2, unsigned int unk3)
{
	unsigned int thethis = 0;
	_asm mov thethis, ecx
	char* pkgname = *(char**)(thethis + 0xC);

	if ((GameMode == 1) && !bGameIsOver && LockedGameDifficulty)
	{
		switch (LockedGameDifficulty)
		{
		case 2:
			FE_SetButtonState_Str("EASY", pkgname, 0);
			FE_SetButtonState_Str("MEDIUM", pkgname, 0);
			FE_SetColor_Str("EASY", pkgname, 0xFF404040);
			FE_SetColor_Str("MEDIUM", pkgname, 0xFF404040);
			break;
		case 1:
			FE_SetButtonState_Str("EASY", pkgname, 0);
			FE_SetColor_Str("EASY", pkgname, 0xFF404040);
			break;
			break;
		default:
			break;
		}
	}

	return UndergroundBriefScreen_NotificationMessage((void*)thethis, msg, FEObject, unk2, unk3);
}
#pragma runtime_checks( "", restore )
#pragma runtime_checks( "", off )
void __stdcall FEngSetCurrentButton_Hook_BriefScreen(unsigned int cfeng)
{
	char* pkgname = 0;
	_asm mov pkgname, eax
	unsigned int in_button = 0;
	_asm mov in_button, ecx

	unsigned int DifficultyHash = 0;
	switch (LockedGameDifficulty)
	{
	case 2:
		if ((in_button == 0x6BAA2200) || (in_button == 0x14DD31))
			in_button = 0x16821E;
		break;
	case 1:
		if ((in_button == 0x14DD31))
			in_button = 0x6BAA2200;
		break;
	default:
		break;
	}

	FEngSetCurrentButton(pkgname, in_button);
}
#pragma runtime_checks( "", restore )
// we must cave it in because we want to control it by a boolean variable easily
// entrypoint: 0x044AAF3
unsigned int TrafficRacersExit1True = 0x0044AA3F;
unsigned int TrafficRacersExit1False = 0x00044AAF9;
void __declspec(naked) TrafficRacersCave1()
{
	if (bTrafficRacers)
		_asm jmp TrafficRacersExit1True

	_asm
	{
		mov eax, ds:0x73609C
		push esi
		jmp TrafficRacersExit1False
	}
}

// entrypoint: 0x44AAA6
unsigned int bOMalloc_Addr = 0x00567160;
unsigned int TrafficRacersExit2True = 0x0044AA3F;
unsigned int TrafficRacersExit2False = 0x0044AAB0;
void __declspec(naked) TrafficRacersCave2()
{
	if (bTrafficRacers)
		_asm jmp TrafficRacersExit2True

	_asm
	{
		mov eax, ds:0x73609C
		call bOMalloc_Addr
		jmp TrafficRacersExit2False
	}
}

// entrypoint: 0x44AAC7
unsigned int TrafficRacersExit3True = 0x0044AA3F;
unsigned int TrafficRacersExit3False = 0x0044AAD1;
void __declspec(naked) TrafficRacersCave3()
{
	if (bTrafficRacers)
		_asm jmp TrafficRacersExit3True

	_asm
	{
		mov eax, ds:0x73609C
		call bOMalloc_Addr
		jmp TrafficRacersExit3False
	}
}

// entrypoint: 0x0044D817
unsigned int TrafficRacersExit4True = 0x0044D7D2;
unsigned int TrafficRacersExit4False = 0x0044D821;
void __declspec(naked) TrafficRacersCave4()
{
	if (bTrafficRacers)
		_asm jmp TrafficRacersExit4True

	_asm
	{
		mov eax, ds:0x73609C
		call bOMalloc_Addr
		jmp TrafficRacersExit4False
	}
}

// entrypoint: 0x0044D82E
unsigned int TrafficRacersExit5True = 0x0044D7D2;
unsigned int TrafficRacersExit5False = 0x0044D838;
void __declspec(naked) TrafficRacersCave5()
{
	if (bTrafficRacers)
		_asm jmp TrafficRacersExit5True

	_asm
	{
		mov eax, ds:0x73609C
		call bOMalloc_Addr
		jmp TrafficRacersExit5False
	}
}


void UGModeStart_Hook()
{
	bGameStarted = true;
	return sub_4DFD70();
}

void InitDDayCar()
{
	DDayCar.CarName = DDayCarName;
	DDayCar.bUnlocked = true;
	DDayCar.Lives = NumberOfLives;
	DDayCar.Wins = 0;
	DDayCar.Losses = 0;
	DDayCar.TimeSpentRacing = 0;
}

void ResetNuzlocke()
{
	for (int i = 0; i < NumberOfCars; i++)
	{
		NuzCars[i].Losses = 0;
		NuzCars[i].Wins = 0;
		NuzCars[i].Lives = NumberOfLives;
		NuzCars[i].TimeSpentRacing = 0;
	}
	InitDDayCar();
	bGameStarted = false;
	bGameIsOver = false;
	bGameComplete = false;
	bAllCarsLost = false;
	bCantAffordAnyCar = false;
	bRaceFinished = false;
	bMarkedStatusAlready = false;

	// shown once bools
	bShownGameOverOnce = false;
	bShownLifeOverOnce = false;

	TotalWins = 0;
	TotalLivesLost = 0;
	TotalLosses = 0;
	TotalTimeSpentRacing = 0;
	TotalTimePlaying = 0;
	LastCarTime = 0;
	LastTotalPlayTime = 0;
	LastTotalTime = 0;

	UpdateCarUnlockStatus();
}

void InitNuzlocke()
{
	// allocate the struct
	NuzCars = (NuzlockeStruct*)calloc(NumberOfCars, sizeof(NuzlockeStruct));
	// point the CarTypeInfoArray
	CarTypeInfoArray = (CarTypeInfo*)((*(int*)CARTYPEINFOARRAY_ADDRESS));
	// initialize params
	for (int i = 0; i < NumberOfCars; i++)
	{
		NuzCars[i].CarName = CarTypeInfoArray[i].CarTypeName;
		NuzCars[i].CarNameHash = CarTypeInfoArray[i].CarTypeNameHash;
		NuzCars[i].UsageType = CarTypeInfoArray[i].UsageType;
	}
	InitDDayCar();
}

void UpdateNuzGameStatus()
{
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;

	bAllCarsLost = bCheckIfAllCarsLocked();

	if (bAllCarsLost && bGameStarted)
	{
		bGameIsOver = true;
		if (!bShownGameOverOnce)
		{
			ImGui::OpenPopup(NUZLOCKE_HEADER_GAMEOVER);
			bShowGameOverScreen = true;
			bShownGameOverOnce = true;
		}
	}
	else if ((*car).Lives <= 0 && bCantAffordAnyCar && bGameStarted)
	{
		bGameIsOver = true;
		if (!bShownGameOverOnce)
		{
			ImGui::OpenPopup(NUZLOCKE_HEADER_GAMEOVER);
			bShowGameOverScreen = true;
			bShownGameOverOnce = true;
		}
	}
	else if (*(int*)CAREER_STATUS_ADDR == 2) // career completed
	{
		bGameComplete = true;
		bGameIsOver = true;
		if (!bShownGameOverOnce)
		{
			ImGui::OpenPopup(NUZLOCKE_HEADER_GAMEOVER);
			bShowGameOverScreen = true;
			bShownGameOverOnce = true;
		}
	}
	else
		bGameIsOver = false;

	if (!bGameIsOver)
	{
		if ((GameFlowStatus == 6) && (GameMode == 1) && *(int*)CURRENTRACE_POINTER_ADDR)
		{
			TimeCurrentCar(*(unsigned int*)((*(int*)CURRENTRACE_POINTER_ADDR) + 0x14));
		}

		if (*(int*)CURRENTRACE_POINTER_ADDR == 0)
		{
			LastCarTime = (*car).TimeSpentRacing;
			LastTotalTime = TotalTimeSpentRacing;
		}

		TimeTotalPlaytime();
	}

}

//////////////////////////////////////////////////////////////////
// Nuzlocke logic end
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// ImGui Code
//////////////////////////////////////////////////////////////////
void GenerateJoyEvents_Hook()
{
	if (bBlockedGameInput)
		return;
	return GenerateJoyEvents();
}

unsigned int __stdcall cFEng_GetMouseInfo_Hook(unsigned int FEMouseInfo)
{
	if (bBlockedGameInput)
		return FEMouseInfo;
	return cFEng_GetMouseInfo(FEMouseInfo);
}

float fl_round(float var)
{
	// 37.66666 * 100 =3766.66
	// 3766.66 + .5 =3767.16    for rounding off value
	// then type cast to int so value is 3767
	// then divided by 100 so the value converted into 37.67
	float value = (int)(var * 100 + .5);
	return (float)value / 100;
}

// 4ms variant (since the game's timer resolution is 4ms)
void CalcMinSecHuns4(unsigned int ct, int* mins, int* secs, int* huns)
{


	*mins = (int)(((float)ct / 4000.0) / 60.0);
	*secs = (int)(((float)ct / 4000.0) - (60.0 * (float)*mins));
	*huns = (int)((((float)ct / 4000.0) - floor((fl_round((float)ct / 4000.0)))) * 100);
}

void CalcMinSecHuns(unsigned int ct, int* mins, int* secs, int* huns)
{
	*mins = (int)(((float)ct / 1000.0) / 60.0);
	*secs = (int)(((float)ct / 1000.0) - (60.0 * (float)*mins));
	*huns = (int)((((float)ct / 1000.0) - floor((fl_round((float)ct / 1000.0)))) * 100);
}

void InitImgui()
{
	// Setup Dear ImGui context
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.IniFilename = NULL;
	io.LogFilename = NULL;
	ImFontConfig config;
	config.PixelSnapH = true;

	io.Fonts->AddFontFromFileTTF("scripts\\nuzlocke_fonts\\Conduit_ITC_Medium_Italic.ttf", 28.0);
	lcd_font = io.Fonts->AddFontFromFileTTF("scripts\\nuzlocke_fonts\\LCDN.ttf", 38.0, &config);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	g_pd3dDevice = (IDirect3DDevice9*)(*(int*)GAME_D3DDEVICE_ADDR);
	ImGui_ImplWin32_Init((HWND)(*(int*)GAME_HWND_ADDR));
	ImGui_ImplDX9_Init(g_pd3dDevice);

	bInitedImgui = true;
}

void DrawFEHUD()
{
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;
	char disp_string[64];
	sprintf(disp_string, "Car: %s // Lives: %d\nCars Available: %d/%d", (*car).CarName, (*car).Lives, UnlockedCarCount, GameUnlockedCarCount);

	float text_width = ImGui::CalcTextSize(disp_string).x + 8;
	float text_height = (ImGui::GetTextLineHeightWithSpacing() + 4) * 2;
	float max_width = 0; // we need to work with max 16:9 aspect ratio
	float ratio = 0;
	float width_diff = 0;

	// make a non-interactive window for status display in FE
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
									| ImGuiWindowFlags_NoMove
									| ImGuiWindowFlags_NoBringToFrontOnFocus
									| ImGuiWindowFlags_NoInputs
									| ImGuiWindowFlags_NoFocusOnAppearing
									| ImGuiWindowFlags_NoCollapse
									| ImGuiWindowFlags_NoScrollWithMouse
									| ImGuiWindowFlags_NoScrollbar;

	// put it in the upper-left corner of the FE (calculated from screen centre)
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ratio = main_viewport->Size.x / main_viewport->Size.y;
	// if we're working with a widescreen aspect ratio that is larger than 16:9 (ultrawide) then clamp the width to its 16:9 equivalent
	// this may be removed if UG1's FE gets revamped for widescreen and ultrawide
	if ((ratio > (1.777777)) && (main_viewport->Size.x > main_viewport->Size.y))
		max_width = main_viewport->Size.y * (1.777777);
	else
		max_width = main_viewport->Size.x;

	width_diff = (main_viewport->Size.x - max_width) / 2;

	ImGui::SetNextWindowSize(ImVec2(text_width, text_height));
	//ImGui::SetNextWindowPos(ImVec2(((max_width / 2) - (text_width / 2))* 0.3 + width_diff, 5));	
	ImGui::SetNextWindowPos(ImVec2(width_diff + 35.0, 5));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 6));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.047, 0.2, 0.282, 0.8));	

	ImGui::Begin("FEHud", NULL, window_flags);

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	//ImGui::SetWindowFontScale(FEHUD_FONT_SCALE);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.745, 0.843, 0.878, 1.0));
	ImGui::TextUnformatted(disp_string);
	ImGui::PopStyleColor();

	ImGui::End();
}

void DrawIGHUD()
{
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;


	int mins = 0;
	int sec = 0;
	int hun = 0;
	// this is solely used for precalculating the size, not for actual display
	sprintf(hud_disp_string, "Lives: %d\nCars: %d/%d\nRacing: %03d:%02d.%02d\nTotal: %04d:%02d.%02d", (*car).Lives, UnlockedCarCount, GameUnlockedCarCount, 888, 88, 88, 8888, 88, 88);

	float text_width = ImGui::CalcTextSize(hud_disp_string).x + 22 ;
	float text_height = ((ImGui::GetTextLineHeightWithSpacing() - 1) * 4);
	float max_width = 0; // we need to work with max 16:9 aspect ratio
	float width_scaler = 0;
	float width_diff = 0;
	float ratio = 0;
	float lcd_font_align = 7.5;

	ImVec4 dark_blue = ImVec4(0, 0.501, 0.752, 1.0);
	ImVec4 light_blue = ImVec4(0.584, 0.741, 0.992, 1.0);
	ImVec4 bg_color = ImVec4(0.0, 0.0, 0.0, 0.4);
	
	// make a non-interactive window for status display in game
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoInputs
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoScrollbar;


	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ratio = main_viewport->Size.x / main_viewport->Size.y;
	if (ratio >= 1.777777)
		width_scaler = 0.9;
	else
		width_scaler = 0.84375;

	// if we're working with a widescreen aspect ratio that is larger than 16:9 (ultrawide) then clamp the width to its 16:9 equivalent
	// this may be removed if UG1's FE gets revamped for widescreen and ultrawide
	if ((ratio > (1.777777)) && (main_viewport->Size.x > main_viewport->Size.y))
		max_width = main_viewport->Size.y * (1.777777);
	else
		max_width = main_viewport->Size.x;

	width_diff = (main_viewport->Size.x - max_width) / 2;

	//printf("max_width: &.2f\n", max_width);

	ImGui::SetNextWindowSize(ImVec2(text_width, text_height));
	// position = above tachometer
	//ImGui::SetNextWindowPos(ImVec2(((max_width) - (text_width / 2)) * width_scaler + width_diff, ((main_viewport->Size.y / 2) - (text_height / 2)) * 1.35));
	// NEW POSITION = bottom left corner - this decision was brought later because it didn't hide originally while it was paused, now it does, so the corner is free
	if (ratio > (1.3333333333))
		ImGui::SetNextWindowPos(ImVec2(width_diff + 45.0, main_viewport->Size.y - text_height - 35.0)); // widescreen fix position
	else
		ImGui::SetNextWindowPos(ImVec2(width_diff + 28.0, main_viewport->Size.y - text_height - 20.0)); // 4:3 position

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 6));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
	ImGui::Begin("IGHud", NULL, window_flags);
	ImGui::PopStyleColor();

	ImGui::PopStyleVar();
	ImGui::PopStyleVar();

	//ImGui::SetWindowFontScale(IGHUD_FONT_SCALE);

	ImGui::PushStyleColor(ImGuiCol_Text, light_blue);

	// LIVES
	// set darker color
	ImGui::PushStyleColor(ImGuiCol_Text, dark_blue);
	ImGui::TextUnformatted("Lives:");
	ImGui::PopStyleColor();

	ImGui::SameLine(0, 0);
	ImGui::PushFont(lcd_font);

	// manually aligning the text because the lcd font is... misaligned...
	ImVec2 curpos = ImGui::GetCursorPos();

	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));
	ImGui::Text("%d", (*car).Lives);
	ImGui::PopFont();

	// CARS AVAILABLE
	// align non-lcd font for newline
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));
	
	// set darker color
	ImGui::PushStyleColor(ImGuiCol_Text, dark_blue);
	ImGui::TextUnformatted("Cars:");
	ImGui::PopStyleColor();

	ImGui::SameLine(0, 0);
	ImGui::PushFont(lcd_font);
	// align lcd font
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));
	
	ImGui::Text("%d", UnlockedCarCount);
	ImGui::PopFont();
	// align non-lcd font
	ImGui::SameLine(0, 0);
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x + 4, curpos.y + lcd_font_align + 1.25));
	// set darker color
	ImGui::PushStyleColor(ImGuiCol_Text, dark_blue);
	ImGui::TextUnformatted("/");
	ImGui::PopStyleColor();
	// lcd font again
	ImGui::SameLine(0, 0);
	ImGui::PushFont(lcd_font);
	// align lcd font
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align - 1.25));

	ImGui::Text("%d", GameUnlockedCarCount);
	ImGui::PopFont();

	// RACE TIME
	// align non-lcd font for newline
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));

	// set darker color
	ImGui::PushStyleColor(ImGuiCol_Text, dark_blue);
	ImGui::TextUnformatted("Racing:");
	ImGui::PopStyleColor();

	ImGui::SameLine(0, 0);
	ImGui::PushFont(lcd_font);
	// align lcd font
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));

	CalcMinSecHuns4((*car).TimeSpentRacing, &mins, &sec, &hun);
	ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
	ImGui::PopFont();

	// TOTAL TIME
	// align non-lcd font for newline
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));

	// set darker color
	ImGui::PushStyleColor(ImGuiCol_Text, dark_blue);
	ImGui::TextUnformatted("Total:");
	ImGui::PopStyleColor();

	ImGui::SameLine(0, 0);
	ImGui::PushFont(lcd_font);
	// align lcd font
	curpos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(curpos.x, curpos.y - lcd_font_align));

	CalcMinSecHuns4(TotalTimeSpentRacing, &mins, &sec, &hun);
	ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
	ImGui::PopFont();

	ImGui::PopStyleColor();

	ImGui::End();
}

void __stdcall ShowDebugWindow()
{
	NuzlockeStruct* car;

	if (bProfileStartedCareer)
	{
		unsigned int ci = FindCarIndexByHash(CareerCarHash);
		car = &NuzCars[ci];
	}
	else
		car = &DDayCar;
	
	ImGui::Begin("NUZLOCKE DEBUG");
	ImGui::Text("Mouse X: %d Mouse Y: %d", *(int*)FEMOUSECURSOR_X_ADDR, *(int*)FEMOUSECURSOR_Y_ADDR);

	ImGui::Separator();
	if (ImGui::Button("Lose life"))
		(*car).Lives++;
	if (ImGui::Button("Add life"))
		(*car).Lives--;

	ImGui::End();
}

void ShowStatsWindow()
{
	unsigned int ci = FindCarIndexByHash(CareerCarHash);

	ImGui::Begin("Nuzlocke Statistics", &bShowStatsWindow);
	ImGui::Text("STATISTICS");
	ImGui::Separator();
	if (bProfileStartedCareer)
		ImGui::Text("Car: %s\nLives: %d\nWins: %d\nLosses: %d\n", NuzCars[ci].CarName, NuzCars[ci].Lives, NuzCars[ci].Wins, NuzCars[ci].Losses);
	else
		ImGui::Text("Car: %s\nLives: %d", DDayCar.CarName, DDayCar.Lives);
	ImGui::Separator();
	ImGui::Text("Cars available: %d/%d\nTotal wins: %d\nTotal losses: %d\nTotal lives lost: %d\nEvents played: %d\nCash: %d\nStyle points: %d", UnlockedCarCount, GameUnlockedCarCount, TotalWins, TotalLosses, TotalLivesLost, TotalEventsPlayed, Money, StylePoints);
	int mins = 0;
	int sec = 0;
	int hun = 0;
	CalcMinSecHuns4(TotalTimeSpentRacing, &mins, &sec, &hun);
	ImGui::Text("Total race time: %2d:%02d.%02d", mins, sec, hun);
	CalcMinSecHuns(TotalTimePlaying, &mins, &sec, &hun);
	ImGui::Text("Total play time: %2d:%02d.%02d", mins, sec, hun);

	if (bGameIsOver && bGameStarted)
	{
		ImGui::Separator();
		ImGui::Text("Game over!");
		if (bCantAffordAnyCar)
			ImGui::Text(NUZLOCKE_REASON_FINANCIAL);
		if (bAllCarsLost)
			ImGui::Text(NUZLOCKE_REASON_NOCARS);
		if (bGameComplete)
			ImGui::Text(NUZLOCKE_REASON_COMPLETE);
	}
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Per-car stats##StatsWindow", ImGuiTreeNodeFlags_None))
	{
		if (ImGui::BeginTable("car_table_stats", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Car");
			ImGui::TableSetupColumn("Lives");
			ImGui::TableSetupColumn("Unlocked");
			ImGui::TableSetupColumn("Wins");
			ImGui::TableSetupColumn("Losses");
			ImGui::TableSetupColumn("Time");
			ImGui::TableSetupColumn("Time of Death (RT)");
			ImGui::TableSetupColumn("Time of Death (PT)");
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(DDayCar.CarName);
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%d", DDayCar.Lives);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%d", DDayCar.bUnlocked);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%d", DDayCar.Wins);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%d", DDayCar.Losses);
			ImGui::TableSetColumnIndex(5);
			CalcMinSecHuns4(DDayCar.TimeSpentRacing, &mins, &sec, &hun);
			ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
			ImGui::TableSetColumnIndex(6);
			CalcMinSecHuns4(DDayCar.TimeOfDeathRT, &mins, &sec, &hun);
			ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
			ImGui::TableSetColumnIndex(7);
			CalcMinSecHuns(DDayCar.TimeOfDeathPT, &mins, &sec, &hun);
			ImGui::Text("%2d:%02d.%02d", mins, sec, hun);

			for (int i = 0; i < NumberOfCars; i++)
			{
				if (NuzCars[i].UsageType == CAR_USAGE_TYPE_RACING)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(NuzCars[i].CarName);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", NuzCars[i].Lives);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%d", NuzCars[i].bUnlocked);
					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%d", NuzCars[i].Wins);
					ImGui::TableSetColumnIndex(4);
					ImGui::Text("%d", NuzCars[i].Losses);
					ImGui::TableSetColumnIndex(5);
					CalcMinSecHuns4(NuzCars[i].TimeSpentRacing, &mins, &sec, &hun);
					ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
					ImGui::TableSetColumnIndex(6);
					CalcMinSecHuns4(NuzCars[i].TimeOfDeathRT, &mins, &sec, &hun);
					ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
					ImGui::TableSetColumnIndex(7);
					CalcMinSecHuns(NuzCars[i].TimeOfDeathPT, &mins, &sec, &hun);
					ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
				}
			}
			ImGui::EndTable();
		}
	}

	ImGui::End();
}

void ShowIntroMessage()
{
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowSize(ImVec2(800.0f, 0.0f));

	if (main_viewport->Size.y <= 1000.0f)
		ImGui::SetNextWindowPos(ImVec2((main_viewport->Size.x / 2) - 400.0f, 0.0f)); // put the dialog at center-top of the screen (if the screen is small in height -- TODO: implement screen scaling)

	if (ImGui::BeginPopupModal(NUZLOCKE_HEADER_INTRO, &bShowIntroMessage, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted(NUZLOCKE_INTRO_MSG);
		ImGui::PopTextWrapPos();
		ImGui::Separator();
		if (ImGui::TreeNode(NUZLOCKE_INTRO_RULES_TREEHEAD))
		{
			ImGui::PushTextWrapPos();
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_RULES_BP1);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_RULES_BP2);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_RULES_BP3);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_RULES_BP4);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_RULES_BP5);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_RULES_BP6);
			ImGui::PopTextWrapPos();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode(NUZLOCKE_INTRO_ADDNOTE_TREEHEAD))
		{
			ImGui::PushTextWrapPos();
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_ADDNOTE_BP1);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_ADDNOTE_BP2);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_ADDNOTE_BP3);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_ADDNOTE_BP4);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_ADDNOTE_BP5);
			ImGui::Bullet(); ImGui::TextUnformatted(NUZLOCKE_INTRO_ADDNOTE_BP6);
			ImGui::PopTextWrapPos();
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::Checkbox("Don't show again", &bSkipIntroMessage))
			UpdateIniFile();
		ImGui::Separator();
		if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
			ImGui::SetKeyboardFocusHere(0);
		if (ImGui::Button(NUZLOCKE_UI_CLOSE_TXT))
			bShowIntroMessage = false;
		ImGui::EndPopup();
	}
	if (!bShowIntroMessage && bProfileStartedCareer)
	{
		if (!bSkipAlreadyStartedWarning)
		{
			ImGui::OpenPopup(NUZLOCKE_HEADER_PROFILEWARNING);
			bShowAlreadyStartedWarning = true;
		}
		else
		{
			ImGui::OpenPopup(NUZLOCKE_HEADER_DIFFICULTY);
			bShowDifficultySelector = true;
		}
	}
	if (!bShowIntroMessage && !bProfileStartedCareer)
	{
		ImGui::OpenPopup(NUZLOCKE_HEADER_DIFFICULTY);
		bShowDifficultySelector = true;
	}
}

void ShowAlreadyLoadedWarning()
{
	ImGui::SetNextWindowSize(ImVec2(800.0, 0.0));
	if (ImGui::BeginPopupModal(NUZLOCKE_HEADER_PROFILEWARNING, &bShowAlreadyStartedWarning, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("WARNING");
		ImGui::Separator();
		ImGui::PushTextWrapPos();
		ImGui::Text(NUZLOCKE_ALREADYSTARTED_WARNING_MSG);
		ImGui::PopTextWrapPos();
		ImGui::Separator();
		if (ImGui::Checkbox("Don't show again", &bSkipAlreadyStartedWarning))
			UpdateIniFile();
		ImGui::Separator();
		if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
			ImGui::SetKeyboardFocusHere(0);
		if (ImGui::Button(NUZLOCKE_UI_CLOSE_TXT))
			bShowAlreadyStartedWarning = false;
		ImGui::EndPopup();
	}
	if (!bShowAlreadyStartedWarning)
	{
		ImGui::OpenPopup(NUZLOCKE_HEADER_DIFFICULTY);
		bShowDifficultySelector = true;
	}
}

void ShowGameOverScreen()
{
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	
	ImGui::SetNextWindowSize(ImVec2(800.0f, 0.0f));
	ImGui::SetNextWindowPos(ImVec2((main_viewport->Size.x / 2.0f) - 400.0f, 0.0f)); // put the dialog at center-top of the screen because car stats are giant and gamepad controls don't really allow for easy window moving
	
	if (ImGui::BeginPopupModal(NUZLOCKE_HEADER_GAMEOVER, &bShowGameOverScreen, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("GAME OVER");
		ImGui::Separator();
		
		if (bGameComplete)
		{
			ImGui::PushTextWrapPos();
			ImGui::Text(NUZLOCKE_REASON_COMPLETE);
			ImGui::PopTextWrapPos();
		}
		if (bAllCarsLost)
		{
			ImGui::PushTextWrapPos();
			ImGui::Text(NUZLOCKE_REASON_NOCARS);
			ImGui::PopTextWrapPos();
		}
		if (bCantAffordAnyCar)
		{
			ImGui::PushTextWrapPos();
			ImGui::Text(NUZLOCKE_REASON_FINANCIAL);
			ImGui::PopTextWrapPos();
		}
		
		ImGui::Separator();
		ImGui::Text("Cars available: %d/%d\nTotal wins: %d\nTotal losses: %d\nTotal lives lost: %d\nEvents played: %d\nCash: %d\nStyle points: %d", UnlockedCarCount, GameUnlockedCarCount, TotalWins, TotalLosses, TotalLivesLost, TotalEventsPlayed, Money, StylePoints);
		int mins = 0;
		int sec = 0;
		int hun = 0;
		CalcMinSecHuns4(TotalTimeSpentRacing, &mins, &sec, &hun);
		ImGui::Text("Total race time: %2d:%02d.%02d", mins, sec, hun);
		CalcMinSecHuns(TotalTimePlaying, &mins, &sec, &hun);
		ImGui::Text("Total play time: %2d:%02d.%02d", mins, sec, hun);
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Per-car stats##GameOverWindow", ImGuiTreeNodeFlags_None))
		{
			if (ImGui::BeginTable("car_table_gameover", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Car");
				ImGui::TableSetupColumn("Lives");
				ImGui::TableSetupColumn("Unlocked");
				ImGui::TableSetupColumn("Wins");
				ImGui::TableSetupColumn("Losses");
				ImGui::TableSetupColumn("Time");
				ImGui::TableSetupColumn("Time of Death (RT)");
				ImGui::TableSetupColumn("Time of Death (PT)");
				ImGui::TableHeadersRow();

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(DDayCar.CarName);
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%d", DDayCar.Lives);
				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%d", DDayCar.bUnlocked);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%d", DDayCar.Wins);
				ImGui::TableSetColumnIndex(4);
				ImGui::Text("%d", DDayCar.Losses);
				ImGui::TableSetColumnIndex(5);
				CalcMinSecHuns4(DDayCar.TimeSpentRacing, &mins, &sec, &hun);
				ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
				ImGui::TableSetColumnIndex(6);
				CalcMinSecHuns4(DDayCar.TimeOfDeathRT, &mins, &sec, &hun);
				ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
				ImGui::TableSetColumnIndex(7);
				CalcMinSecHuns(DDayCar.TimeOfDeathPT, &mins, &sec, &hun);
				ImGui::Text("%2d:%02d.%02d", mins, sec, hun);

				for (int i = 0; i < NumberOfCars; i++)
				{
					if (NuzCars[i].UsageType == CAR_USAGE_TYPE_RACING)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::TextUnformatted(NuzCars[i].CarName);
						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%d", NuzCars[i].Lives);
						ImGui::TableSetColumnIndex(2);
						ImGui::Text("%d", NuzCars[i].bUnlocked);
						ImGui::TableSetColumnIndex(3);
						ImGui::Text("%d", NuzCars[i].Wins);
						ImGui::TableSetColumnIndex(4);
						ImGui::Text("%d", NuzCars[i].Losses);
						ImGui::TableSetColumnIndex(5);
						CalcMinSecHuns4(NuzCars[i].TimeSpentRacing, &mins, &sec, &hun);
						ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
						ImGui::TableSetColumnIndex(6);
						CalcMinSecHuns4(NuzCars[i].TimeOfDeathRT, &mins, &sec, &hun);
						ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
						ImGui::TableSetColumnIndex(7);
						CalcMinSecHuns(NuzCars[i].TimeOfDeathPT, &mins, &sec, &hun);
						ImGui::Text("%2d:%02d.%02d", mins, sec, hun);
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::Separator();
		if (ImGui::Button(NUZLOCKE_UI_CLOSE_TXT))
			bShowGameOverScreen = false;
		ImGui::EndPopup();
	}
}

void ShowCarLifeHint()
{
	ImGui::SetNextWindowSize(ImVec2(800.0, 0.0));
	if (ImGui::BeginPopupModal(NUZLOCKE_HEADER_CARLIFE, &bShowCarLifeHint, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted(NUZLOCKE_CARLIFE_MSG);
		ImGui::PopTextWrapPos();
		ImGui::Separator();
		if (!ImGui::IsAnyItemFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
			ImGui::SetKeyboardFocusHere(0);
		if (ImGui::Button(NUZLOCKE_UI_CLOSE_TXT))
			bShowCarLifeHint = false;

		ImGui::EndPopup();
	}
}

void UpdateCustomDifficultySettings()
{
	NuzlockeDifficulty = NUZLOCKE_DIFF_CUSTOM;
	NumberOfLives = CustomNumberOfLives;
	bAllowTradingCarMidGame = bCustomAllowTrading;
	LockedGameDifficulty = CustomLockedGameDifficulty;
	UpdateIniFile();
}

void ShowDifficultySelector()
{
	ImGui::SetNextWindowSize(ImVec2(800.0, 0.0));
	if (ImGui::BeginPopupModal(NUZLOCKE_HEADER_DIFFICULTY, NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		bool bFocusedAlready = false;

		ImGui::PushTextWrapPos();
		ImGui::Text("Difficulty: %s\nNumber of lives: %d\nCar trading: %s\nMinimum game difficulty: %s\n", NuzDifficultyNames[NuzlockeDifficulty], NumberOfLives, CarTradingStatusNames[bAllowTradingCarMidGame], GameDifficultyNames[LockedGameDifficulty]);
		ImGui::PopTextWrapPos();
		ImGui::Separator();
		// EASY
		if (ImGui::Button(NUZLOCKE_UI_NUZ_DIFF_EASY))
		{
			bShowDifficultySelector = false;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemFocused())
		{
			NuzlockeDifficulty = NUZLOCKE_DIFF_EASY;
			NumberOfLives = NUZLOCKE_DIFF_EASY_LIVES;
			bAllowTradingCarMidGame = NUZLOCKE_DIFF_EASY_TRADING;
			LockedGameDifficulty = NUZLOCKE_DIFF_EASY_LOCKEDDIFF;
			bFocusedAlready = true;
		}
		// MEDIUM
		if (ImGui::Button(NUZLOCKE_UI_NUZ_DIFF_MEDIUM))
		{
			bShowDifficultySelector = false;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemFocused() && !bFocusedAlready)
		{
			NuzlockeDifficulty = NUZLOCKE_DIFF_MEDIUM;
			NumberOfLives = NUZLOCKE_DIFF_MEDIUM_LIVES;
			bAllowTradingCarMidGame = NUZLOCKE_DIFF_MEDIUM_TRADING;
			LockedGameDifficulty = NUZLOCKE_DIFF_MEDIUM_LOCKEDDIFF;
			bFocusedAlready = true;
		}

		// HARD
		if (ImGui::Button(NUZLOCKE_UI_NUZ_DIFF_HARD))
		{
			bShowDifficultySelector = false;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemFocused() && !bFocusedAlready)
		{
			NuzlockeDifficulty = NUZLOCKE_DIFF_HARD;
			NumberOfLives = NUZLOCKE_DIFF_HARD_LIVES;
			bAllowTradingCarMidGame = NUZLOCKE_DIFF_HARD_TRADING;
			LockedGameDifficulty = NUZLOCKE_DIFF_HARD_LOCKEDDIFF;
			bFocusedAlready = true;
		}

		// ULTRA HARD
		if (ImGui::Button(NUZLOCKE_UI_NUZ_DIFF_ULTRAHARD))
		{
			bShowDifficultySelector = false;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemFocused() && !bFocusedAlready)
		{
			NuzlockeDifficulty = NUZLOCKE_DIFF_ULTRAHARD;
			NumberOfLives = NUZLOCKE_DIFF_ULTRAHARD_LIVES;
			bAllowTradingCarMidGame = NUZLOCKE_DIFF_ULTRAHARD_TRADING;
			LockedGameDifficulty = NUZLOCKE_DIFF_ULTRAHARD_LOCKEDDIFF;
			bFocusedAlready = true;
		}

		// CUSTOM
		if (ImGui::Button(NUZLOCKE_UI_NUZ_DIFF_CUSTOM))
		{
			bShowDifficultySelector = false;
		}
		if (ImGui::IsItemHovered() || ImGui::IsItemFocused() && !bFocusedAlready)
		{
			UpdateCustomDifficultySettings();
			bFocusedAlready = true;
		}
		ImGui::Separator();
		ImGui::Checkbox("Use racer AI for traffic", &bTrafficRacers);
		ImGui::Separator();
		if (ImGui::CollapsingHeader("Custom settings", ImGuiTreeNodeFlags_None))
		{
			bool bChangedSetting = false;

			if (ImGui::InputInt("Lives", (int*)&CustomNumberOfLives, 1, 100, ImGuiInputTextFlags_CharsDecimal))
				bChangedSetting = true;

			if ((int)CustomNumberOfLives < 0)
				CustomNumberOfLives = 0;

			if (ImGui::Checkbox("Car trading", &bCustomAllowTrading))
				bChangedSetting = true;

			ImGui::TextUnformatted("Minimum game difficulty:");
			
			if (ImGui::RadioButton("Unlocked##Game", (int*)&CustomLockedGameDifficulty, 0))
				bChangedSetting = true;
			if (ImGui::RadioButton("Medium##Game", (int*)&CustomLockedGameDifficulty, 1))
				bChangedSetting = true;
			if (ImGui::RadioButton("Hard##Game", (int*)&CustomLockedGameDifficulty, 2))
				bChangedSetting = true;

			if (bChangedSetting)
				UpdateCustomDifficultySettings();
		}
		ImGui::EndPopup();
	}

	if (!bShowDifficultySelector)
	{
		ResetNuzlocke();
		if (bProfileStartedCareer) // if we've already started career, start the game immediately
			bGameStarted = true;
		else
			bGameStarted = false;
	}
}

void ShowWindows()
{
	//ShowDebugWindow();

	if (bGameIsOver && bShowGameOverScreen)
	{
		ShowGameOverScreen();
	}

	if ((GameFlowStatus == 3) && GameMode == 1 && !bHideFEHUD)
		DrawFEHUD();
	if ((GameFlowStatus == 6) && GameMode == 1 && !*(bool*)GAMEPAUSED_ADDR && !bHideIGHUD)
		DrawIGHUD();

	if (bShowStatsWindow)
		ShowStatsWindow();
	if (bShowIntroMessage)
		ShowIntroMessage();
	if (bShowAlreadyStartedWarning)
		ShowAlreadyLoadedWarning();
	if (bShowCarLifeHint)
		ShowCarLifeHint();
	if (bShowDifficultySelector)
		ShowDifficultySelector();
}


void ImguiUpdate()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImguiResetHook2()
{
	if (bInitedImgui)
		ImGui_ImplDX9_InvalidateDeviceObjects();

	sub_40A4E0();

	if (bInitedImgui)
		ImGui_ImplDX9_CreateDeviceObjects();
}

void ImguiRenderFrame()
{
	if (bInitedImgui)
	{
		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
}

unsigned int ImguiRenderCaveExit = 0x0040A6CB;
void __declspec(naked) ImguiRenderCave()
{
	_asm
	{
		call ImguiRenderFrame
		mov eax, ds:GAME_D3DDEVICE_ADDR
		mov ecx, [eax]
		jmp ImguiRenderCaveExit
	}
}

LRESULT WINAPI WndProcImgui(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_LBUTTONDOWN:
		bMousePressedDown = true;
		break;
	case WM_LBUTTONUP:
		bMousePressedDown = false;
		break;
	case WM_SETFOCUS:
		// confine mouse within the game window
		if (bConfineMouse)
		{
			GetWindowRect(*(HWND*)GAME_HWND_ADDR, &windowRect);
			ClipCursor(&windowRect);
		}
		break;
	}


	return GameWndProc(hWnd, msg, wParam, lParam);
	//return DefWindowProc(hWnd, msg, wParam, lParam);
}

void UpdateFECursorPos()
{
	bool bMouseInGameWindow = false;
	POINT MousePos;
	GetCursorPos(&MousePos);
	GetWindowRect(*(HWND*)GAME_HWND_ADDR, &windowRect);

	float ratio = 480.0 / (windowRect.bottom - windowRect.top); // scaling it to 480 height since that's what FE wants

	if ((MousePos.x >= windowRect.left) || (MousePos.x <= windowRect.right) && (MousePos.y >= windowRect.top) || (MousePos.y <= windowRect.bottom))
		bMouseInGameWindow = true;

	MousePos.x = MousePos.x - windowRect.left;
	MousePos.y = MousePos.y - windowRect.top;

	MousePos.x = (int)((float)(MousePos.x) * ratio);
	MousePos.y = (int)((float)(MousePos.y) * ratio);


	// car orbiting position calculation - always relative to old
	if (bMousePressedDown)
	{
		*(int*)FEMOUSECURSOR_CARORBIT_X_ADDR = MousePos.x - *(int*)FEMOUSECURSOR_X_ADDR;
		*(int*)FEMOUSECURSOR_CARORBIT_Y_ADDR = MousePos.y - *(int*)FEMOUSECURSOR_Y_ADDR;
	}

	if (!bMouseInGameWindow)
		SetCursor(LoadCursor(NULL, IDC_ARROW));

	// get time since last movement and hide it after a while (unless the cursor is within the game window so we don't hide it)
	if ((MousePos.x != *(int*)FEMOUSECURSOR_X_ADDR) || (MousePos.y != *(int*)FEMOUSECURSOR_Y_ADDR))
	{
		TimeSinceLastMouseMovement = timeGetTime();
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
	else
	{
		if ((TimeSinceLastMouseMovement + MOUSEHIDE_TIME) < timeGetTime())
			SetCursor(NULL);
	}

	*(int*)FEMOUSECURSOR_X_ADDR = MousePos.x;
	*(int*)FEMOUSECURSOR_Y_ADDR = MousePos.y;

	// track mouse click state - make sure it lasts for exactly 1 tick of a loop, because the game is a little overzelaous with reading this input
	if (bMousePressedDown != bMousePressedDownOldState)
	{
		*(bool*)FEMOUSECURSOR_BUTTONPRESS_ADDR = bMousePressedDown;
		*(bool*)FEMOUSECURSOR_BUTTONPRESS2_ADDR = bMousePressedDown;
		*(bool*)FEMOUSECURSOR_BUTTONPRESS3_ADDR = bMousePressedDown;
		bMousePressedDownOldState = bMousePressedDown;
	}
	else
	{
		//*(bool*)FEMOUSECURSOR_BUTTONPRESS_ADDR = false; // except this one, it's used for car orbiting
		*(bool*)FEMOUSECURSOR_BUTTONPRESS2_ADDR = false;
		*(bool*)FEMOUSECURSOR_BUTTONPRESS3_ADDR = false;
	}


}

// put "Return / Enter" to be the default accept button instead of "Space" to make it more convenient -- TODO: fix this
void ImguiIO_SetAcceptButton(ImGuiIO& io)
{
	 // using Win32 here since it responds better than ImGui (isn't overzelaous and quick with reading inputs)
	if (GetAsyncKeyState(VK_RETURN) & 1)
		io.NavInputs[ImGuiNavInput_Activate] = 1.0f;
}

//////////////////////////////////////////////////////////////////
// ImGui Code End
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// Common code
//////////////////////////////////////////////////////////////////

// Called after profile has changed
void OnProfileChange()
{
	ReadIniFile();

	bProfileStartedCareer = bUserStartedCareer();

	if (!bSkipIntroMessage)
	{
		ImGui::OpenPopup(NUZLOCKE_HEADER_INTRO);
		bShowIntroMessage = true;
	}
	else
	{
		ImGui::OpenPopup(NUZLOCKE_HEADER_DIFFICULTY);
		bShowDifficultySelector = true;
	}
}

#pragma runtime_checks( "", off )
// on profile creation
void NewProfile_Hook(unsigned int msg)
{
	unsigned int theint = 0;
	_asm mov theint, eax

	OnProfileChange();

	return FEngSendMessageToPackage(msg, (char*)theint);
}
#pragma runtime_checks( "", restore )

// on profile load
void ProfileLoad_Hook(void* block)
{
	OnProfileChange();

	return game_crt_free(block);
}

// Called during InitStomper -- late in InitializeEverything
void* Init_Hook(size_t size)
{
	InitImgui();
	InitNuzlocke();
	if (bConfineMouse)
	{
		// confine mouse within the game window
		GetWindowRect(*(HWND*)GAME_HWND_ADDR, &windowRect);
		ClipCursor(&windowRect);
	}

	return malloc(size);
}

// Called during MainLoop() before FEngUpdate
void __stdcall MainLoopHook()
{
	// do stuff you want to be executed in MainLoop here
	ImGuiIO& io = ImGui::GetIO();

	GameFlowStatus = *(int*)GAMEFLOWMANAGER_STATUS_ADDR;
	if (GameFlowStatus == 3)
		CareerCarHash = *(int*)CAREER_CAR_TYPE_HASH_POINTER;
	GameMode = *(int*)GAMEMODE_ADDR;
	RaceType = *(int*)RACETYPE_ADDR;
	Money = *(int*)PLAYERMONEY_ADDR;
	StylePoints = *(int*)THEGARAGEMANAGER_ADDR;

	if (!bBlockedGameInput)
		UpdateFECursorPos();

	// commented out because it breaks if a gamepad is connected...
	//ImguiIO_SetAcceptButton(io);

	ImguiUpdate();
	ShowWindows();
	UpdateNuzGameStatus();

	if (/*bAreAnyDialogsOpen() ||*/ io.WantCaptureKeyboard || io.WantCaptureMouse)
		bBlockedGameInput = true;
	else
		bBlockedGameInput = false;

	if (GetAsyncKeyState(VK_NEXT) & 0x01)
		bShowStatsWindow = true;

	// keep the return to the hooked function
	return sub_546780();
}

//////////////////////////////////////////////////////////////////
// Common code end
//////////////////////////////////////////////////////////////////

int Init()
{
	// main code hooks (init and loop)
	injector::MakeCALL(0x0059DFE3, Init_Hook, true);
	injector::MakeCALL(0x0043A376, MainLoopHook, true);
	// after profile load hook
	injector::MakeCALL(0x0041DB35, ProfileLoad_Hook, true);
	// after new profile hook
	injector::MakeCALL(0x00418829, NewProfile_Hook, true);

	injector::MakeCALL(0x004C2F94, BuildTradeableList_Hook, true);
	// hook in UndergroundTradeCarScreen::BuildTradeableList
	injector::MakeCALL(0x004C326E, IsCarUnlockedHook, true);

	// delete "Presets" entry from GarageScreen (to prevent cheating)
	injector::MakeJMP(0x00504E0A, 0x00504E7B, true);
	// add a check for last car & disable trade menu (unless otherwise specified)
	injector::MakeJMP(0x00504E84, DisableTradeMenu_Cave, true);

	// events of loss
	injector::MakeCALL(0x0049ED60, HasEventBeenWon_hook, true);
	injector::MakeCALL(0x005A2CA7, NotifyRestart_hook, true);
	injector::MakeCALL(0x005A2C66, NotifyRestart_hook, true);
	injector::MakeCALL(0x004CD9EE, PauseMenu_DoQuitRace_Hook, true);
	injector::MakeCALL(0x0049706E, PostRace_DoQuitRace_hook, true);

	// restart button disablers
	//injector::MakeJMP(0x004CE146, PauseMenuSetup_Cave1, true);
	//injector::MakeJMP(0x004CE398, PauseMenuSetup_Cave2, true);
	injector::MakeCALL(0x004CE151, FEngSetButtonState_Hook, true);
	injector::MakeCALL(0x004CE3A3, FEngSetButtonState_Hook, true);
	injector::MakeCALL(0x0049671E, PostRaceMenuScreen_Setup_Hook, true);
	injector::MakeCALL(0x004969A6, PostRaceMenuScreen_Setup_Hook, true);

	// hook in RaceCoordinator::TheRaceHasFinished to mark the end of race
	injector::MakeCALL(0x00423F60, GameFlowManager_IsPaused_Hook, true);

	// UndergroundMenu vtable hook
	injector::WriteMemory<unsigned int>(0x006C2D9C, (unsigned int)&UndergroundMenuScreen_NotificationMessage_Hook, true);

	// UndergroundBrief vtable hook for forced game difficulty setting
	injector::WriteMemory<unsigned int>(0x006C52E8, (unsigned int)&UndergroundBriefScreen_NotificationMessage_Hook, true);
	// hooks to prevent the button from being selected by default
	injector::MakeCALL(0x004C5CA7, FEngSetCurrentButton_Hook_BriefScreen, true);
	injector::MakeCALL(0x004C5CBF, FEngSetCurrentButton_Hook_BriefScreen, true);
	injector::MakeCALL(0x004C5CD7, FEngSetCurrentButton_Hook_BriefScreen, true);

	// ChooseCareerCar cave
	injector::MakeJMP(0x004E8A2F, ChooseCareerCar_Cave, true);
	// UG mode start hook (on new game)
	injector::MakeCALL(0x004B382D, UGModeStart_Hook, true);
	// TradeCarScreen hook -- for car price checking
	injector::MakeCALL(0x004C3649, UndergroundTradeCarScreen_Constructor_Hook, true);
	
	// FUN - traffic racer AI swap
	injector::MakeJMP(0x0044AAF3, TrafficRacersCave1, true);
	injector::MakeJMP(0x44AAA6, TrafficRacersCave2, true);
	injector::MakeJMP(0x44AAC7, TrafficRacersCave3, true);
	injector::MakeJMP(0x0044D817, TrafficRacersCave4, true);
	injector::MakeJMP(0x0044D82E, TrafficRacersCave5, true);

	// ImGui hooks
	// render hook
	injector::MakeJMP(0x0040A6C4, ImguiRenderCave, true);
	// reset device hook
	injector::MakeCALL(0x00401603, ImguiResetHook2, true);
	injector::MakeCALL(0x00401876, ImguiResetHook2, true);
	injector::MakeCALL(0x0040A70F, ImguiResetHook2, true);
	injector::MakeJMP(0x004087A9, ImguiResetHook2, true);
	injector::MakeJMP(0x004087C2, ImguiResetHook2, true);
	injector::MakeJMP(0x004087DB, ImguiResetHook2, true);
	injector::MakeJMP(0x004087F4, ImguiResetHook2, true);
	injector::MakeJMP(0x0040880D, ImguiResetHook2, true);
	injector::MakeJMP(0x004494D9, ImguiResetHook2, true);

	// dereference the current WndProc from the game executable and write to the function pointer (to maximize compatibility)
	GameWndProcAddr = *(unsigned int*)0x4088FC;
	GameWndProc = (LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM))GameWndProcAddr;
	injector::WriteMemory<unsigned int>(0x4088FC, (unsigned int)&WndProcImgui, true);

	// hook the joy event generator to block inputs when needed
	injector::MakeCALL(0x00447991, GenerateJoyEvents_Hook, true);
	injector::MakeCALL(0x0044794E, GenerateJoyEvents_Hook, true);

	// jump over cursor hiding and take control of it
	injector::MakeJMP(0x004089C3, 0x004089D1, true);
	// hook cFEng::GetMouseInfo in its vtable
	injector::WriteMemory<unsigned int>(0x006C1B5C, (unsigned int)&cFEng_GetMouseInfo_Hook, true);
	// disable DInput mouse -- we're gonna use Windows API to get the mouse position instead to avoid some bugs
	injector::MakeNOP(0x00408B04, 5, true);
	// disable PC_CURSOR texture to avoid duplicate cursors
	injector::WriteMemory<unsigned int>(0x004F30A1, 0, true);

	// read config
	ReadIniFile();

	// misc stuff, unrelated to the main functions of this mod but harmless
	if (bIncreaseMiniworldSegments)
	{
		*(int*)MINIWORLD_NUMVISIBLESEGMENTS_MAIN_ADDR = 10;
		*(int*)MINIWORLD_NUMVISIBLESEGMENTS_REF_ADDR = 5;
		*(int*)MINIWORLD_NUMVISIBLESEGMENTS_ENV_ADDR = 5;
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}
	return TRUE;
}
