/*	Renegade Scripts.dll
    Dragonade Domination Game Mode
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
#include "scripts.h"
#include "engine.h"
#include "engine_DA.h"
#include "da.h"
#include "da_domination.h"
#include "da_levelcleaner.h"
#include "da_settings.h"
#include "da_string.h"
#include "GameObjManager.h"
#include "PurchaseSettingsDefClass.h"
#include "TeamPurchaseSettingsDefClass.h"
#include "cTeam.h"

void DAControlNodeClass::Init(const INIClass *INI,const StringClass &Header) {
	Preset = DADominationManager->Get_Control_Node_Preset();
	Model = DADominationManager->Get_Control_Node_Model();
	Animation = DADominationManager->Get_Control_Node_Animation();
	DASpawnNodeClass::Init(INI,Header);
}

void DAControlNodeClass::Timer_Expired(int Number,unsigned int Data) {
	if (Number == 10) {
		DADominationManager->Increment_Score(Team);
	}
	else {
		DASpawnNodeClass::Timer_Expired(Number,Data);
	}
}

void DAControlNodeClass::Captured_Callback() {
	DASpawnNodeClass::Captured_Callback();
	Stop_Timer(10);
	Start_Timer(10,DADominationManager->Get_Point_Tick_Time(),true);
	DA::Create_2D_Sound_Team(Team,"M00EVAG_DSGN0048I1EVAG_snd.wav");
	DADominationManager->Update_Node_Count();
}

void DAControlNodeClass::Set_Contested_Callback() {
	if (Is_Contested()) {
		Stop_Timer(10);
	}
	else {
		Stop_Timer(10);
		Start_Timer(10,DADominationManager->Get_Point_Tick_Time(),true);
	}
	DASpawnNodeClass::Set_Contested_Callback();
}

void DATiberiumNodeClass::Init(const INIClass *INI,const StringClass &Header) {
	Preset = DADominationManager->Get_Tiberium_Node_Preset();
	Model = DADominationManager->Get_Tiberium_Node_Model();
	Animation = DADominationManager->Get_Tiberium_Node_Animation();
	DABaseNodeClass::Init(INI,Header);
}

void DATiberiumNodeClass::Timer_Expired(int Number,unsigned int Data) {
	if (Number == 10) {
		DADominationManager->Increment_Credits(Team);
	}
	else {
		Stop_Timer(10);
		DABaseNodeClass::Timer_Expired(Number,Data);
	}
}

void DATiberiumNodeClass::Captured_Callback() {
	Stop_Timer(10);
	Start_Timer(10,DADominationManager->Get_Credit_Tick_Time(),true);
	DA::Create_2D_Sound_Team(Team,"M00EVAG_DSGN0049I1EVAG_snd.wav");
	DADominationManager->Update_Node_Count();
}

void DATiberiumNodeClass::Set_Contested_Callback() {
	if (Is_Contested()) {
		Stop_Timer(10);
	}
	else {
		Start_Timer(10,DADominationManager->Get_Credit_Tick_Time(),true);
	}
}

void DADominationManagerClass::Init() {
	Score[0] = 0.0f;
	Score[1] = 0.0f;
	
	const DASettingsClass *Settings = DASettingsManager::Get_GameMode_Settings();
	Settings->Get_String(ControlNodePreset,"ControlNodePreset",0);
	Settings->Get_String(ControlNodeModel,"ControlNodeModel",0);
	Settings->Get_String(ControlNodeAnimation,"ControlNodeAnimation",0);

	Settings->Get_String(TiberiumNodePreset,"TiberiumNodePreset",0);
	Settings->Get_String(TiberiumNodeModel,"TiberiumNodeModel",0);
	Settings->Get_String(TiberiumNodeAnimation,"TiberiumNodeAnimation",0);

	PointTickAmount = Settings->Get_Float("DominationPointTickAmount",10.0f);
	PointTickTime = Settings->Get_Float("DominationPointTickTime",10.0f);
	CreditTickAmount = Settings->Get_Float("DominationCreditTickAmount",10.0f);
	CreditTickTime = Settings->Get_Float("DominationCreditTickTime",10.0f);

	WinningPoints = Settings->Get_Float("DominationWinningPoints",1000.0f);

	if ((The_Game()->MapName[0] == 'M' || The_Game()->MapName[0] == 'm')) {
		DALevelCleaner::Clean_Level();
		DALevelCleaner::Load_Blockers(Settings->Get_INI());
	}

	if (Settings->Get_Float(The_Game()->MapName,"GDI_Spawn1_X",0)) {
		DASpawnManager->Create_Instance()->Init(Settings->Get_INI());
		DASpawnManager->Create_Spawn_Point(Settings->Get_INI(),"Nod"); //Starting spawners
		DASpawnManager->Create_Spawn_Point(Settings->Get_INI(),"GDI");
	}
	if (Settings->Get_Float(The_Game()->MapName,"GDI_Vehicle_X",0)) {
		DAVehicleSpawnManager->Create_Instance()->Init(Settings->Get_INI());
		DAVehicleSpawnManager->Create_Air_Drop_Point(Settings->Get_INI(),"Nod"); //Vehicle spawn locations
		DAVehicleSpawnManager->Create_Air_Drop_Point(Settings->Get_INI(),"GDI");
	}
	if (Settings->Get_Float(The_Game()->MapName,"Teleporter1_TeleporterEntrance_X",0)) {
		DATeleporterManager->Create_Instance();
		for (int i = 1;;i++) {
			StringClass Header;
			Header.Format("Teleporter%d",i);
			StringClass X;
			Settings->Get_String(X,The_Game()->MapName,StringFormat("%s_TeleporterEntrance_X",Header),0);
			if (!X.Is_Empty()) {
				DATeleporterManager->Create_Teleporter(Settings->Get_INI(),Header);
			}
			else {
				break;
			}
		}
	}
	
	DANodeManager->Create_Instance()->Init(Settings->Get_INI());
	
	for (int i = 1;;i++) {
		StringClass Header;
		Header.Format("Control%d",i);
		StringClass X;
		Settings->Get_String(X,The_Game()->MapName,StringFormat("%s_Node_X",Header),0);
		if (!X.Is_Empty()) {
			DANodeManager->Create_Node<DAControlNodeClass>(Settings->Get_INI(),Header);
		}
		else {
			break;
		}
	}
	for (int i = 1;;i++) {
		StringClass Header;
		Header.Format("Tiberium%d",i);
		StringClass X;
		Settings->Get_String(X,The_Game()->MapName,StringFormat("%s_Node_X",Header),0);
		if (!X.Is_Empty()) {
			DANodeManager->Create_Node<DATiberiumNodeClass>(Settings->Get_INI(),Header);
		}
		else {
			break;
		}
	}
	
	//Change Stealth Tanks into Recon Bikes.
	PurchaseSettingsDefClass *PT = PurchaseSettingsDefClass::Find_Definition(PurchaseSettingsDefClass::TYPE_VEHICLES,PurchaseSettingsDefClass::TEAM_NOD);
	VehicleGameObjDef *Recon = (VehicleGameObjDef*)Find_Named_Definition("CnC_Nod_Recon_Bike");
	Recon->WeaponDefID = Get_Definition_ID("Weapon_StealthTank_Player");
	const_cast<DefenseObjectDefClass&>(Recon->Get_DefenseObjectDef()).Skin = ArmorWarheadManager::Get_Armor_Type("CNCVehicleHeavy");
	const_cast<DefenseObjectDefClass&>(Recon->Get_DefenseObjectDef()).ShieldType = ArmorWarheadManager::Get_Armor_Type("CNCVehicleHeavy");
	PT->Set_Definition(5,Recon->Get_ID());
	PT->Set_Cost(5,500);

	//Change beacons into Remote C4.
	TeamPurchaseSettingsDefClass *PT2 = TeamPurchaseSettingsDefClass::Get_Definition(TeamPurchaseSettingsDefClass::TEAM_NOD);
	PT2->Set_Beacon_Definition(Get_Definition_ID("CnC_POW_MineRemote_02"));
	PT2->Set_Beacon_Cost(200);
	PT2 = TeamPurchaseSettingsDefClass::Get_Definition(TeamPurchaseSettingsDefClass::TEAM_GDI);
	PT2->Set_Beacon_Definition(Get_Definition_ID("CnC_POW_MineRemote_02"));
	PT2->Set_Beacon_Cost(200);
	
	Register_Event(DAEvent::PLAYERLOADED);
	Register_Object_Event(DAObjectEvent::CREATED,DAObjectEvent::PLAYER);
	Register_Object_Event(DAObjectEvent::KILLDEALT,DAObjectEvent::PLAYER,INT_MIN);
	Register_Object_Event(DAObjectEvent::DAMAGEDEALT,DAObjectEvent::PLAYER,INT_MIN);
	
	Start_Timer(10,CreditTickTime,true);
}

void DADominationManagerClass::Player_Loaded_Event(cPlayer *Player) {
	//Send updated stuff to new players when they join.
	cTeam *Nod = Find_Team(0);
	cTeam *GDI = Find_Team(1);
	Nod->Set_Object_Dirty_Bit(NetworkObjectClass::BIT_RARE,true);
	Nod->Set_Object_Dirty_Bit(NetworkObjectClass::BIT_OCCASIONAL,true);
	GDI->Set_Object_Dirty_Bit(NetworkObjectClass::BIT_RARE,true);
	GDI->Set_Object_Dirty_Bit(NetworkObjectClass::BIT_OCCASIONAL,true);
	Send_Info_Message(Player->Get_ID());
}

void DADominationManagerClass::Object_Created_Event(GameObject *obj) {
	Update_Node_Count();
	((SoldierGameObj*)obj)->Give_Key(1);
	((SoldierGameObj*)obj)->Give_Key(2);
	((SoldierGameObj*)obj)->Give_Key(3);
	if (!_stricmp(obj->Get_Definition().Get_Name(),"CnC_Nod_FlameThrower_2SF")) { //Change Stealth Black Hands into Black Hand Rocket Soldier.
		Change_Character(obj,"CnC_Nod_RocketSoldier_2SF");
		WeaponBagClass *Bag = ((SoldierGameObj*)obj)->Get_Weapon_Bag();
		Bag->Remove_Weapon("Weapon_LaserChaingun_Player");
		WeaponDefinitionClass *Weapon = (WeaponDefinitionClass*)Find_Named_Definition("CnC_Weapon_RocketLauncher_Player");
		Bag->Add_Weapon(Weapon,999,true);
		Bag->Select_Weapon_Name("CnC_Weapon_RocketLauncher_Player");
	}
}

void DADominationManagerClass::Damage_Event(DamageableGameObj *Victim,ArmedGameObj *Damager,float Damage,unsigned int Warhead,DADamageType::Type Type,const char *Bone) {
	Update_Score(Damager->Get_Player_Type()?1:0); //Remove player score from team total.
}

void DADominationManagerClass::Kill_Event(DamageableGameObj *Victim,ArmedGameObj *Killer,float Damage,unsigned int Warhead,DADamageType::Type Type,const char *Bone) {
	int Team = Killer->Get_Player_Type();
	if (Team == 0 || Team == 1) {
		Update_Score(Team); //Remove player score from team total.
		if (Victim != Killer && Victim->As_SoldierGameObj()) {
			Find_Team(Team)->Dec_Kills(); //Remove player kill from team total.
		}
	}
}

void DADominationManagerClass::Timer_Expired(int Number,unsigned int Data) {
	if (Number == 10) {
		Increment_Credits(0);
		Increment_Credits(1);
	}
	else if (Number == 11) {
		Console_Input("timeout");
	}
}

void DADominationManagerClass::Set_Winner(int Winner) {
	if (Winner == 0) {
		DA::Create_2D_Sound_Team(0,"M00EVAN_DSGN0077I1EVAN_snd.wav");
		DA::Create_2D_Sound_Team(1,"M00EVAG_DSGN0011I1EVAG_snd.wav");
	}
	else if (Winner == 1) {
		DA::Create_2D_Sound_Team(0,"M00EVAN_DSGN0078I1EVAN_snd.wav");
		DA::Create_2D_Sound_Team(1,"M00EVAG_DSGN0005I1EVAG_snd.wav");
	}
	DA::Host_Message("%ls has won the game!",Get_Wide_Team_Name(Winner));

	int ControlNodeCount[2];
	ControlNodeCount[0] = 0;
	ControlNodeCount[1] = 0;

	int TiberiumNodeCount[2];
	TiberiumNodeCount[0] = 0;
	TiberiumNodeCount[1] = 0;

	for (int i = 0;i < DANodeManager->Get_Node_Count();i++) {
		DABaseNodeClass *Node = DANodeManager->Get_Node(i);
		Node->Unregister_Event(DAEvent::THINK);
		Node->Stop_Timer(10);
		if (!_stricmp(Node->Get_Type(),ControlNodeType)) {
			if (Node->Get_Team() == 0) {
				ControlNodeCount[0]++;
			}
			else if (Node->Get_Team() == 1) {
				ControlNodeCount[1]++;
			}
		}
		else if (!_stricmp(Node->Get_Type(),TiberiumNodeType)) {
			if (Node->Get_Team() == 0) {
				TiberiumNodeCount[0]++;
			}
			else if (Node->Get_Team() == 1) {
				TiberiumNodeCount[1]++;
			}
		}
	}

	DA::Host_Message("%ls controls %d Tiberium Node(s), %d Control Node(s), and has a score of %d.",Get_Wide_Team_Name(0),TiberiumNodeCount[0],ControlNodeCount[0],(int)Score[0]);
	DA::Host_Message("%ls controls %d Tiberium Node(s), %d Control Node(s), and has a score of %d.",Get_Wide_Team_Name(1),TiberiumNodeCount[1],ControlNodeCount[1],(int)Score[1]);

	Stop_Timer(10);
	Start_Timer(11,5.0f,false,Winner?0:1);
}

void DADominationManagerClass::Update_Node_Count() {
	int ControlNodeCount[2];
	ControlNodeCount[0] = 0;
	ControlNodeCount[1] = 0;

	int TiberiumNodeCount[2];
	TiberiumNodeCount[0] = 0;
	TiberiumNodeCount[1] = 0;

	for (int i = 0;i < DANodeManager->Get_Node_Count();i++) {
		DABaseNodeClass *Node = DANodeManager->Get_Node(i);
		if (!_stricmp(Node->Get_Type(),ControlNodeType)) {
			if (Node->Get_Team() == 0) {
				ControlNodeCount[0]++;
			}
			else if (Node->Get_Team() == 1) {
				ControlNodeCount[1]++;
			}
		}
		else if (!_stricmp(Node->Get_Type(),TiberiumNodeType)) {
			if (Node->Get_Team() == 0) {
				TiberiumNodeCount[0]++;
			}
			else if (Node->Get_Team() == 1) {
				TiberiumNodeCount[1]++;
			}
		}
	}
	cTeam *Nod = Find_Team(0);
	cTeam *GDI = Find_Team(1);
	Nod->Set_Kills(ControlNodeCount[0]);
	Nod->Set_Deaths(TiberiumNodeCount[0]);
	GDI->Set_Kills(ControlNodeCount[1]);
	GDI->Set_Deaths(TiberiumNodeCount[1]);
}

void DADominationManagerClass::Send_Info_Message(int ID) {
	if (DASpawnManager && DASpawnManager->Using_Waiting_Room()) {
		DA::Private_Admin_Message(ID,"When you die you will be asked to use the Purchase Terminal to select a character to spawn as. Once selected you will spawn at either the closest node controlled by your team or your team's starting location.");
	}
	DA::Private_Color_Message(ID,WHITE,"This is a Domination Mode match. The objective is to capture and hold Tiberium Nodes for money and Control Nodes for points. The first team to %d points wins.",(int)WinningPoints);
	DA::Private_Admin_Message(ID,"This is a Domination Mode match. The objective is to capture and hold Tiberium Nodes for money and Control Nodes for points. The first team to %d points wins.",(int)WinningPoints);
	if (DASpawnManager && DASpawnManager->Using_Waiting_Room()) {
		DA::Private_Color_Message(ID,WHITE,"When you die you will be asked to use the Purchase Terminal to select a character to spawn as. Once selected you will spawn at either the closest node controlled by your team or your team's starting location.");
	}
}

Register_Game_Mode(DADominationManagerClass,"Domination","Domination","Control1_Node_X");