/*	Renegade Scripts.dll
    Dragonade Vehicle Only Crates
	Copyright 2013 Whitedragon, Tiberian Technologies

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
#include "engine_da.h"
#include "da.h"
#include "da_crate.h"
#include "da_event.h"
#include "da_gameobj.h"
#include "da_translation.h"
#include "PurchaseSettingsDefClass.h"
#include "GameObjManager.h"

/********************************************************************************************************************************/

class DAAblativeArmorCrateObserverClass : public DAGameObjObserverClass {
	virtual const char *Get_Name() { 
		return "DAAblativeArmorCrateObserverClass"; 
	}

	virtual void Init() {
		Total = 0;
	}

	virtual bool Damage_Received_Request(OffenseObjectClass *Offense,DADamageType::Type Type,const char *Bone) {
		if (Offense->Get_Damage() > 0) {
			Offense->Scale_Damage(0.75f); //25% damage reduction.
		}
		return true;
	} 
	
	virtual void Damage_Received(ArmedGameObj *Damager,float Damage,unsigned int Warhead,DADamageType::Type Type,const char *Bone) {
		if (Damage > 0) {
			Total += Damage/3;
			DefenseObjectClass *Defense = ((VehicleGameObj*)Get_Owner())->Get_Defense_Object();
			if (Total >= (Defense->Get_Health_Max()+Defense->Get_Shield_Strength_Max())) { //Wear off after absorbing damage equal to the vehicle's health.
				Set_Delete_Pending();
			}
		}
	}
	
	float Total;
};

class DAAblativeArmorCrateClass : public DACrateClass {
	virtual bool Can_Activate(cPlayer *Player) { //Prevent crate from triggering if already in effect.
		return !Player->Get_GameObj()->Get_Vehicle()->Has_Observer("DAAblativeArmorCrateObserverClass");
	}

	virtual void Activate(cPlayer *Player) {
		Player->Get_GameObj()->Get_Vehicle()->Add_Observer(new DAAblativeArmorCrateObserverClass);
		DA::Page_Player(Player,"Your vehicle has been upgraded by the Ablative Armor Crate. It will take reduced damage while the upgraded armor holds.");
	}
};

Register_Crate(DAAblativeArmorCrateClass,"Ablative Armor",DACrateType::VEHICLE);

/********************************************************************************************************************************/

class DATankDestroyerCrateSoldierObserverClass : public DAGameObjObserverClass {
public:
	virtual const char *Get_Name() { 
		return "DATankDestroyerCrateSoldierObserverClass"; 
	}

	DATankDestroyerCrateSoldierObserverClass(int Warhead) {
		this->Warhead = Warhead;
	}

	virtual bool Damage_Dealt_Request(DamageableGameObj *Victim,OffenseObjectClass *Offense,DADamageType::Type Type,const char *Bone) {
		if (!Warhead) { //Cannon/Missile
			if (Type == DADamageType::SPLASH) {
				return false; //No splash damage.
			}
			else if ((Type == DADamageType::NORMAL || Type == DADamageType::SPLASHDIRECT) && Victim->As_VehicleGameObj()) {
				Offense->Scale_Damage(1.25f); //25% increase to vehicles.
			}
		}
		else { //Machine gun
			if (Type == DADamageType::NORMAL && (int)Offense->Get_Warhead() == Warhead) {
				if (Victim->As_VehicleGameObj()) {
					Offense->Scale_Damage(1.25f); //25% increase to vehicles.
				}
				else if (Victim->As_SoldierGameObj()) {
					Offense->Scale_Damage(0.5f); //50% decrease to infantry.
				}
			}
		}
		return true;
	}
	
	virtual void Vehicle_Exit(VehicleGameObj *Vehicle,int Seat) {
		Set_Delete_Pending(); //Destroy when exiting vehicle.
	}
	
	int Warhead;
};

class DATankDestroyerCrateVehicleObserverClass : public DAGameObjObserverClass {
	virtual const char *Get_Name() { 
		return "DATankDestroyerCrateVehicleObserverClass"; 
	}
	
	virtual void Init() {
		VehicleGameObj *Vehicle = ((VehicleGameObj*)Get_Owner());
		WeaponClass *Weapon = Vehicle->Get_Weapon_Bag()->Get_Weapon();
		if (Weapon->PrimaryAmmoDefinition && !Weapon->PrimaryAmmoDefinition->ExplosionDefID) {
			Warhead = Weapon->PrimaryAmmoDefinition->Warhead;
		}
		else if (Weapon->SecondaryAmmoDefinition && !Weapon->SecondaryAmmoDefinition->ExplosionDefID) {
			Warhead = Weapon->SecondaryAmmoDefinition->Warhead;
		}
		else {
			Warhead = 0;
		}
		int Rounds = 0;
		if (Warhead) { //200 rounds if weapon is a machine gun.
			Rounds = 500;
			DA::Page_Player(Vehicle->Get_Occupant(0)->Get_Player(),"The Tank Destroyer Crate has equipped your vehicle with %d rounds of armor piercing ammunition, increasing damage to vehicles at the cost of infantry damage.",Rounds);
		}
		else { //30 if its a cannon or missile.
			Rounds = 30;
			DA::Page_Player(Vehicle->Get_Occupant(0)->Get_Player(),"The Tank Destroyer Crate has equipped your vehicle with %d rounds of armor piercing ammunition, increasing damage to vehicles at the cost of splash damage.",Rounds);
		}
		if (Weapon->Get_Definition()->ClipSize == -1) {
			Set_Current_Bullets(Vehicle,Rounds);
			Set_Current_Clip_Bullets(Vehicle,0);
		}
		else {
			Set_Current_Clip_Bullets(Vehicle,Rounds-Vehicle->Get_Weapon_Bag()->Get_Weapon()->Get_Clip_Rounds());
		}
		Vehicle->Get_Occupant(0)->Add_Observer(new DATankDestroyerCrateSoldierObserverClass(Warhead));
		Start_Timer(1,1.0f);
	}

	virtual void Vehicle_Enter(cPlayer *Player,int Seat) {
		if (Seat == 0) { //Attach observer to driver.
			Player->Get_GameObj()->Add_Observer(new DATankDestroyerCrateSoldierObserverClass(Warhead));
		}
	}

	virtual void Timer_Expired(GameObject *obj,int Number) {
		VehicleGameObj *Vehicle = ((VehicleGameObj*)Get_Owner());
		if (Vehicle->Get_Weapon_Bag()->Get_Weapon() && !Vehicle->Get_Weapon_Bag()->Get_Weapon()->Get_Inventory_Rounds() && !Vehicle->Get_Weapon_Bag()->Get_Weapon()->Get_Clip_Rounds()) { //Ammo used up.
			if (Vehicle->Get_Weapon_Bag()->Get_Weapon()->Get_Definition()->ClipSize == -1) {
				Set_Current_Bullets(Vehicle,-1);
				Set_Current_Clip_Bullets(Vehicle,-1); //Reset ammo counter.
			}
			else {
				Set_Current_Clip_Bullets(Vehicle,-1); //Reset ammo counter.
			}
			if (Vehicle->Get_Occupant(0)) { //Remove observer from driver.
				Vehicle->Get_Occupant(0)->Remove_Observer("DATankDestroyerCrateSoldierObserverClass");
			}
			Set_Delete_Pending();
		}
		else {
			Start_Timer(1,1.0f);
		}
	}

	int Warhead;
};

class DATankDestroyerCrateClass : public DACrateClass {
	virtual bool Can_Activate(cPlayer *Player) { //Don't activate if the vehicle has no weapon or already in effect.
		return (Player->Get_GameObj()->Get_Vehicle()->Get_Weapon_Bag()->Get_Weapon() && !Player->Get_GameObj()->Get_Vehicle()->Has_Observer("DATankDestroyerCrateVehicleObserverClass"));
	}

	virtual void Activate(cPlayer *Player) {
		Player->Get_GameObj()->Get_Vehicle()->Add_Observer(new DATankDestroyerCrateVehicleObserverClass);
	}
};

Register_Crate(DATankDestroyerCrateClass,"Tank Destroyer",DACrateType::VEHICLE);

/********************************************************************************************************************************/

class DAOverhaulCrateClass : public DACrateClass {
	virtual void Settings_Loaded() {
		DACrateClass::Settings_Loaded();

		//Vehicles
		Vehicles.Delete_All();
		for (int Team = 0;Team < 2;Team++) { //Add all purchasable vehicles into vector.
			for (int Type = 0;Type < 7;Type++) {
				PurchaseSettingsDefClass *PT = PurchaseSettingsDefClass::Find_Definition((PurchaseSettingsDefClass::TYPE)Type,(PurchaseSettingsDefClass::TEAM)Team);
				if (PT) {
					for (int i = 0;i < 10;i++) {
						VehicleGameObjDef *Def = (VehicleGameObjDef*)Find_Definition(PT->Get_Definition(i));
						if (Def && Def->Get_Class_ID() == CID_Vehicle && (Def->Get_Type() != VEHICLE_TYPE_FLYING || Is_Map_Flying())) {
							Vehicles.Add(Def);
						}
					}
				}
			}
		}
		VehicleGameObjDef *Def = (VehicleGameObjDef*)Find_Named_Definition("CnC_Nod_Recon_Bike"); //Special case for Recon Bike.
		if (Def) {
			Def->WeaponDefID = Get_Definition_ID("Weapon_StealthTank_Player");
			DefenseObjectDefClass &Defense = const_cast<DefenseObjectDefClass&>(Def->Get_DefenseObjectDef());
			Defense.Skin = ArmorWarheadManager::Get_Armor_Type("CNCVehicleHeavy");
			Defense.ShieldType = ArmorWarheadManager::Get_Armor_Type("CNCVehicleHeavy");
			Defense.ShieldStrengthMax = 150;
			Defense.ShieldStrength = 150;
			Vehicles.Add(Def);
		}
		Def = (VehicleGameObjDef*)Find_Named_Definition("Nod_SSM_Launcher_Player"); //Special case for SSM.
		if (Def) {
			Def->WeaponDefID = 0;
			DefenseObjectDefClass &Defense = const_cast<DefenseObjectDefClass&>(Def->Get_DefenseObjectDef());
			Defense.Skin = ArmorWarheadManager::Get_Armor_Type("CNCVehicleHeavy");
			Defense.ShieldType = ArmorWarheadManager::Get_Armor_Type("CNCVehicleHeavy");
			Defense.ShieldStrengthMax = 125;
			Defense.ShieldStrength = 125;
			Vehicles.Add(Def);
		}
	}

	virtual bool Can_Activate(cPlayer *Player) { //No vehicles on this map.
		return Vehicles.Count() > 1;
	}
	
	virtual void Activate(cPlayer *Player) {
		VehicleGameObj *Vehicle = Player->Get_GameObj()->Get_Vehicle();
		Vehicle->Set_Delete_Pending(); //Destroy old vehicle.
		Reselect:
		int Rand = Get_Random_Int(0,Vehicles.Count()); //Get random vehicle.
		const VehicleGameObjDef *Def = Vehicles[Rand];
		if (Def == &Vehicle->Get_Definition()) { //Don't pick the vehicle they already have.
			goto Reselect;
		}
		Vehicle = (VehicleGameObj*)Create_Object(Def,Vehicle->Get_Transform()); //Create new vehicle.
		Vehicle->Set_Player_Type(-2);
		Vehicle->Lock_Vehicle(Player->Get_GameObj(),5.0f);
		Fix_Stuck_Objects(Vehicle->Get_Position(),10.0f,15.0f);
		DA::Page_Player(Player,"Your vehicle has been transformed into %s by the Overhaul Crate.",a_or_an_Prepend(DATranslationManager::Translate(Def)));
	}
	DynamicVectorClass<const VehicleGameObjDef*> Vehicles;
};

Register_Crate(DAOverhaulCrateClass,"Overhaul",DACrateType::VEHICLE);

/********************************************************************************************************************************/

class DAEMPCrateObserverClass : public DAGameObjObserverClass {
	virtual const char *Get_Name() { 
		return "DAEMPCrateObserverClass"; 
	}

	virtual void Init() {
		VehicleGameObj *Vehicle = ((VehicleGameObj*)Get_Owner());
		Commands->Enable_Engine(Vehicle,false); //Turn off engine.
		//Vehicle->Get_Weapon_Bag()->Deselect(); //Remove weapon.
		if (Vehicle->Get_Definition().Get_Type() != VEHICLE_TYPE_FLYING) {
			Vehicle->Set_Immovable(true); //Freeze vehicle. Turning off the engine will prevent flying vehicles from moving, plus we want them to fall to the ground.
		}
		if (Vehicle->Is_Stealth_Enabled()) {
			Stealth = true;
			Commands->Enable_Stealth(Vehicle,false);
		}
		else {
			Stealth = false;
		}
		Start_Timer(1,Get_Random_Float(15.0f,30.0f)); //End
		Start_Timer(2,2.0f); //Destroy effect.
		Effect = Create_Object("Generic_Cinematic",Vehicle->Get_Transform());
		Commands->Set_Model(Effect,"ag_braxis_small");
	}

	virtual void Vehicle_Enter(cPlayer *Player,int Seat) {
		Commands->Enable_Engine(Get_Owner(),false);
		//((VehicleGameObj*)Get_Owner())->Get_Weapon_Bag()->Deselect();
	}
	
	virtual void Timer_Expired(GameObject *obj,int Number) {
		if (Number == 1) {
			VehicleGameObj *Vehicle = ((VehicleGameObj*)Get_Owner());
			if (!Vehicle->Are_Transitions_Enabled() || !Vehicle->Get_Definition().Get_Seat_Count() || Vehicle->Get_Occupant(0)) {
				Commands->Enable_Engine(Vehicle,true);
			}
			//Vehicle->Get_Weapon_Bag()->Select_Next();
			Vehicle->Set_Immovable(false);
			if (Stealth) {
				Commands->Enable_Stealth(Vehicle,true);
			}
			Set_Delete_Pending();
		}
		else if (Number == 2) {
			if (Effect) {
				Effect->Set_Delete_Pending();
			}
		}
	}
	
	ReferencerClass Effect;
	bool Stealth;
};

class DAEMPCrateClass : public DACrateClass {
	virtual void Activate(cPlayer *Player) {
		Vector3 Position;
		Player->Get_GameObj()->Get_Position(&Position);
		for (SLNode<VehicleGameObj> *x = GameObjManager::VehicleGameObjList.Head();x;x = x->Next()) { //Mark all vehicles.
			if (x->Data()->Peek_Physical_Object()->As_MoveablePhysClass() && Commands->Get_Distance(Position,x->Data()->Get_Position()) < 20.0f) {
				x->Data()->Add_Observer(new DAEMPCrateObserverClass);
			}
		}
		DA::Host_Message("Electro magnetic pulse detected! Any nearby vehicles have been temporarily disabled.");
	}
};

Register_Crate(DAEMPCrateClass,"EMP",DACrateType::VEHICLE);

/********************************************************************************************************************************/

class DARenJackCrateObserverClass : public DAGameObjObserverClass {
	virtual const char *Get_Name() { 
		return "DARenJackCrateObserverClass"; 
	}

	virtual void Init() {
		Team = ((VehicleGameObj*)Get_Owner())->Get_Player_Type();
	}

	virtual bool Vehicle_Entry_Request(cPlayer *Player,int &Seat) {
		if (Player->Get_Team() != Team) {
			DA::Page_Player(Player,"This vehicle is protected by a RenJack(TM) Unit. You may not enter it.");
			return false;
		}
		return true;
	}

	int Team;
};

class DARenJackCrateClass : public DACrateClass {
	virtual bool Can_Activate(cPlayer *Player) { //Prevent crate from triggering if already in effect.
		return !Player->Get_GameObj()->Get_Vehicle()->Has_Observer("DARenJackCrateObserverClass");
	}
	
	virtual void Activate(cPlayer *Player) {
		Player->Get_GameObj()->Get_Vehicle()->Add_Observer(new DARenJackCrateObserverClass);
		DA::Page_Player(Player,"Your vehicle has been equipped with a RenJack(TM) Unit. Enemy players will be unable to steal it.");
	}
};

Register_Crate(DARenJackCrateClass,"RenJack",DACrateType::VEHICLE);

/********************************************************************************************************************************/