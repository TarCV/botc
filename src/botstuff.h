// Macros and enums from zandronum bots.h

#ifndef BOTC_BOTSTUFF_H
#define BOTC_BOTSTUFF_H

static const int g_max_states			= 256;
static const int g_max_events			= 32;
static const int g_max_global_events	= 32;
static const int g_max_global_vars		= 128;
static const int g_max_global_arrays	= 16;
static const int g_max_array_size		= 65536;
static const int g_max_state_vars		= 16;
static const int g_max_stringlist_size	= 128;
static const int g_max_string_length	= 256;
static const int g_max_reaction_time	= 52;
static const int g_max_stored_events	= 64;

enum e_data_header
{
	dh_command,
	dh_state_index,
	dh_state_name,
	dh_on_enter,
	dh_main_loop,
	dh_on_exit,
	dh_event,
	dh_end_on_enter,
	dh_end_main_loop,
	dh_end_on_exit,
	dh_end_event,
	dh_if_goto,
	dh_if_not_goto,
	dh_goto,
	dh_or_logical,
	dh_and_logical,
	dh_or_bitwise,
	dh_eor_bitwise,
	dh_and_bitwise,
	dh_equals,
	dh_not_equals,
	dh_less_than,
	dh_at_most,
	dh_greater_than,
	dh_at_least,
	dh_negate_logical,
	dh_left_shift,
	dh_right_shift,
	dh_add,
	dh_subtract,
	dh_unary_minus,
	dh_multiply,
	dh_divide,
	dh_modulus,
	dh_push_number,
	dh_push_string_index,
	dh_push_global_var,
	dh_push_local_var,
	dh_drop_stack_position,
	dh_script_var_list,
	dh_string_list,
	dh_increase_global_var,
	dh_decrease_global_var,
	dh_assign_global_var,
	dh_add_global_var,
	dh_subtract_global_var,
	dh_multiply_global_var,
	dh_divide_global_var,
	dh_mod_global_var,
	dh_increase_local_var,
	dh_decrease_local_var,
	dh_assign_local_var,
	dh_add_local_var,
	dh_subtract_local_var,
	dh_multiply_local_var,
	dh_divide_local_var,
	dh_mod_local_var,
	dh_case_goto,
	dh_drop,
	dh_increase_global_array,
	dh_decrease_global_array,
	dh_assign_global_array,
	dh_add_global_array,
	dh_subtract_global_array,
	dh_multiply_global_array,
	dh_divide_global_array,
	dh_mod_global_array,
	dh_push_global_array,
	dh_swap,
	dh_dup,
	dh_array_set,
	num_data_headers
};

//*****************************************************************************
//	These are the different bot events that can be posted to a bot's state.
enum e_event
{
	ev_killed_by_enemy,
	ev_killed_by_player,
	ev_killed_by_self,
	ev_killed_by_environment,
	ev_reached_goal,
	ev_goal_removed,
	ev_damaged_by_player,
	ev_player_say,
	ev_enemy_killed,
	ev_respawned,
	ev_intermission,
	ev_new_maps,
	ev_enemy_used_fist,
	ev_enemy_used_chainsaw,
	ev_enemy_fired_pistol,
	ev_enemy_fired_shotgun,
	ev_enemy_fired_ssg,
	ev_enemy_fired_chaingun,
	ev_enemy_fired_minigun,
	ev_enemy_fired_rocket,
	ev_enemy_fired_grenade,
	ev_enemy_fired_railgun,
	ev_enemy_fired_plasma,
	ev_enemy_fired_bfg,
	ev_enemy_fired_bfg10k,
	ev_player_used_fist,
	ev_player_used_chainsaw,
	ev_player_fired_pistol,
	ev_player_fired_shotgun,
	ev_player_fired_ssg,
	ev_player_fired_chaingun,
	ev_player_fired_minigun,
	ev_player_fired_rocket,
	ev_player_fired_grenade,
	ev_player_fired_railgun,
	ev_player_fired_plasma,
	ev_player_fired_bfg,
	ev_player_fired_bfg10k,
	ev_used_fist,
	ev_used_chainsaw,
	ev_fired_pistol,
	ev_fired_shotgun,
	ev_fired_ssg,
	ev_fired_chaingun,
	ev_fired_minigun,
	ev_fired_rocket,
	ev_fired_grenade,
	ev_fired_railgun,
	ev_fired_plasma,
	ev_fired_bfg,
	ev_fired_bfg10k,
	ev_player_joined_game,
	ev_joined_game,
	ev_duel_starting_countdown,
	ev_duel_fight,
	ev_duel_win_sequence,
	ev_spectating,
	ev_lms_starting_countdown,
	ev_lms_fight,
	ev_lms_win_sequence,
	ev_weapon_change,
	ev_enemy_bfg_explode,
	ev_player_bfg_explode,
	ev_bfg_explode,
	ev_recieved_medal,

	num_bot_events
};

#endif	// BOTC_BOTSTUFF_H
