set(COMPONENT_SRCS
    icon_add_a_photo.c
    icon_add_a_photo_24.c
    icon_all_inclusive.c
    icon_arrow_back_24.c
    icon_arrow_downward_24.c
    icon_arrow_forward_24.c
    icon_arrow_upward_24.c
    icon_battery_android_0.c
    icon_battery_android_frame_2.c
    icon_battery_android_frame_4.c
    icon_battery_android_frame_6.c
    icon_battery_android_frame_full.c
    icon_camera.c
    icon_camera_24.c
    icon_center_focus_strong.c
    icon_center_focus_strong_24.c
    icon_check_24.c
    icon_delete.c
    icon_delete_24.c
    icon_info.c
    icon_info_24.c
    icon_linked_camera.c
    icon_linked_camera_24.c
    icon_location_disabled.c
    icon_location_searching.c
    icon_location_searching_24.c
    icon_lock.c
    icon_lock_24.c
    icon_lock_open_right.c
    icon_lock_open_right_24.c
    icon_my_location.c
    icon_no_photography.c
    icon_no_photography_24.c
    icon_palette.c
    icon_palette_24.c
    icon_power_settings_new.c
    icon_power_settings_new_24.c
    icon_remote_gen.c
    icon_remote_gen_24.c
    icon_settings.c
    icon_settings_24.c
    icon_settings_brightness.c
    icon_settings_brightness_24.c
    icon_settings_remote.c
    icon_settings_remote_24.c
    icon_timer.c
    icon_timer_24.c
    icon_undo.c
    icon_wand_stars.c
    icon_wand_stars_24.c)

set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -DLV_LVGL_H_INCLUDE_SIMPLE=1)
