/*	Renegade Scripts.dll
    Dragonade Loot Game Feature
	Copyright 2013 Whitedragon, Tiberian Technologies

	This file is part of the Renegade scripts.dll
	The Renegade scripts.dll is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2, or (at your option) any later
	version. See the file COPYING for more details.
	In addition, an exemption is given to allow Run Time Dynamic Linking of this code with any closed source module that does not contain code covered by this licence.
	Only the source code to the module(s) containing the licenced code has to be released.
*/

#ifndef INCLUDE_DALOOT
#define INCLUDE_DALOOT

#include "da_event.h"
#include "da_gamefeature.h"
#include "da_gameobj.h"
#include "da_player.h"
#include "HashTemplateClass.h"

class DA_API DALootPlayerDataClass : public DAPlayerDataClass {
public:
	void Init() {
		CreationTime = 0;
	}
	void Clear_Level() {
		Clear_Weapons();
	}
	void Add_Weapon(WeaponClass *Weapon);
	void Add_Weapon(const WeaponDefinitionClass *Weapon);
	void Remove_Weapon(WeaponClass *Weapon);
	void Remove_Weapon(const WeaponDefinitionClass *Weapon);
	bool Has_Weapon(WeaponClass *Weapon);
	bool Has_Weapon(const WeaponDefinitionClass *Weapon);
	void Clear_Weapons() {
		Locker.Delete_All();
	}
	inline int Get_Weapon_Count() {
		return Locker.Count();
	}
	DynamicVectorClass<const WeaponDefinitionClass*> Locker;
	unsigned long CreationTime;
};

class DA_API DALootPowerUpClass : public DAGameObjObserverClass {
public:
	DALootPowerUpClass() {
		Dropper = 0;
	}
	DALootPowerUpClass(SoldierGameObj *Soldier) {
		Dropper = Soldier->Get_Player();
	}
	void Init_Damagers(float Range,float ExpireTime);
	void Init_Damagers(float Range,float ExpireTime,SoldierGameObj *Soldier);
	void Add_Damager(cPlayer *Player);
	inline cPlayer *Get_Dropper() {
		return Dropper;
	}
	virtual ~DALootPowerUpClass() { }

protected:
	virtual bool PowerUp_Grant_Request(cPlayer *Player);
	virtual void PowerUp_Grant(cPlayer *Player);
	virtual void Timer_Expired(GameObject *obj,int Number);
	virtual const char *Get_Name() { 
		return "DALootPowerUpClass"; 
	}
	
	DynamicVectorClass<cPlayer*> Damagers;
	cPlayer *Dropper;
};

class DA_API DALootBackpackClass : public DALootPowerUpClass {
public:
	DALootBackpackClass() : DALootPowerUpClass() { }
	DALootBackpackClass(SoldierGameObj *Soldier) : DALootPowerUpClass(Soldier) { }
	void Add_Weapon(const WeaponDefinitionClass *Weapon,int Rounds);
	inline int Get_Weapon_Count() {
		return Weapons.Count();
	}
	virtual ~DALootBackpackClass();

private:
	struct WeaponStruct {
		bool operator==(const WeaponStruct &That) {
			return Weapon == That.Weapon;
		}
		bool operator!=(const WeaponStruct &That) {
			return Weapon != That.Weapon;
		}
		const WeaponDefinitionClass *Weapon;
		int Rounds;
	};
	virtual void Init();
	virtual void PowerUp_Grant(cPlayer *Player);
	virtual void Timer_Expired(GameObject *obj,int Number);
	virtual const char *Get_Name() { 
		return "DALootBackpackClass"; 
	}
	
	DynamicVectorClass<WeaponStruct> Weapons;
	Vector3 LastPos;
	ReferencerClass Icon;
	int IconIndex;
};

class DA_API DALootDNAClass : public DALootPowerUpClass {
public:
	DALootDNAClass(const SoldierGameObjDef *Char) : DALootPowerUpClass() { Character = Char; }
	DALootDNAClass(SoldierGameObj *Soldier) : DALootPowerUpClass(Soldier) { Character = &Soldier->Get_Definition(); }
	virtual ~DALootDNAClass();

private:
	DALootDNAClass();
	virtual void Init();
	virtual bool PowerUp_Grant_Request(cPlayer *Player);
	virtual void PowerUp_Grant(cPlayer *Player);
	virtual void Timer_Expired(GameObject *obj,int Number);
	virtual const char *Get_Name() { 
		return "DALootDNAClass"; 
	}
	const SoldierGameObjDef *Character;
	Vector3 LastPos;
	ReferencerClass Icon;
};

class DA_API DALootGameFeatureClass : public DAEventClass, public DAGameFeatureClass, public DAPlayerDataManagerClass<DALootPlayerDataClass> {
public:
	const PowerUpGameObjDef *Get_Character_PowerUp_Drop(SoldierGameObj *Soldier);
	const PowerUpGameObjDef *Get_Character_PowerUp_Drop(const SoldierGameObjDef *Soldier);
	DALootPowerUpClass *Create_PowerUp(SoldierGameObj *Soldier);
	DALootPowerUpClass *Create_PowerUp(const Vector3 &Position,const SoldierGameObjDef *Soldier);

	const WeaponDefinitionClass *Get_Character_Weapon_Drop(SoldierGameObj *Soldier);
	const WeaponDefinitionClass *Get_Character_Weapon_Drop(const SoldierGameObjDef *Soldier);
	bool Is_Droppable(WeaponClass *Weapon);
	bool Is_Droppable(const WeaponDefinitionClass *Weapon);
	DALootBackpackClass *Create_Backpack(SoldierGameObj *Soldier);
	DALootBackpackClass *Create_Backpack(const Vector3 &Position);

	DALootDNAClass *Create_DNA(SoldierGameObj *Soldier);
	DALootDNAClass *Create_DNA(const Vector3 &Position,const SoldierGameObjDef *Soldier);

private:
	virtual void Init();
	virtual void Settings_Loaded_Event();
	virtual void Object_Created_Event(GameObject *obj);
	virtual void Change_Character_Event(cPlayer *Player,const SoldierGameObjDef *Soldier);
	virtual void Object_Destroyed_Event(GameObject *obj);
	virtual void Add_Weapon_Event(cPlayer *Player,WeaponClass *Weapon);
	virtual void Remove_Weapon_Event(cPlayer *Player,WeaponClass *Weapon);
	virtual void Clear_Weapons_Event(cPlayer *Player);

	bool Drop_Chat_Command(cPlayer *Player,const DATokenClass &Text,TextMessageEnum ChatType);

	struct DropOddsStruct {
		int Total;
		int PowerUp;
		int Weapon;
		int DNA;
	};

	PowerUpGameObjDef *BasePowerUpDef;

	//Main settings
	float ExpireTime;
	float DamagersOnlyTime;
	float DamagersOnlyDistance;
	DropOddsStruct DropOdds;
	HashTemplateClass<unsigned int,DropOddsStruct> CharacterDropOdds; //SoldierGameObjDef*,DropOddsStruct
	bool EnableDropCommand;

	//PowerUp settings
	DynamicVectorClass<const PowerUpGameObjDef*> DefaultPowerUps;
	HashTemplateClass<unsigned int,DynamicVectorClass<const PowerUpGameObjDef*>> CharacterPowerUps; //SoldierGameObjDef*,DynamicVectorClass<const PowerUpGameObjDef*>

	//Weapon settings
	StringClass WeaponModel;
	DynamicVectorClass<const WeaponDefinitionClass*> UndroppableWeapons;
	HashTemplateClass<unsigned int,const WeaponDefinitionClass*> CharacterWeapons; //SoldierGameObjDef*,WeaponDefinitionClass*

	//DNA settings
	StringClass DNAModel;
};

extern DA_API DAGameFeatureRegistrant<DALootGameFeatureClass> DALootGameFeatureClassRegistrant;
#define DALootGameFeature ((DALootGameFeatureClass*)DALootGameFeatureClassRegistrant.Get_Instance())

#endif