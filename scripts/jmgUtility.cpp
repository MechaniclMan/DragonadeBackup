
#include "general.h"

#include "JmgUtility.h"
#include "GameObjManager.h"
bool JmgUtility::hasStatedDeathMessage[128] = {false};

void JMG_Utility_Check_If_Script_Is_In_Library::Created(GameObject *obj)
{
	Commands->Start_Timer(obj,this,0.1f,1);
}
void JMG_Utility_Check_If_Script_Is_In_Library::Timer_Expired(GameObject *obj,int number)
{
	if (1 == number)
	{
		if (!ScriptRegistrar::GetScriptFactory(Get_Parameter("ScriptName")))
		{
			char errMsg[220],pErrMsg[220];
			sprintf(errMsg,"ERROR: The Script %s was not found in scripts.dll. Please check your workspace and make sure you have included %s.cpp and its includes!",Get_Parameter("ScriptName"),Get_Parameter("CppName"));
			Console_Input(errMsg);
			sprintf(pErrMsg,"msg %s",errMsg);
			Console_Input(pErrMsg);
		}
	}
}

void JMG_Send_Custom_When_Custom_Sequence_Matched::Created(GameObject *obj)
{
	depth = 0;
	failCount = 0;
	enabled = Get_Int_Parameter("Starts_Enabled") == 0 ? false : true;
	if (!Get_Int_Parameter("Custom_0"))
	{
		Console_Input("msg JMG_Send_Custom_When_Custom_Sequence_Matched ERROR: Custom_0 must not be 0!");
		enabled = false;
	}
}

void JMG_Send_Custom_When_Custom_Sequence_Matched::Custom(GameObject *obj,int message,int param,GameObject *sender)
{
	if (message == Get_Int_Parameter("Custom_0") || message == Get_Int_Parameter("Custom_1") || message == Get_Int_Parameter("Custom_2") || message == Get_Int_Parameter("Custom_3") || message == Get_Int_Parameter("Custom_4") || message == Get_Int_Parameter("Custom_5") || message == Get_Int_Parameter("Custom_6") || message == Get_Int_Parameter("Custom_7") || message == Get_Int_Parameter("Custom_8") || message == Get_Int_Parameter("Custom_9"))
	{
		if (!enabled)
			return;
		char custom[32];
		sprintf(custom,"Custom_%0d",depth);
		bool correctStep = false;
		if (message == Get_Int_Parameter(custom))
		{
			correctStep = true;
			depth++;
		}
		else
		{
			if (!Get_Int_Parameter("Failure_Safty") || depth)
			{
				failCount++;
				if (failCount < Get_Int_Parameter("Max_Failures"))
				{
					Send_Custom(sender,Get_Int_Parameter("Partial_Failure_Custom"),depth);
					return;
				}
				Send_Custom(sender,Get_Int_Parameter("Failure_Custom"),depth);
				if (Get_Int_Parameter("Disable_On_Failure"))
					enabled = false;
			}
			depth = 0;
			failCount = 0;
		}
		sprintf(custom,"Custom_%0d",depth);
		if (depth >= 10 || !Get_Int_Parameter(custom))
		{
			if (Get_Int_Parameter("Disable_On_Success"))
				enabled = false;
			depth = 0;
			failCount = 0;
			Send_Custom(sender,Get_Int_Parameter("Success_Custom"),param);
		}
		else if (correctStep && (!Get_Int_Parameter("Correct_Step_Safty") || depth))
			Send_Custom(sender,Get_Int_Parameter("Correct_Step_Custom"),depth);
	}
	if (message == Get_Int_Parameter("Enable_Custom"))
	{
		if (enabled)
			return;
		depth = 0;
		failCount = 0;
		enabled = true;
		if (!Get_Int_Parameter("Custom_0"))
		{
			Console_Input("msg JMG_Send_Custom_When_Custom_Sequence_Matched ERROR: Custom_0 must not be 0!");
			enabled = false;
		}
	}
}
void JMG_Send_Custom_When_Custom_Sequence_Matched::Send_Custom(GameObject *obj,int custom,int param)
{
	GameObject *sendId = Commands->Find_Object(Get_Int_Parameter("Send_To_ID"));
	if (sendId)
		Commands->Send_Custom_Event(obj,sendId,custom,param,0);
}
void JMG_Utility_Change_Model_On_Timer::Created(GameObject *obj)
{
	Commands->Start_Timer(obj,this,Get_Float_Parameter("Time"),1);
}
void JMG_Utility_Change_Model_On_Timer::Timer_Expired(GameObject *obj,int number)
{
	if (number == 1)
	{
		Commands->Set_Model(obj,Get_Parameter("Model"));
	}
}
void JMG_Utility_Emulate_DamageableStaticPhys::Created(GameObject *obj)
{
	team = Commands->Get_Player_Type(obj);
	alive = true;
	playingTwitch = false;
	playingTransition = false;
	Set_Death_Points(obj,0);
	Set_Damage_Points(obj,Get_Float_Parameter("Damage_Points"));
	Play_Animation(obj,true,Get_Float_Parameter("Live_Loop_Start"),Get_Float_Parameter("Live_Loop_End"));
}
void JMG_Utility_Emulate_DamageableStaticPhys::Damaged(GameObject *obj,GameObject *damager,float damage)
{
	if (!damage)
		return;
	if (alive && !Commands->Get_Shield_Strength(obj))
	{
		alive = false;
		if (Get_Int_Parameter("Neutral_When_Dead"))
			Commands->Set_Player_Type(obj,-2);
		Set_Damage_Points(obj,Get_Float_Parameter("Damage_Points"));
		Play_Animation(obj,false,Get_Float_Parameter("Death_Transition_Start"),Get_Float_Parameter("Death_Transition_End"));
		playingTransition = true;
		GameObject *sendId = Commands->Find_Object(Get_Int_Parameter("Send_To_ID"));
		if (sendId)
			Commands->Send_Custom_Event(obj,sendId,Get_Int_Parameter("Death_Message"),Commands->Get_ID(damager),0);
		if (_stricmp(Get_Parameter("Killed_Explosion"),"null"))
			Commands->Create_Explosion(Get_Parameter("Killed_Explosion"),Commands->Get_Position(obj),damager);
		if (damager)
			Commands->Give_Points(damager,(Commands->Get_Player_Type(obj) == Commands->Get_Player_Type(damager) ? -1 : 1) * Get_Float_Parameter("Death_Points"),0);
		if (Get_Int_Parameter("Disable_Collision_When_Dead"))
			Commands->Disable_Physical_Collisions(obj);
	}
	if (!alive && Commands->Get_Shield_Strength(obj) >= Commands->Get_Max_Shield_Strength(obj))
	{
		alive = true;
		Commands->Set_Player_Type(obj,team);
		Set_Damage_Points(obj,Get_Float_Parameter("Heal_Points"));
		Play_Animation(obj,false,Get_Float_Parameter("Live_Transition_Start"),Get_Float_Parameter("Live_Transition_End"));
		playingTransition = true;
		GameObject *sendId = Commands->Find_Object(Get_Int_Parameter("Send_To_ID"));
		if (sendId)
			Commands->Send_Custom_Event(obj,sendId,Get_Int_Parameter("Repaired_Message"),Commands->Get_ID(damager),0);
		if (_stricmp(Get_Parameter("Repaired_Explosion"),"null"))
			Commands->Create_Explosion(Get_Parameter("Repaired_Explosion"),Commands->Get_Position(obj),damager);
		if (damager)
			Commands->Give_Points(damager,(Commands->Get_Player_Type(obj) == Commands->Get_Player_Type(damager) ? 1 : -1) * Get_Float_Parameter("Repair_Points"),0);
		if (Get_Int_Parameter("Disable_Collision_When_Dead"))
			Commands->Enable_Collisions(obj);
	}
	if (!playingTransition && !playingTwitch)
	{
		if (alive)
			Play_Animation(obj,false,Get_Float_Parameter("Live_Twitch_Start"),Get_Float_Parameter("Live_Twitch_End"));
		else
			Play_Animation(obj,false,Get_Float_Parameter("Death_Twitch_Start"),Get_Float_Parameter("Death_Twitch_End"));
		playingTwitch = Get_Int_Parameter("Play_Twitches_To_Completion") ? true : false;
	}
}
void JMG_Utility_Emulate_DamageableStaticPhys::Animation_Complete(GameObject *obj,const char *anim)
{
	if (alive)
		Play_Animation(obj,true,Get_Float_Parameter("Live_Loop_Start"),Get_Float_Parameter("Live_Loop_End"));
	else
		Play_Animation(obj,true,Get_Float_Parameter("Death_Loop_Start"),Get_Float_Parameter("Death_Loop_End"));
	if (playingTransition)
		playingTransition = false;
	if (playingTwitch)
		playingTwitch = false;
}
void JMG_Utility_Emulate_DamageableStaticPhys::Play_Animation(GameObject *obj,bool loop,float start,float end)
{
	if (start == -1 || end == -1)
		return;
	Commands->Set_Animation(obj,Get_Parameter("Animation"),loop,0,start,end,false);
}
void JMG_Utility_Display_HUD_Info_Text_To_All_Players_On_Custom::Custom(GameObject *obj,int message,int param,GameObject *sender)
{
	if (message == Get_Int_Parameter("Custom"))
		JmgUtility::SetHUDHelpText(Get_Int_Parameter("StringId"),Get_Vector3_Parameter("ColorRGB"));
}
void JMG_Utility_Display_HUD_Info_Text_To_Sender_On_Custom::Custom(GameObject *obj,int message,int param,GameObject *sender)
{
	if (message == Get_Int_Parameter("Custom"))
	{
		GameObject *player = sender;
		GameObject *tempPlayer = Get_GameObj(param);
		if (tempPlayer)
			player = tempPlayer;
		Set_HUD_Help_Text_Player(player,Get_Int_Parameter("StringId"),Get_Vector3_Parameter("ColorRGB"));
	}
}
void JMG_Utility_Soldier_Transition_On_Custom::Custom(GameObject *obj,int message,int param,GameObject *sender)
{
	if (message == Get_Int_Parameter("Custom"))
	{
		if (Get_Vehicle(obj))
			Soldier_Transition_Vehicle(obj);
		else
		{
			Vector3 pos = Commands->Get_Position(obj);
			float dist = 0;
			GameObject *nearest = NULL;
			for (SLNode<SmartGameObj> *current = GameObjManager::SmartGameObjList.Head();current;current = current->Next())
			{
				SmartGameObj* o = current->Data();
				if (o->As_VehicleGameObj() && Get_Vehicle_Occupant_Count(o) < Get_Vehicle_Seat_Count(o))
				{
					float tempDistance = JmgUtility::SimpleDistance(pos,Commands->Get_Position(o));
					if (!nearest || tempDistance < dist)
					{
						nearest = o;
						dist = tempDistance;
					}
				}
			}
			if (nearest)
				Force_Vehicle_Entry(obj,nearest);
		}
	}
}
void JMG_Utility_Poke_Send_Self_Custom::Created(GameObject *obj)
{
	poked = false;
	Commands->Enable_HUD_Pokable_Indicator(obj,true);
}
void JMG_Utility_Poke_Send_Self_Custom::Poked(GameObject *obj, GameObject *poker)
{
	if (!poked && Commands->Is_A_Star(poker))
	{
		poked= true;
		Commands->Send_Custom_Event(obj,obj,Get_Int_Parameter("Custom"),Get_Int_Parameter("Param"),Get_Float_Parameter("Delay"));
		Commands->Enable_HUD_Pokable_Indicator(obj,false);
		if (Get_Int_Parameter("LockoutTime") >= 0.0f)
			Commands->Start_Timer(obj,this,Get_Float_Parameter("LockoutTime"),1);
	}
}
void JMG_Utility_Poke_Send_Self_Custom::Timer_Expired(GameObject *obj,int number)
{
	if (number == 1)
	{
		poked = false;
		Commands->Enable_HUD_Pokable_Indicator(obj,true);
	}
}
ScriptRegistrant<JMG_Utility_Check_If_Script_Is_In_Library> JMG_Utility_Check_If_Script_Is_In_Library_Registrant("JMG_Utility_Check_If_Script_Is_In_Library","ScriptName:string,CppName:string");
ScriptRegistrant<JMG_Send_Custom_When_Custom_Sequence_Matched> JMG_Send_Custom_When_Custom_Sequence_Matched_Registrant("JMG_Send_Custom_When_Custom_Sequence_Matched","Success_Custom=0:int,Correct_Step_Custom=0:int,Partial_Failure_Custom=0:int,Failure_Custom=0:int,Send_To_ID=0:int,Custom_0=0:int,Custom_1=0:int,Custom_2=0:int,Custom_3=0:int,Custom_4=0:int,Custom_5=0:int,Custom_6=0:int,Custom_7=0:int,Custom_8=0:int,Custom_9=0:int,Disable_On_Success=1:int,Disable_On_Failure=0:int,Starts_Enabled=1:int,Enable_Custom=0:int,Correct_Step_Safty=0:int,Failure_Safty=1:int,Max_Failures=1:int");
ScriptRegistrant<JMG_Utility_Change_Model_On_Timer> JMG_Utility_Change_Model_On_Timer_Registrant("JMG_Utility_Change_Model_On_Timer","Model=null:string,Time=0:float");
ScriptRegistrant<JMG_Utility_Emulate_DamageableStaticPhys> JMG_Utility_Emulate_DamageableStaticPhys_Registrant("JMG_Utility_Emulate_DamageableStaticPhys","Animation=null:string,Killed_Explosion=null:string,Live_Loop_Start=-1.0:float,Live_Loop_End=-1.0:float,Live_Twitch_Start=-1.0:float,Live_Twitch_End=-1.0:float,Death_Transition_Start=-1.0:float,Death_Transition_End=-1.0:float,Death_Loop_Start=-1.0:float,Death_Loop_End=-1.0:float,Death_Twitch_Start=-1.0:float,Death_Twitch_End=-1.0:float,Live_Transition_Start=-1.0:float,Live_Transition_End=-1.0:float,Repaired_Explosion=null:string,Play_Twitches_To_Completion:int,Send_To_ID:int,Death_Message:int,Repaired_Message:int,Damage_Points:float,Death_Points:float,Heal_Points:float,Repair_Points:float,Neutral_When_Dead:int,Disable_Collision_When_Dead:int");
ScriptRegistrant<JMG_Utility_Display_HUD_Info_Text_To_All_Players_On_Custom> JMG_Utility_Display_HUD_Info_Text_To_All_Players_On_Custom_Registrant("JMG_Utility_Display_HUD_Info_Text_To_All_Players_On_Custom","Custom:int,StringId:int,ColorRGB:Vector3");
ScriptRegistrant<JMG_Utility_Display_HUD_Info_Text_To_Sender_On_Custom> JMG_Utility_Display_HUD_Info_Text_To_Sender_On_Custom_Registrant("JMG_Utility_Display_HUD_Info_Text_To_Sender_On_Custom","Custom:int,StringId:int,ColorRGB:Vector3");
ScriptRegistrant<JMG_Utility_Soldier_Transition_On_Custom> JMG_Utility_Soldier_Transition_On_Custom_Registrant("JMG_Utility_Soldier_Transition_On_Custom","Custom:int");
ScriptRegistrant<JMG_Utility_Poke_Send_Self_Custom> JMG_Utility_Poke_Send_Self_Custom_Registrant("JMG_Utility_Poke_Send_Self_Custom","Custom:int,Param:int,Delay:float,LockoutTime=-1.0:float");
