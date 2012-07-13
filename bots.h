//-----------------------------------------------------------------------------
//
// Skulltag Source
// Copyright (C) 2002 Brad Carney
// Copyright (C) 2007-2012 Skulltag Development Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the Skulltag Development Team nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 4. Redistributions in any form must be accompanied by information on how to
//    obtain complete source code for the software and any accompanying
//    software that uses the software. The source code must either be included
//    in the distribution or be available for no more than the cost of
//    distribution plus a nominal fee, and must be freely redistributable
//    under reasonable conditions. For an executable file, complete source
//    code means the source code for all modules it contains. It does not
//    include source code for modules or files that typically accompany the
//    major components of the operating system on which the executable file
//    runs.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//
//
// Filename: bots.h
//
// Description: Contains bot structures and prototypes
// [Dusk] Cropped out the stuff botc doesn't need.
//
//-----------------------------------------------------------------------------

#ifndef __BOTS_H__
#define __BOTS_H__

//*****************************************************************************
//  DEFINES

// Maximum number of variables on the bot evaluation stack.
#define	BOTSCRIPT_STACK_SIZE		8

// Maximum number of botinto structures that can be defines.
#define	MAX_BOTINFO					128

// Maximum number of states that can appear in a script.
#define	MAX_NUM_STATES				256

// Maximum number of bot events that can be defined.
#define	MAX_NUM_EVENTS				32

// Maximum number of global bot events that can be defined.
#define	MAX_NUM_GLOBAL_EVENTS		32

// Maximum number of global variables that can be defined in a script.
#define	MAX_SCRIPT_VARIABLES		128

// Maximum number of global arrays that can be defined in a script.
#define	MAX_SCRIPT_ARRAYS			16

// Maximum number of global arrays that can be defined in a script.
#define	MAX_SCRIPTARRAY_SIZE		65536

// Maxmimum number of state (local) variables that can appear in a script.
#define	MAX_STATE_VARIABLES			16

// Maximum number of strings that can appear in a script stringlist.
#define	MAX_LIST_STRINGS			128

// Maximum length of those strings in the stringlist.
#define	MAX_STRING_LENGTH			256

// Maximum reaction time for a bot.
#define	MAX_REACTION_TIME			52

// Maximum number of events the bots can store up that it's waiting to react to.
#define	MAX_STORED_EVENTS			64

//*****************************************************************************
typedef enum
{
	// Bot skill ratings.
	BOTSKILL_VERYPOOR,
	BOTSKILL_POOR,
	BOTSKILL_LOW,
	BOTSKILL_MEDIUM,
	BOTSKILL_HIGH,
	BOTSKILL_EXCELLENT,
	BOTSKILL_SUPREME,
	BOTSKILL_GODLIKE,
	BOTSKILL_PERFECT,

	NUM_BOT_SKILLS

} BOTSKILL_e;

//*****************************************************************************
//  STRUCTURES
//

//*****************************************************************************
//	These are the botscript data headers that it writes out.
typedef enum
{
	DH_COMMAND,
	DH_STATEIDX,
	DH_STATENAME,
	DH_ONENTER,
	DH_MAINLOOP,
	DH_ONEXIT,
	DH_EVENT,
	DH_ENDONENTER,
	DH_ENDMAINLOOP,
	DH_ENDONEXIT,
	DH_ENDEVENT,
	DH_IFGOTO,
	DH_IFNOTGOTO,
	DH_GOTO,
	DH_ORLOGICAL,
	DH_ANDLOGICAL,
	DH_ORBITWISE,
	DH_EORBITWISE,
	DH_ANDBITWISE,
	DH_EQUALS,
	DH_NOTEQUALS,
	DH_LESSTHAN,
	DH_LESSTHANEQUALS,
	DH_GREATERTHAN,
	DH_GREATERTHANEQUALS,
	DH_NEGATELOGICAL,
	DH_LSHIFT,
	DH_RSHIFT,
	DH_ADD,
	DH_SUBTRACT,
	DH_UNARYMINUS,
	DH_MULTIPLY,
	DH_DIVIDE,
	DH_MODULUS,
	DH_PUSHNUMBER,
	DH_PUSHSTRINGINDEX,
	DH_PUSHGLOBALVAR,
	DH_PUSHLOCALVAR,
	DH_DROPSTACKPOSITION,
	DH_SCRIPTVARLIST,
	DH_STRINGLIST,
	DH_INCGLOBALVAR,
	DH_DECGLOBALVAR,
	DH_ASSIGNGLOBALVAR,
	DH_ADDGLOBALVAR,
	DH_SUBGLOBALVAR,
	DH_MULGLOBALVAR,
	DH_DIVGLOBALVAR,
	DH_MODGLOBALVAR,
	DH_INCLOCALVAR,
	DH_DECLOCALVAR,
	DH_ASSIGNLOCALVAR,
	DH_ADDLOCALVAR,
	DH_SUBLOCALVAR,
	DH_MULLOCALVAR,
	DH_DIVLOCALVAR,
	DH_MODLOCALVAR,
	DH_CASEGOTO,
	DH_DROP,
	DH_INCGLOBALARRAY,
	DH_DECGLOBALARRAY,
	DH_ASSIGNGLOBALARRAY,
	DH_ADDGLOBALARRAY,
	DH_SUBGLOBALARRAY,
	DH_MULGLOBALARRAY,
	DH_DIVGLOBALARRAY,
	DH_MODGLOBALARRAY,
	DH_PUSHGLOBALARRAY,
	DH_SWAP,
	DH_DUP,
	DH_ARRAYSET,

	NUM_DATAHEADERS

} DATAHEADERS_e;

//*****************************************************************************
//	These are the different bot events that can be posted to a bot's state.
typedef enum
{
	BOTEVENT_KILLED_BYENEMY,
	BOTEVENT_KILLED_BYPLAYER,
	BOTEVENT_KILLED_BYSELF,
	BOTEVENT_KILLED_BYENVIORNMENT,
	BOTEVENT_REACHED_GOAL,
	BOTEVENT_GOAL_REMOVED,
	BOTEVENT_DAMAGEDBY_PLAYER,
	BOTEVENT_PLAYER_SAY,
	BOTEVENT_ENEMY_KILLED,
	BOTEVENT_RESPAWNED,
	BOTEVENT_INTERMISSION,
	BOTEVENT_NEWMAP,
	BOTEVENT_ENEMY_USEDFIST,
	BOTEVENT_ENEMY_USEDCHAINSAW,
	BOTEVENT_ENEMY_FIREDPISTOL,
	BOTEVENT_ENEMY_FIREDSHOTGUN,
	BOTEVENT_ENEMY_FIREDSSG,
	BOTEVENT_ENEMY_FIREDCHAINGUN,
	BOTEVENT_ENEMY_FIREDMINIGUN,
	BOTEVENT_ENEMY_FIREDROCKET,
	BOTEVENT_ENEMY_FIREDGRENADE,
	BOTEVENT_ENEMY_FIREDRAILGUN,
	BOTEVENT_ENEMY_FIREDPLASMA,
	BOTEVENT_ENEMY_FIREDBFG,
	BOTEVENT_ENEMY_FIREDBFG10K,
	BOTEVENT_PLAYER_USEDFIST,
	BOTEVENT_PLAYER_USEDCHAINSAW,
	BOTEVENT_PLAYER_FIREDPISTOL,
	BOTEVENT_PLAYER_FIREDSHOTGUN,
	BOTEVENT_PLAYER_FIREDSSG,
	BOTEVENT_PLAYER_FIREDCHAINGUN,
	BOTEVENT_PLAYER_FIREDMINIGUN,
	BOTEVENT_PLAYER_FIREDROCKET,
	BOTEVENT_PLAYER_FIREDGRENADE,
	BOTEVENT_PLAYER_FIREDRAILGUN,
	BOTEVENT_PLAYER_FIREDPLASMA,
	BOTEVENT_PLAYER_FIREDBFG,
	BOTEVENT_PLAYER_FIREDBFG10K,
	BOTEVENT_USEDFIST,
	BOTEVENT_USEDCHAINSAW,
	BOTEVENT_FIREDPISTOL,
	BOTEVENT_FIREDSHOTGUN,
	BOTEVENT_FIREDSSG,
	BOTEVENT_FIREDCHAINGUN,
	BOTEVENT_FIREDMINIGUN,
	BOTEVENT_FIREDROCKET,
	BOTEVENT_FIREDGRENADE,
	BOTEVENT_FIREDRAILGUN,
	BOTEVENT_FIREDPLASMA,
	BOTEVENT_FIREDBFG,
	BOTEVENT_FIREDBFG10K,
	BOTEVENT_PLAYER_JOINEDGAME,
	BOTEVENT_JOINEDGAME,
	BOTEVENT_DUEL_STARTINGCOUNTDOWN,
	BOTEVENT_DUEL_FIGHT,
	BOTEVENT_DUEL_WINSEQUENCE,
	BOTEVENT_SPECTATING,
	BOTEVENT_LMS_STARTINGCOUNTDOWN,
	BOTEVENT_LMS_FIGHT,
	BOTEVENT_LMS_WINSEQUENCE,
	BOTEVENT_WEAPONCHANGE,
	BOTEVENT_ENEMY_BFGEXPLODE,
	BOTEVENT_PLAYER_BFGEXPLODE,
	BOTEVENT_BFGEXPLODE,
	BOTEVENT_RECEIVEDMEDAL,

	NUM_BOTEVENTS

} BOTEVENT_e;

#endif	// __BOTS_H__
