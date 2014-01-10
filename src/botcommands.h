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
// Date created:  5/18/04
//
//
// Filename: botcommands.h
//
// Description: Contains bot structures and prototypes
// [Dusk] Clipped stuff that botc doesn't need.
//
//-----------------------------------------------------------------------------

#ifndef BOTC_BOTCOMMANDS_H
#define BOTC_BOTCOMMANDS_H

#include "bots.h"

//*****************************************************************************
//  DEFINES

// This is the size of the return string for the bot command functions.
#define	BOTCMD_RETURNSTRING_SIZE	256

//*****************************************************************************
typedef enum
{
	BOTCMD_CHANGESTATE,									// Basic botcmd utility functions.
	BOTCMD_DELAY,
	BOTCMD_RAND,
	BOTCMD_STRINGSAREEQUAL,
	BOTCMD_LOOKFORPOWERUPS,								// Search functions.
	BOTCMD_LOOKFORWEAPONS,
	BOTCMD_LOOKFORAMMO,
	BOTCMD_LOOKFORBASEHEALTH,
	BOTCMD_LOOKFORBASEARMOR,
	BOTCMD_LOOKFORSUPERHEALTH,
	BOTCMD_LOOKFORSUPERARMOR,				/* 10 */
	BOTCMD_LOOKFORPLAYERENEMIES,
	BOTCMD_GETCLOSESTPLAYERENEMY,
	BOTCMD_MOVELEFT,									// Movement functions.
	BOTCMD_MOVERIGHT,
	BOTCMD_MOVEFORWARD,
	BOTCMD_MOVEBACKWARDS,
	BOTCMD_STOPMOVEMENT,
	BOTCMD_STOPFORWARDMOVEMENT,
	BOTCMD_STOPSIDEWAYSMOVEMENT,
	BOTCMD_CHECKTERRAIN,					/* 20 */
	BOTCMD_PATHTOGOAL,									// Pathing functions.
	BOTCMD_PATHTOLASTKNOWNENEMYPOSITION,
	BOTCMD_PATHTOLASTHEARDSOUND,
	BOTCMD_ROAM,
	BOTCMD_GETPATHINGCOSTTOITEM,
	BOTCMD_GETDISTANCETOITEM,
	BOTCMD_GETITEMNAME,
	BOTCMD_ISITEMVISIBLE,
	BOTCMD_SETGOAL,
	BOTCMD_BEGINAIMINGATENEMY,				/* 30 */	// Aiming functions.
	BOTCMD_STOPAIMINGATENEMY,
	BOTCMD_TURN,
	BOTCMD_GETCURRENTANGLE,
	BOTCMD_SETENEMY,									// Enemy functions.
	BOTCMD_CLEARENEMY,
	BOTCMD_ISENEMYALIVE,
	BOTCMD_ISENEMYVISIBLE,
	BOTCMD_GETDISTANCETOENEMY,
	BOTCMD_GETPLAYERDAMAGEDBY,
	BOTCMD_GETENEMYINVULNERABILITYTICKS,	/* 40 */
	BOTCMD_FIREWEAPON,									// Weapon functions.
	BOTCMD_BEGINFIRINGWEAPON,
	BOTCMD_STOPFIRINGWEAPON,
	BOTCMD_GETCURRENTWEAPON,
	BOTCMD_CHANGEWEAPON,
	BOTCMD_GETWEAPONFROMITEM,
	BOTCMD_ISWEAPONOWNED,
	BOTCMD_ISFAVORITEWEAPON,
	BOTCMD_SAY,											// Chat functions.
	BOTCMD_SAYFROMFILE,						/* 50 */
	BOTCMD_SAYFROMCHATFILE,
	BOTCMD_BEGINCHATTING,
	BOTCMD_STOPCHATTING,
	BOTCMD_CHATSECTIONEXISTS,
	BOTCMD_CHATSECTIONEXISTSINFILE,
	BOTCMD_GETLASTCHATSTRING,
	BOTCMD_GETLASTCHATPLAYER,
	BOTCMD_GETCHATFREQUENCY,
	BOTCMD_JUMP,										// Jumping functions.
	BOTCMD_BEGINJUMPING,					/* 60 */
	BOTCMD_STOPJUMPING,
	BOTCMD_TAUNT,										// Other action functions.
	BOTCMD_RESPAWN,
	BOTCMD_TRYTOJOINGAME,
	BOTCMD_ISDEAD,										// Information about self functions.
	BOTCMD_ISSPECTATING,
	BOTCMD_GETHEALTH,
	BOTCMD_GETARMOR,
	BOTCMD_GETBASEHEALTH,
	BOTCMD_GETBASEARMOR,					/* 70 */
	BOTCMD_GETBOTSKILL,									// Botskill functions.
	BOTCMD_GETACCURACY,
	BOTCMD_GETINTELLECT,
	BOTCMD_GETANTICIPATION,
	BOTCMD_GETEVADE,
	BOTCMD_GETREACTIONTIME,
	BOTCMD_GETPERCEPTION,
	BOTCMD_SETSKILLINCREASE,							// Botskill modifying functions functions.
	BOTCMD_ISSKILLINCREASED,
	BOTCMD_SETSKILLDECREASE,				/* 80 */
	BOTCMD_ISSKILLDECREASED,
	BOTCMD_GETGAMEMODE,									// Other functions.
	BOTCMD_GETSPREAD,
	BOTCMD_GETLASTJOINEDPLAYER,
	BOTCMD_GETPLAYERNAME,
	BOTCMD_GETRECEIVEDMEDAL,
	BOTCMD_ACS_EXECUTE,
	BOTCMD_GETFAVORITEWEAPON,
	BOTCMD_SAYFROMLUMP,
	BOTCMD_SAYFROMCHATLUMP,					/* 90 */
	BOTCMD_CHATSECTIONEXISTSINLUMP,
	BOTCMD_CHATSECTIONEXISTSINCHATLUMP,

	NUM_BOTCMDS

} BOTCMD_e;

#endif	// BOTC_BOTCOMMANDS_H