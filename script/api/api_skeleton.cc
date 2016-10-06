/** @file api_skeleton.cc Documents the necessary functions to be implemented by a script */

// It is in a C++ file to be processed by Doxygen filters to get function typemasks right

#ifdef DOXYGEN

/**
 * This function is called when the scenario starts. Do all the initializations here,
 * as you cannot initialize global variables with non-built-in squirrel types.
 * @typemask void()
 * @ingroup scen_skel
 */
register_function("start");

/**
 * This function is called when the AI starts. Do all the initializations here,
 * as you cannot initialize global variables with non-built-in squirrel types.
 *
 * @param pl_num the number of the AI player. Call <code>player_x(pl_num)</code> to obtain
 * 		 a corresponding player_x instance. Definitely store this value!
 *
 * @typemask void(int)
 * @ingroup ai_skel
 */
register_function("start");

/**
 * This function is called when a savegame with active scenario is loaded.
 * Do all the initializations and post-processing here.
 * @typemask void()
 * @ingroup scen_skel
 * @ingroup quick_return_func
 */
register_function("resume_game");

/**
 * This function is called when a savegame with active AI player is loaded.
 * Do all the initializations and post-processing here.
 *
 * @param pl_num the number of the AI player. Call <code>player_x(pl_num)</code> to obtain
 * 		 a corresponding player_x instance. Definitely store this value!
 * @typemask void(int)
 * @ingroup ai_skel
 */
register_function("resume_game");

/**
 * The heartbeat of the AI player. Here, all AI-related calculations and work can be done.
 *
 * @typemask void()
 * @ingroup ai_skel
 */
register_function("step");

/**
 * Called at the beginning of a new month.
 * Statistics of the last (complete) month is now in position [1] of any statistics array.
 * @typemask void()
 * @ingroup scen_skel
 * @ingroup ai_skel
 */
register_function("new_month")

/**
 * Called at the beginning of a new year.
 * Statistics of the last (complete) year is now in position [1] of any statistics array.
 * @typemask void()
 * @ingroup scen_skel
 * @ingroup ai_skel
 */
register_function("new_year")

/**
 * Called before starting the scenario.
 * Should return filename of the savegame to be used for this scenario.
 * If it returns "<attach>" the scenario is started with the currently running world.
 * By default returns the string @ref map.file.
 * @returns filename or "<attach>"
 * @typemask string()
 * @ingroup scen_skel
 * @ingroup quick_return_func
 */
register_function("get_map_file")

/**
 * Text shown in the 'About' tab in the scenario info window.
 *
 * There is a default implementation, which returns concatenation
 * of #scenario author, version, short_description.
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 * @ingroup scen_skel
 */
register_function("get_about_text");

/**
 * Text shown in the 'Rules' tab in the scenario info window.
 *
 * Text can contain several html-like tags:
 * - \<p\>, \<br\>: line break
 * - \<h1\>, \<em\>, \<it\>, \<st\>: text between start and end tag will be colored. It's a matter of taste, of course.
 * - \<a href="..."\>: insert hyper link, text between start and end tag will be colored blue.
 *       - link to another tab of scenario info window: href="tabname", where tabname is one of: info, goal, rules, result, about
 *       - link to position on the map: href="(x,y)" or href="(x,y,z)", click on link will jump to the map position
 *
 * @code
 * <h1>Here is an example.</h1>
 * <br>
 * Do not build anything at the position <a href='(47,11)'>near Cologne</a>.
 * The mayor of <a href='(8,15)'>Berlin</a> seems to be frustrated with your airport building capabilities.
 * <br>
 * Your results can be found in the <a href='result'>results</a> tab.
 * @endcode
 *
 * @param pl player number of active player
 * @typemask string(integer)
 * @ingroup scen_skel
 */
register_function("get_rule_text");

/**
 * Text shown in the 'Goal' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 * @ingroup scen_skel
 */
register_function("get_goal_text");

/**
 * Text shown in the 'Info' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 * @ingroup scen_skel
 */
register_function("get_info_text");

/**
 * Text shown in the 'Result' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 * @ingroup scen_skel
 */
register_function("get_result_text");

/**
 * Text shown in the 'Debug' tab in the scenario info window.
 *
 * Html-like tags can be used, see @ref get_rule_text.
 *
 * @param pl player number of active player
 * @typemask string(integer)
 * @ingroup scen_skel
 */
register_function("get_debug_text");

/**
 * Returns string containing the version of the api
 * that the scenario supports.
 * By default returns the string @ref scenario.api.
 *
 * If it returns "*" then this indicates that the scenario works in the most current api version.
 * Currently "112.3" and "120.1" are supported.
 *
 * @typemask string()
 * @ingroup scen_skel
 * @ingroup quick_return_func
 */
register_function("get_api_version");

/**
 * Core function of a scenario: It returns the completion percentage for the
 * specified player.
 *
 * If it returns a negative value the respective player has lost. This state will not be changed
 * even if the function later returns positive values again.
 *
 * If it returns a value >= 100 then the respective player has won. This state will not be changed
 * even if the function later returns lower values again.
 *
 * @param pl player number of active player
 * @typemask integer(integer)
 * @ingroup scen_skel
 */
register_function("is_scenario_completed");


/**
 * Function is called when toolbars are filled with icons or when a tool should be activated.
 *
 * There is a default implementation, which forbids map-editing tools.
 *
 * @attention Results are not transfered over network, use the functions provided in #rules in this case.
 *
 * @param pl player number
 * @param tool_id see @ref tool_ids
 * @param wt waytype of tool
 * @returns true if tool is allowed.
 * @typemask bool(integer,integer,way_types)
 * @ingroup scen_skel
 * @ingroup quick_return_func
 */
register_function("is_tool_allowed");

/**
 * Called when user clicks on a tile to build or remove something.
 *
 * This function is network-aware:
 * Error messages are sent back over network to clients.
 *
 * The error message can contain a coordinate, which is used to show a location on the map.
 * In order to show the right place, use @ref coord_to_string. The must be enclosed in parentheses
 * or prefixed with @ .
 *	@code
		return "You cannot do this. The guy living at (" + coord_to_string({x=47, y=11}) + ") does not like you!"
	@endcode
 *
 * @attention Does not work with waybuilding and all tools that need path-finding, use the functions provided in #rules in this case.
 *
 * @param pl player number
 * @param tool_id see @ref tool_ids
 * @param pos coordinate
 *
 * @return null if allowed, an error message otherwise
 * @typemask string(integer,integer,coord3d)
 * @ingroup scen_skel
 * @ingroup quick_return_func
 */
register_function("is_work_allowed_here");

/**
 * Called when user changed a schedule (closes schedule window).
 *
 * @warning Function will NOT be called in network games.
 *
 * @param pl player number
 * @param schedule the schedule
 *
 * @return null if allowed, an error message otherwise
 * @typemask string(integer,schedule_x)
 * @ingroup scen_skel
 * @ingroup quick_return_func
 */
register_function("is_schedule_allowed");

#endif
