/*	Renegade Scripts.dll
    Dragonade Game Manager
	Copyright 2012 Whitedragon, Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/

#include "general.h"
#include "engine.h"
#include "engine_da.h"
#include "da.h"
#include "da_game.h"
#include "da_settings.h"
#include "da_log.h"
#include "da_chatcommand.h"

#pragma warning(disable: 4073)
#pragma init_seg(lib)

DAGameModeFactoryClass *DAGameManager::GameMode = 0;
DynamicVectorClass<DAGameModeFactoryClass*> DAGameManager::GameModes;
bool DAGameManager::ShowGameModeTitle = true;
DynamicVectorClass<DAGameFeatureFactoryClass*> DAGameManager::GameFeatures;
bool DAGameManager::FirstMap = true;
bool DAGameManager::ShutdownPending = false;
StringClass DAGameManager::Map;

class TimeoutConsoleCommand : public ConsoleFunctionClass {
public:
	const char *Get_Name() { return "timeout"; }
	const char *Get_Help() { return "TIMEOUT - Ends the game by time limit expired."; }
	void Activate(const char *ArgumentsString) {
		if (!The_Game()->Get_Time_Limit_Minutes()) {
			The_Game()->Set_Time_Limit_Minutes(1u); //Needs a time limit or it won't timeout.
		}
		The_Game()->Set_Time_Remaining_Seconds(0); //Time out game by setting time remaining to 0.
	}
};

class TimeChatCommand : public DAChatCommandClass {
	bool Activate(cPlayer *Player,const DATokenClass &Text,TextMessageEnum ChatType) {
		DA::Host_Message("Time Elapsed: %s",Format_Time((unsigned long)The_Game()->Get_Game_Duration_S()));
		return true;
	}
};
DAChatCommandRegistrant<TimeChatCommand> TimeChatCommandRegistrant("!time");


void DAGameManager::Init() {
	static DAGameManager Instance;
	Instance.Register_Event(DAEvent::GAMEOVER,INT_MAX);
	Instance.Register_Event(DAEvent::LEVELLOADED,INT_MAX);
	Instance.Register_Event(DAEvent::SETTINGSLOADED,INT_MAX);
	ConsoleFunctionList.Add(new TimeoutConsoleCommand);
	Sort_Function_List();
	Verbose_Help_File();
}

void DAGameManager::Shutdown() {
	ShutdownPending = true;
	for (int i = 0;i < GameFeatures.Count();i++) {
		GameFeatures[i]->Destroy_Instance();
	}
	if (GameMode) {
		GameMode->Destroy_Instance();
	}
}

void DAGameManager::Game_Over_Event() {
	StringClass WinType;
	if (The_Game()->Get_Win_Type() == 0) {
		WinType = "server shutdown/gameover";
	}
	else if (The_Game()->Get_Win_Type() == 1) {
		WinType = "score limit reached";
	}
	else if (The_Game()->Get_Win_Type() == 2) {
		WinType = "high score when time limit expired";
	}
	else if (The_Game()->Get_Win_Type() == 3) {
		WinType = "building destruction";
	}
	else if (The_Game()->Get_Win_Type() == 4) {
		WinType = "pedestal beacon";
	}
	else {
		WinType = "unknown";
	}
	StringClass Message;
	Message.Format("Current game on map %s has ended. Game was won by %ls by %s, lasted %s, and had a total of %d player(s).",Get_Map(),Get_Wide_Team_Name(The_Game()->Get_Winner_ID()),WinType,Format_Time(The_Game()->Get_Game_Duration_S()),Get_Player_List()->Get_Count());
	DALogManager::Write_Log("_GENERAL","%s",Message);
	Console_Output("%s\n",Message);
	DALogManager::Write_Log("_GAMEOVER","%s %ls %u %d %s",Get_Map(),Get_Wide_Team_Name(The_Game()->Get_Winner_ID()),The_Game()->Get_Game_Duration_S(),Get_Player_List()->Get_Count(),WinType);
}

void DAGameManager::Level_Loaded_Event() {
	if (FirstMap) {
		INISection *Section = DASettingsManager::Get_Main_Settings()->Get_INI()->Get_Section("Startup_Commands");
		if (Section) {
			for (int i = 0;i < Section->Count();i++) {
				Console_InputF("%s %s",Section->Peek_Entry(i)->Entry,Section->Peek_Entry(i)->Value);
			}
		}
		DA::Host_Message("Running Dragonade %s with Scripts %.1f. Created by Black-Cell.net.",DA_VERSION,TT_VERSION);
		FirstMap = false;
	}
	Console_Output("Level %s Loaded OK\n",The_Game()->Get_Map_Name());
	DALogManager::Write_Log("_LEVEL","%s",The_Game()->Get_Map_Name());
	Map = The_Game()->Get_Map_Name();

	DASettingsManager::Reload();
	
	//Load Game Mode
	if (GameMode) {
		GameMode->Destroy_Instance();
		GameMode = 0;
	}
	//Unlike other settings the game mode needs to be loaded in the order: gamemode.ini map specific > da.ini map specific > gamemode.ini general > da.ini general.
	StringClass Name;
	DASettingsManager::Get_GameMode_Settings()->Get_INI()->Get_String(Name,The_Game()->Get_Map_Name(),"GameMode",DASettingsManager::Get_Main_Settings()->Get_INI()->Get_String(Name,The_Game()->Get_Map_Name(),"GameMode",DASettingsManager::Get_GameMode_Settings()->Get_INI()->Get_String(Name,"General","GameMode",DASettingsManager::Get_Main_Settings()->Get_INI()->Get_String(Name,"General","GameMode","AOW"))));
	DAGameModeFactoryClass *Factory = 0;
	
	if (Name == "Random") { //Select random game mode. Default to AOW if no other game modes exist or are usable on this map.
		DynamicVectorClass<DAGameModeFactoryClass*> ValidGameModes;
		for (int i = 0;i < GameModes.Count();i++) {
			if (GameModes[i]->Check_Map_Support()) {
				ValidGameModes.Add(GameModes[i]);
			}
		}
		int Rand = Commands->Get_Random_Int(0,ValidGameModes.Count());
		Factory = ValidGameModes[Rand];
	}
	else {
		DATokenParserClass Parser(Name,'|');
		DynamicVectorClass<DAGameModeFactoryClass*> ValidGameModes;
		while (const char *Token = Parser.Get_String()) {
			DAGameModeFactoryClass *Fac = Find_Game_Mode(Token);
			if (Fac && Fac->Check_Map_Support()) {
				ValidGameModes.Add(Fac);
			}
		}
		if (!ValidGameModes.Count()) {
			Factory = Find_Game_Mode("AOW");
			DALogManager::Write_Log("_ERROR","None of the selected game modes support map %s. Defaulting to AOW",The_Game()->Get_Map_Name());
		}
		else {
			int Rand = Commands->Get_Random_Int(0,ValidGameModes.Count());
			Factory = ValidGameModes[Rand];
		}
	}
	
	GameMode = Factory;
	DASettingsManager::Reload_GameMode();
	GameMode->Create_Instance();
	DASettingsManager::Post_Reload();
	
	StringClass Config;
	DASettingsClass("server.ini").Get_String(Config,"Server","Config","svrcfg_cnc.ini");
	StringClass Title;
	DASettingsClass(StringFormat("data\\%s",Config)).Get_INI()->Get_String(Title,"Settings","bGameTitle");

	ShowGameModeTitle = DASettingsManager::Get_Bool("ShowGameModeTitle",true);
	if (ShowGameModeTitle) {
		The_Game()->Get_Game_Title().Format(L"%hs %hs",Title,Factory->Get_Short_Name());
	}
	else {
		The_Game()->Get_Game_Title().Format(L"%hs",Title);
	}
	DA::Host_Message("Running in %s Mode.",GameMode->Get_Long_Name());
	DALogManager::Write_Log("_GAMEMODE","%s",GameMode->Get_Long_Name());
}

void DAGameManager::Settings_Loaded_Event() {
	//Load Game Features
	for (int i = 0;i < GameFeatures.Count();i++) {
		if (GameFeatures[i]->Get_Instance()) {
			if (!GameFeatures[i]->Check_Enabled()) {
				GameFeatures[i]->Destroy_Instance();
				if (GameFeatures[i]->Get_Name()) {
					Console_Output("Game feature %s unloaded: Disabled.\n",GameFeatures[i]->Get_Name());
					DALogManager::Write_Log("_GAMEFEATURE","unload %s",GameFeatures[i]->Get_Name());
				}
			}
			else if (!GameFeatures[i]->Check_Map_Support()) {
				GameFeatures[i]->Destroy_Instance();
				if (GameFeatures[i]->Get_Name()) {
					Console_Output("Game feature %s unloaded: Map %s not supported.\n",GameFeatures[i]->Get_Name(),The_Game()->Get_Map_Name());
					DALogManager::Write_Log("_GAMEFEATURE","unload %s",GameFeatures[i]->Get_Name());
				}
			}
		}
		else if (GameFeatures[i]->Check_Enabled() && GameFeatures[i]->Check_Map_Support()) {
			GameFeatures[i]->Create_Instance();
			if (GameFeatures[i]->Get_Name()) {
				Console_Output("Game feature %s loaded.\n",GameFeatures[i]->Get_Name());
				DALogManager::Write_Log("_GAMEFEATURE","load %s",GameFeatures[i]->Get_Name());
			}
		}
	}


	//svrcfg_cnc.ini settings
	StringClass Config;
	DASettingsClass("server.ini").Get_String(Config,"Server","Config","svrcfg_cnc.ini");
	DASettingsClass SvrCfg(StringFormat("data\\%s",Config));

	The_Cnc_Game()->TimeLimit_Minutes = DASettingsManager::Get_Int("TimeLimitMinutes",SvrCfg.Get_Int("Settings","TimeLimitMinutes",30));
	if (!The_Game()->Get_Game_Duration_S()) {
		The_Cnc_Game()->Reset_Time_Remaining_Seconds();
	}
	The_Cnc_Game()->RadarMode = DASettingsManager::Get_Int("RadarMode",SvrCfg.Get_Int("Settings","RadarMode",1));
	The_Cnc_Game()->IsLaddered = DASettingsManager::Get_Bool("IsLaddered",SvrCfg.Get_Bool("Settings","IsLaddered",true));
	The_Cnc_Game()->CanRepairBuildings = DASettingsManager::Get_Bool("CanRepairBuildings",SvrCfg.Get_Bool("Settings","CanRepairBuildings",true));
	The_Cnc_Game()->SpawnWeapons = DASettingsManager::Get_Bool("SpawnWeapons",SvrCfg.Get_Bool("Settings","SpawnWeapons",true));
	The_Cnc_Game()->IsFriendlyFirePermitted = DASettingsManager::Get_Bool("IsFriendlyFirePermitted",SvrCfg.Get_Bool("Settings","IsFriendlyFirePermitted",false));
	The_Cnc_Game()->IsTeamChangingAllowed = DASettingsManager::Get_Bool("IsTeamChangingAllowed",SvrCfg.Get_Bool("Settings","IsTeamChangingAllowed",false));
	The_Cnc_Game()->BaseDestructionEndsGame = DASettingsManager::Get_Bool("BaseDestructionEndsGame",SvrCfg.Get_Bool("Settings","BaseDestructionEndsGame",true));
	The_Cnc_Game()->BeaconPlacementEndsGame = DASettingsManager::Get_Bool("BeaconPlacementEndsGame",SvrCfg.Get_Bool("Settings","BeaconPlacementEndsGame",true));
	The_Cnc_Game()->StartingCredits = DASettingsManager::Get_Int("StartingCredits",SvrCfg.Get_Int("Settings","StartingCredits",0));
	Update_Game_Settings();
}

DAGameModeFactoryClass *DAGameManager::Find_Game_Mode(const char *Name) {
	for (int i = 0;i < GameModes.Count();i++) {
		if (!_stricmp(GameModes[i]->Get_Short_Name(),Name)) {
			return GameModes[i];
		}
	}
	return 0;
}

void DAGameManager::Add_Game_Mode(DAGameModeFactoryClass *Factory) {
	for (int i = 0;i < GameModes.Count();i++) {
		if (!_stricmp(Factory->Get_Short_Name(),GameModes[i]->Get_Short_Name())) {
			GameModes.Delete(i);
			break;
		}
	}
	GameModes.Add(Factory);
}

void DAGameManager::Add_Game_Feature(DAGameFeatureFactoryClass* Factory) {
	if (Factory->Get_Name()) {
		for (int i = 0;i < GameFeatures.Count();i++) {
			if (GameFeatures[i]->Get_Name() && !_stricmp(Factory->Get_Name(),GameFeatures[i]->Get_Name())) {
				GameFeatures.Delete(i);
				break;
			}
		}
	}
	GameFeatures.Add(Factory);
}

/********************************************************************************************************************************/

void DAGameModeFactoryClass::Add_Game_Mode() {
	DAGameManager::Add_Game_Mode(this);
}

bool DAGameModeFactoryClass::Check_Map_Support() const {
	if (!RequiredData) {
		return true;
	}
	StringClass Buffer;
	DASettingsClass(StringFormat("da_%s.ini",Get_Short_Name())).Get_INI()->Get_String(Buffer,The_Game()->Get_Map_Name(),RequiredData);
	return !Buffer.Is_Empty();
}

void DAGameModeFactoryClass::Destroy_Instance() {
	delete Instance;
	Instance = 0;
}

/********************************************************************************************************************************/

void DAGameFeatureFactoryClass::Add_Game_Feature() {
	DAGameManager::Add_Game_Feature(this);
}

bool DAGameFeatureFactoryClass::Check_Map_Support() const {
	if (!RequiredData) {
		return true;
	}
	StringClass Buffer;
	DASettingsManager::Get_String(Buffer,RequiredData,0);
	return !Buffer.Is_Empty();
}

bool DAGameFeatureFactoryClass::Check_Enabled() const {
	if (!EnableSetting) {
		return true;
	}
	StringClass Buffer;
	DASettingsManager::Get_String(Buffer,EnableSetting,"0");
	return (Buffer != "0" && Buffer != "false");
}

void DAGameFeatureFactoryClass::Destroy_Instance() {
	delete Instance;
	Instance = 0;
}