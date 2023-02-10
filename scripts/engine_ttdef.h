/*	Renegade Scripts.dll
	Copyright 2015 Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/
//Changes made in DA:
//Added TEXT_MESSAGE_KEYHOOK.
#ifndef SCRIPTS_INCLUDE__ENGINE_TTDEF_H
#define SCRIPTS_INCLUDE__ENGINE_TTDEF_H
#include "scripts.h"

class BaseControllerClass;
class PhysicalGameObj;
class AmmoDefinitionClass;

enum TextMessageEnum
{
	TEXT_MESSAGE_PUBLIC,
	TEXT_MESSAGE_TEAM,
	TEXT_MESSAGE_PRIVATE,
	TEXT_MESSAGE_TMSG, //Special extra value for the TMSG console command.
	TEXT_MESSAGE_KEYHOOK, //For chat commands triggered by keyhooks.
};
enum AnnouncementEnum
{
	ANNOUNCE_PUBLIC,
	ANNOUNCE_TEAM,
	ANNOUNCE_PRIVATE,
};
typedef bool (*ChatHook) (int PlayerID,TextMessageEnum Type,const wchar_t *Message,int recieverID);
typedef bool (*HostHook) (int PlayerID,TextMessageEnum Type,const char *Message);
typedef void (*ObjectCreateHook) (void *data,GameObject *obj);
typedef void (*PlayerJoin) (int PlayerID,const char *PlayerName);
typedef void (*PlayerLeave) (int PlayerID);
typedef void (*LoadLevelHook) ();
typedef void (*ConsoleOutputHook) (const char *output);
typedef int (*PurchaseHook) (BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset,const char *data);
typedef void (*PurchaseMonHook) (BaseControllerClass *base,GameObject *purchaser,unsigned int cost,unsigned int preset,unsigned int purchaseret,const char *data);
typedef bool (*RefillHook) (GameObject *purchaser);
typedef bool (*RadioHook) (int PlayerType, int PlayerID, int AnnouncementID, int IconID, AnnouncementEnum AnnouncementType);
typedef bool (*StockDamageHook) (PhysicalGameObj* damager, PhysicalGameObj* target, float damage, uint warheadId);
typedef bool (*TtDamageHook) (PhysicalGameObj* damager, PhysicalGameObj* target, const AmmoDefinitionClass* ammo, const char* bone);

struct ObjectCreateHookStruct {
	ObjectCreateHook hook;
	void *data;
};

typedef void (*KeyHook) (void *data);
struct KeyHookStruct {
	KeyHook hook;
	const char *key;
	int PlayerID;
	void *data;
};

typedef void (*ShaderNotify) (void *data, int notify);
struct ShaderNotifyStruct {
	ShaderNotify hook;
	int ID;
	int PlayerID;
	void *data;
};

//values for Set_Fog_Mode and Set_Fog_Mode_Player
#define FOG_EXP 1 //Set_Fog_Density applies to this mode
#define FOG_EXP2 2 //Set_Fog_Density applies to this mode
#define FOG_LINEAR 3 //This is the default if you are using fog
#endif
