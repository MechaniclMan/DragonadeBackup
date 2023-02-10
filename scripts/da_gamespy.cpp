/*	Renegade Scripts.dll
    Dragonade GameSpy Q&R
	Copyright 2012 Whitedragon, Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/

#include "General.h"
#include "engine.h"
#include "engine_DA.h"
#include "da.h"
#include "da_gamespy.h"
#include "da_game.h"
#include "da_settings.h"
#include "da_translation.h"
#include "SysTimeClass.h"
#include "HashTemplateClass.h"
#include "GameDefinition.h"
#include "GameObjManager.h"

void DAGameSpyGameFeatureClass::Init() {
	StringClass IP = Long_To_IP(The_Game()->Get_Ip_Address());
	Port = (unsigned short)DASettingsClass("server.ini").Get_INI()->Get_Int("Server","GameSpyQueryPort",25300);
	ClientAddressSize = sizeof(ClientAddress);
	QueryID = 0;

	hostent *HostInfo;
	HostInfo = gethostbyname("92.242.144.2");
	if (HostInfo) {
		MasterAddress1.sin_family = HostInfo->h_addrtype;
		memcpy((char*)&MasterAddress1.sin_addr.s_addr,HostInfo->h_addr_list[0],HostInfo->h_length);
		MasterAddress1.sin_port = htons(27900);
	}
	HostInfo = gethostbyname("master.gamespy.com");
	if (HostInfo) {
		MasterAddress2.sin_family = HostInfo->h_addrtype;
		memcpy((char*)&MasterAddress2.sin_addr.s_addr,HostInfo->h_addr_list[0],HostInfo->h_length);
		MasterAddress2.sin_port = htons(27900);
	}
	ListenSocket = socket(AF_INET, SOCK_DGRAM,IPPROTO_IP);
	int vals = 0;
	setsockopt(ListenSocket,IPPROTO_IP,IP_DONTFRAGMENT,(const char*)&vals,sizeof(vals));
	u_long on = 1;
	ioctlsocket(ListenSocket, FIONBIO, &on);
	HostInfo = gethostbyname(IP);
	if (HostInfo) {
		ServerAddress.sin_family = AF_INET;
		memcpy((char*)&ServerAddress.sin_addr.s_addr,HostInfo->h_addr_list[0],HostInfo->h_length);
		ServerAddress.sin_port = htons(Port);
		bind(ListenSocket,(sockaddr *)&ServerAddress,sizeof(ServerAddress));
		listen(ListenSocket,5);
		Register_Event(DAEvent::THINK);
		Register_Event(DAEvent::SETTINGSLOADED);
		Register_Event(DAEvent::LEVELLOADED);
		Register_Event(DAEvent::PLAYERJOIN);
		Register_Event(DAEvent::PLAYERLEAVE);
		Timer_Expired(1,0);
		Start_Timer(1,60.0f,true);
	}
}

void DAGameSpyGameFeatureClass::Settings_Loaded_Event() {
	ShowGameFeatures = DASettingsManager::Get_Bool("GameSpyShowGameFeatures",true);
	ShowTeamInfo = DASettingsManager::Get_Bool("GameSpyShowTeamInfo",true);
	CustomRules.Delete_All();
	INISection *Section = DASettingsManager::Get_Section("GameSpyRules");
	if (Section) {
		for (int i = 0;i < Section->Count();i++) {
			CustomRules.Add(StringFormat("%s\\%s\\",Section->Peek_Entry(i)->Entry,Section->Peek_Entry(i)->Value));
		}
	}
}

void DAGameSpyGameFeatureClass::Level_Loaded_Event() {
	ShowPlayerCount = DASettingsManager::Get_Bool("GameSpyShowPlayerCount",false);
	Title = The_Game()->Get_Game_Title();
	if (ShowPlayerCount) {
		The_Game()->Get_Game_Title().Format(L"%s (%d/%d)",Title,The_Game()->Get_Current_Players(),The_Game()->Get_Max_Players());
	}
}

void DAGameSpyGameFeatureClass::Think() {
	char Buffer[256];
	memset(Buffer,0,256);
	int retrecv = recvfrom(ListenSocket,Buffer,255,0,(sockaddr*)&ClientAddress,&ClientAddressSize);
	if (retrecv > 0) {
		QueryID++;
		if (!strcmp(Buffer,"\\status\\")) {
			HashTemplateClass<StringClass,GameDefinition> Definitions;
			Get_Game_Definitions(Definitions);
			StringClass NextMap = Get_Map(The_Game()->Get_Map_Number()+1);
			if (NextMap.Is_Empty()) {
				NextMap = Get_Map(0);
			}
			NextMap.ToUpper();
			StringClass Send;

			//Basic
			Send.Format(
				"\\gametype\\%s\\mapname\\%s\\nextmap\\%s\\%s\\%s\\Time Limit\\%s\\Starting Credits\\%d\\FDS\\Dragonade %s\\",
				DAGameManager::Get_Game_Mode_Long_Name(),DAGameManager::Get_Map(),Definitions.Get(NextMap)->mapName,The_Game()->Get_Time_Limit_Minutes()?"Time Remaining":"Time Elapsed",The_Game()->Get_Time_Limit_Minutes()?Format_Time((unsigned long)The_Game()->Get_Time_Remaining_Seconds()):Format_Time(The_Game()->Get_Game_Duration_S()),Format_Time(The_Game()->Get_Time_Limit_Minutes()*60),The_Cnc_Game()->StartingCredits,DA_VERSION
			);
			
			//Custom rules
			for (int i = 0;i < CustomRules.Count();i++) {
				Send += CustomRules[i];
			}

			//Game features
			if (ShowGameFeatures) {
				const DynamicVectorClass<DAGameFeatureFactoryClass*> &GameFeatures = DAGameManager::Get_Game_Features();
				for (int i = 0;i < GameFeatures.Count();i++) {
					if (GameFeatures[i]->Get_Instance() && GameFeatures[i]->Get_Instance() != this && GameFeatures[i]->Get_Name()) {
						Send += GameFeatures[i]->Get_Name();
						Send += "\\1\\";
					}
				}
			}

			//Basic
			Send += StringFormat("hostname\\%ls\\gamename\\ccrenegade\\gamever\\838\\hostport\\%d\\password\\%d\\numplayers\\%d\\maxplayers\\%d\\queryid\\%u.1",
				Title,The_Game()->Get_Port(),The_Game()->Is_Passworded(),The_Game()->Get_Current_Players(),The_Game()->Get_Max_Players(),QueryID
			);

			//Send what we have so far.
			sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr *)&ClientAddress,ClientAddressSize);

			Send = "";
			int PlayerCount = 0;
			int SendCount = 2;

			if (ShowTeamInfo) {
				int BuildingCount[2] = {0,0};
				for (SLNode<BuildingGameObj> *z = GameObjManager::BuildingGameObjList.Head();z;z = z->Next()) {
					if (!z->Data()->Is_Destroyed()) {
						if (z->Data()->Get_Player_Type() == 0) {
							BuildingCount[0]++;
						}
						else if (z->Data()->Get_Player_Type() == 1) {
							BuildingCount[1]++;
						}
					}
				}

				//Nod team
				cTeam *Team = Find_Team(0);
				Send += StringFormat(
					"\\player_%d\\%ls\\score_%d\\%d\\kills_%d\\%d\\deaths_%d\\%d\\time_%d\\%s\\ping_%d\\%d\\team_%d\\%ls",
					PlayerCount,Get_Wide_Team_Name(0),PlayerCount,(int)Team->Get_Score(),PlayerCount,Team->Get_Kills(),PlayerCount,Team->Get_Deaths(),PlayerCount,Format_Time(The_Game()->Get_Game_Duration_S()),PlayerCount,BuildingCount[0],PlayerCount,Get_Wide_Team_Name(0)
				);
				PlayerCount++;
	
				//GDI team
				Team = Find_Team(1);
				Send += StringFormat(
					"\\player_%d\\%ls\\score_%d\\%d\\kills_%d\\%d\\deaths_%d\\%d\\time_%d\\%s\\ping_%d\\%d\\team_%d\\%ls",
					PlayerCount,Get_Wide_Team_Name(1),PlayerCount,(int)Team->Get_Score(),PlayerCount,Team->Get_Kills(),PlayerCount,Team->Get_Deaths(),PlayerCount,Format_Time(The_Game()->Get_Game_Duration_S()),PlayerCount,BuildingCount[1],PlayerCount,Get_Wide_Team_Name(1)
				);
				PlayerCount++;
				
				/*//Nod buildings
				BaseControllerClass *Base = &The_Cnc_Game()->Nod;
				for (int i = 0;i < Base->Get_Building_List().Count();i++) {
					if (Base->Get_Building_List()[i]->Get_Defense_Object()->Get_Skin() != 1) { //Don't display invincible buildings.
						if (Send.Get_Length() > 1000) {
							Send += StringFormat("\\queryid\\%u.%d",QueryID,SendCount);
							sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr *)&ClientAddress,ClientAddressSize);
							Send = "";
							SendCount++;
						}
						Send += StringFormat(
							"\\player_%d\\%s\\score_%d\\0\\kills_%d\\%d\\deaths_%d\\%d\\time_%d\\0\\ping_%d\\0\\team_%d\\%ls\n",
							PlayerCount,DATranslationManager::Translate(Base->Get_Building_List()[i]),PlayerCount,PlayerCount,(int)Base->Get_Building_List()[i]->Get_Defense_Object()->Get_Health(),PlayerCount,(int)Base->Get_Building_List()[i]->Get_Defense_Object()->Get_Health_Max(),PlayerCount,PlayerCount,PlayerCount,Get_Wide_Team_Name(0)
						);
						PlayerCount++;
					}
				}
				
				//GDI buildings
				Base = &The_Cnc_Game()->GDI;
				for (int i = 0;i < Base->Get_Building_List().Count();i++) {
					if (Base->Get_Building_List()[i]->Get_Defense_Object()->Get_Skin() != 1) {
						if (Send.Get_Length() > 1000) {
							Send += StringFormat("\\queryid\\%u.%d",QueryID,SendCount);
							sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr *)&ClientAddress,ClientAddressSize);
							Send = "";
							SendCount++;
						}
						Send += StringFormat(
							"\\player_%d\\%s\\score_%d\\0\\kills_%d\\%d\\deaths_%d\\%d\\time_%d\\0\\ping_%d\\0\\team_%d\\%ls\n",
							PlayerCount,DATranslationManager::Translate(Base->Get_Building_List()[i]),PlayerCount,PlayerCount,(int)Base->Get_Building_List()[i]->Get_Defense_Object()->Get_Health(),PlayerCount,(int)Base->Get_Building_List()[i]->Get_Defense_Object()->Get_Health_Max(),PlayerCount,PlayerCount,PlayerCount,Get_Wide_Team_Name(1)
						);
						PlayerCount++;
					}
				}*/
			}

			//Players
			for (SLNode<cPlayer>* z = Get_Player_List()->Head();z;z = z->Next()) {
				cPlayer *Player = z->Data();
				if (Send.Get_Length() > 1000) {
					Send += StringFormat("\\queryid\\%u.%d",QueryID,SendCount);
					sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr *)&ClientAddress,ClientAddressSize);
					Send = "";
					SendCount++;
				}
				if (Player->Is_Active()) {
					Send += StringFormat(
						"\\player_%d\\%ls\\score_%d\\%d\\kills_%d\\%d\\deaths_%d\\%d\\time_%d\\%s\\ping_%d\\%d\\team_%d\\%ls",
						PlayerCount,Player->Get_Name(),PlayerCount,(int)Player->Get_Score(),PlayerCount,Player->Get_Kills(),PlayerCount,Player->Get_Deaths(),PlayerCount,Format_Time((TIMEGETTIME()-Player->JoinTime)/1000),PlayerCount,Get_Ping(Player->Get_Id()),PlayerCount,Get_Wide_Team_Name(Player->Get_Player_Type())
					);
					PlayerCount++;
				}
			}
			if (!Send.Is_Empty()) {
				Send += StringFormat("\\queryid\\%u.%d",QueryID,SendCount);
				sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr *)&ClientAddress,ClientAddressSize);
				SendCount++;
			}
			Send.Format("\\final\\\\queryid\\%u.%d",QueryID,SendCount);
			sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr *)&ClientAddress,ClientAddressSize);
		}
		else if (strstr(Buffer,"\\echo\\")) {
			StringClass Send;
			Send.Format("%s\\final\\\\queryid\\%u.1",Buffer,QueryID);
			sendto(ListenSocket,Send,Send.Get_Length()+ 1,0,(sockaddr *)&ClientAddress,ClientAddressSize);
		}
	}
}

void DAGameSpyGameFeatureClass::Timer_Expired(int Number,unsigned int Data) {
	StringClass Send;
	Send.Format("\\heartbeat\\%u\\gamename\\ccrenegade",Port);
	sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr*)&MasterAddress1,sizeof(MasterAddress1));
	sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr*)&MasterAddress2,sizeof(MasterAddress2));
}

void DAGameSpyGameFeatureClass::Player_Join_Event(cPlayer *Player) {
	if (ShowPlayerCount) {
		The_Game()->Get_Game_Title().Format(L"%s (%d/%d)",Title,The_Game()->Get_Current_Players(),The_Game()->Get_Max_Players());
	}
}

void DAGameSpyGameFeatureClass::Player_Leave_Event(cPlayer *Player) {
	if (ShowPlayerCount) {
		The_Game()->Get_Game_Title().Format(L"%s (%d/%d)",Title,The_Game()->Get_Current_Players()-1,The_Game()->Get_Max_Players());
	}
}

DAGameSpyGameFeatureClass::~DAGameSpyGameFeatureClass() { 
	StringClass Send;
	Send.Format("\\heartbeat\\%u\\gamename\\ccrenegade\\statechanged\\2",Port);
	sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr*)&MasterAddress1,sizeof(MasterAddress1));
	sendto(ListenSocket,Send,Send.Get_Length()+1,0,(sockaddr*)&MasterAddress2,sizeof(MasterAddress2));
}

Register_Game_Feature(DAGameSpyGameFeatureClass,"GameSpy Q&R","EnableGameSpy",0);