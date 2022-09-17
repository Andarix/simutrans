//
// This file is part of the Simutrans project under the Artistic License.
// (see LICENSE.txt)
//


//
// list  containing all tests
//

include("tests/test_building")
include("tests/test_city")
include("tests/test_climate")
include("tests/test_depot")
include("tests/test_dir")
include("tests/test_factory")
include("tests/test_good")
include("tests/test_halt")
include("tests/test_headquarters")
include("tests/test_label")
include("tests/test_player")
include("tests/test_powerline")
include("tests/test_reservation")
include("tests/test_scenario")
include("tests/test_sign")
include("tests/test_slope")
include("tests/test_terraform")
include("tests/test_trees")
include("tests/test_way_bridge")
include("tests/test_way_road")
include("tests/test_way_runway")
include("tests/test_way_tram")
include("tests/test_way_tunnel")
include("tests/test_wayobj")


all_tests <- [
	test_building_build_house,
	test_building_build_multi_tile_sloped,
	test_building_buy_house_invalid_param,
	test_building_buy_house_from_public_player,
	test_building_buy_house_attraction,
	test_building_rotate_house,
	test_building_rotate_harbour,
	test_building_rotate_station,
	test_building_rotate_factory,
	test_city_add_invalid,
	test_city_add_cannot_afford,
	test_city_add_by_public_player,
	test_city_add_on_existing_townhall,
	test_city_add_near_map_border,
	test_city_change_size_invalid_params,
	test_city_change_size_to_minimum
	test_climate_invalid,
	test_climate_flat,
	test_climate_cliff,
	test_depot_build_invalid_params,
	test_depot_build_invalid_pos,
	test_depot_build_road,
	test_depot_build_road_on_tram_crossing,
	test_depot_build_water,
	test_depot_build_monorail,
	test_depot_build_tram,
	test_depot_build_sloped,
	test_depot_build_on_tunnel_entrance,
	test_depot_build_on_bridge_end,
	test_depot_build_on_halt,
	test_depot_convoy_add_normal,
	test_depot_convoy_add_nonelectrified,
	test_dir_is_single,
	test_dir_is_twoway,
	test_dir_is_threeway,
	test_dir_is_curve,
	test_dir_is_straight,
	test_dir_double,
	test_dir_backward,
	test_dir_to_slope,
	test_dir_to_coord,
	test_factory_build_pp,
	test_factory_build_with_fields,
	test_factory_build_climate,
	test_factory_link,
	test_good_is_interchangeable,
	test_good_speed_bonus,
	test_halt_build_rail_single_tile,
	test_halt_build_harbour,
	test_halt_build_air,
	test_halt_build_multi_tile,
	test_halt_build_multi_mode,
	test_halt_build_multi_player,
	test_halt_build_separate,
	test_halt_build_near_factory,
	test_halt_build_near_factories,
	test_halt_build_on_tunnel_entrance,
	test_halt_build_on_bridge_end,
	test_halt_build_on_depot,
	test_halt_build_station_extension,
	test_halt_upgrade_downgrade,
	test_halt_make_public_single,
	test_halt_make_public_multi_tile,
	test_halt_make_public_underground,
	test_halt_move_stop_invalid_param,
	test_headquarters_build_flat,
	test_label,
	test_player_cash,
	test_player_isactive,
	test_player_headquarters,
	test_player_name,
	test_player_lines,
	test_powerline_connect,
	test_powerline_build_below_powerbridge,
	test_powerline_build_powerbridge_above_powerline,
	test_powerline_build_transformer,
	test_powerline_build_over_transformer,
	test_powerline_build_transformer_multiple,
	test_powerline_remove_powerbridge,
	test_powerline_ways,
	test_reservation_clear_ground,
	test_reservation_clear_road,
	test_reservation_clear_rail,
	test_scenario_rules_allow_forbid_tool,
	test_scenario_rules_allow_forbid_way_tool_rect,
	test_scenario_rules_allow_forbid_way_tool_cube,
	test_scenario_rules_allow_forbid_tool_stacked_rect,
	test_scenario_rules_allow_forbid_tool_stacked_cube,
	test_sign_build_oneway,
	test_sign_build_trafficlight,
	test_sign_remove_trafficlight,
	test_sign_build_private_way,
	test_sign_build_signal,
	test_sign_build_signal_multiple,
	test_sign_replace_signal,
	test_sign_signal_when_player_removed,
	test_slope_to_dir,
	test_slope_can_set,
	test_slope_set_and_restore,
	test_slope_get_price,
	test_slope_set_near_map_border,
	test_slope_restore_on_foundation,
	test_slope_restore_on_bridge,
	test_slope_restore_on_label,
	test_slope_max_height_diff,
	test_terraform_raise_lower_land,
	test_terraform_raise_lower_land_at_map_border,
	test_terraform_raise_lower_land_at_water_center,
	test_terraform_raise_lower_land_at_water_corner,
	test_terraform_raise_lower_land_at_water_edge,
	test_terraform_raise_lower_land_below_way,
	test_terraform_raise_lower_water_level,
	test_trees_plant_single_invalid_pos,
	test_trees_plant_single_invalid_param,
	test_trees_plant_single_null_param,
	test_trees_plant_single_empty_param,
	test_trees_plant_single_invalid_desc,
	test_trees_plant_single_when_disabled,
	test_trees_plant_single_random_age,
	test_trees_plant_single_ignore_climate,
	test_trees_plant_single_max_per_square,
	test_trees_plant_single_occupied,
	test_trees_plant_forest,
	test_way_bridge_build_ground,
	test_way_bridge_build_at_slope,
	test_way_bridge_build_at_slope_stacked,
	test_way_bridge_build_above_way,
	test_way_bridge_build_above_runway,
	test_way_bridge_planner,
	test_way_road_build_single_tile,
	test_way_road_build_straight,
	test_way_road_build_bend,
	test_way_road_build_parallel,
	test_way_road_build_below_powerline,
	test_way_road_build_crossing,
	test_way_road_upgrade_crossing,
	test_way_road_upgrade_downgrade,
	test_way_road_upgrade_downgrade_across_bridge,
	test_way_road_build_cityroad,
	test_way_road_has_double_slopes,
	test_way_road_make_public,
	test_way_runway_build_rw_flat,
	test_way_runway_build_tw_flat,
	test_way_runway_build_mixed_flat,
	test_way_tram_build_flat,
	test_way_tram_build_parallel,
	test_way_tram_build_on_road,
	test_way_tram_build_across_road_bridge,
	test_way_tram_build_across_crossing,
	test_way_tram_build_in_tunel,
	test_way_tram_has_double_slopes,
	test_way_tunnel_build_straight,
	test_way_tunnel_build_up_down,
	test_way_tunnel_make_public,
	test_wayobj_build_straight,
	test_wayobj_build_disconnected,
	test_wayobj_upgrade_downgrade,
	test_wayobj_electrify_depot
]
