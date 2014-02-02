/*
	Copyright 2000-2010 Brad Carney
	Copyright 2012-2014 Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Numeric values and stuff from zandronum bots.h

#ifndef BOTC_BOTSTUFF_H
#define BOTC_BOTSTUFF_H

static const int gMaxStates			= 256;
static const int gMaxEvents			= 32;
static const int gMaxGlobalEvents	= 32;
static const int gMaxGlobalVars		= 128;
static const int gMaxGlobalArrays	= 16;
static const int gMaxArraySize		= 65536;
static const int gMaxStateVars		= 16;
static const int gMaxStringlistSize	= 128;
static const int gMaxStringLength	= 256;
static const int gMaxReactionTime	= 52;
static const int gMaxStoredEvents	= 64;

enum EDataHeader
{
	dhCommand,
	dhStateIndex,
	dhStateName,
	dhOnEnter,
	dhMainLoop,
	dhOnExit,
	dhEvent,
	dhEndOnEnter,
	dhEndMainLoop,
	dhEndOnExit,
	dhEndEvent,
	dhIfGoto,
	dhIfNotGoto,
	dhGoto,
	dhOrLogical,
	dhAndLogical,
	dhOrBitwise,
	dhEorBitwise,
	dhAndBitwise,
	dhEquals,
	dhNotEquals,
	dhLessThan,
	dhAtMost,
	dhGreaterThan,
	dhAtLeast,
	dhNegateLogical,
	dhLeftShift,
	dhRightShift,
	dhAdd,
	dhSubtract,
	dhUnaryMinus,
	dhMultiply,
	dhDivide,
	dhModulus,
	dhPushNumber,
	dhPushStringIndex,
	dhPushGlobalVar,
	dhPushLocalVar,
	dhDropStackPosition,
	dhScriptVarList,
	dhStringList,
	dhIncreaseGlobalVar,
	dhDecreaseGlobalVar,
	dhAssignGlobalVar,
	dhAddGlobalVar,
	dhSubtractGlobalVar,
	dhMultiplyGlobalVar,
	dhDivideGlobalVar,
	dhModGlobalVar,
	dhIncreaseLocalVar,
	dhDecreaseLocalVar,
	dhAssignLocalVar,
	dhAddLocalVar,
	dhSubtractLocalVar,
	dhMultiplyLocalVar,
	dhDivideLocalVar,
	dhModLocalVar,
	dhCaseGoto,
	dhDrop,
	dhIncreaseGlobalArray,
	dhDecreaseGlobalArray,
	dhAssignGlobalArray,
	dhAddGlobalArray,
	dhSubtractGlobalArray,
	dhMultiplyGlobalArray,
	dhDivideGlobalArray,
	dhModGlobalArray,
	dhPushGlobalArray,
	dhSwap,
	dhDup,
	dhArraySet,
	numDataHeaders
};

//*****************************************************************************
//	These are the different bot events that can be posted to a bot's state.
enum eEvent
{
	evKilledByEnemy,
	evKilledByPlayer,
	evKilledBySelf,
	evKilledByEnvironment,
	evReachedGoal,
	evGoalRemoved,
	evDamagedByPlayer,
	evPlayerSay,
	evEnemyKilled,
	evRespawned,
	evIntermission,
	evNewMaps,
	evEnemyUsedFist,
	evEnemyUsedChainsaw,
	evEnemyFiredPistol,
	evEnemyFiredShotgun,
	evEnemyFiredSsg,
	evEnemyFiredChaingun,
	evEnemyFiredMinigun,
	evEnemyFiredRocket,
	evEnemyFiredGrenade,
	evEnemyFiredRailgun,
	evEnemyFiredPlasma,
	evEnemyFiredBfg,
	evEnemyFiredBfg10k,
	evPlayerUsedFist,
	evPlayerUsedChainsaw,
	evPlayerFiredPistol,
	evPlayerFiredShotgun,
	evPlayerFiredSsg,
	evPlayerFiredChaingun,
	evPlayerFiredMinigun,
	evPlayerFiredRocket,
	evPlayerFiredGrenade,
	evPlayerFiredRailgun,
	evPlayerFiredPlasma,
	evPlayerFiredBfg,
	evPlayerFiredBfg10k,
	evUsedFist,
	evUsedChainsaw,
	evFiredPistol,
	evFiredShotgun,
	evFiredSsg,
	evFiredChaingun,
	evFiredMinigun,
	evFiredRocket,
	evFiredGrenade,
	evFiredRailgun,
	evFiredPlasma,
	evFiredBfg,
	evFiredBfg10k,
	evPlayerJoinedGame,
	evJoinedGame,
	evDuelStartingCountdown,
	evDuelFight,
	evDuelWinSequence,
	evSpectating,
	evLmsStartingCountdown,
	evLmsFight,
	evLmsWinSequence,
	evWeaponChange,
	evEnemyBfgExplode,
	evPlayerBfgExplode,
	evBfgExplode,
	evRecievedMedal,

	numBotEvents
};

#endif	// BOTC_BOTSTUFF_H
