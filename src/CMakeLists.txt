#set(icon_sources
#    icons/icon_add_a_photo.c
#    icons/icon_all_inclusive.c
#    icons/icon_battery_android_0.c
#    icons/icon_battery_android_frame_2.c
#    icons/icon_battery_android_frame_4.c
#    icons/icon_battery_android_frame_6.c
#    icons/icon_battery_android_frame_full.c
#    icons/icon_burst_mode.c
#    icons/icon_camera.c
#    icons/icon_center_focus_strong.c
#    icons/icon_delete.c
#    icons/icon_info.c
#    icons/icon_linked_camera.c
#    icons/icon_location_disabled.c
#    icons/icon_location_searching.c
#    icons/icon_my_location.c
#    icons/icon_no_photography.c
#    icons/icon_palette.c
#    icons/icon_power_settings_new.c
#    icons/icon_remote_gen.c
#    icons/icon_settings_brightness.c
#    icons/icon_settings.c
#    icons/icon_settings_remote.c
#    icons/icon_timer.c
#    icons/icon_undo.c
#    icons/icon_wand_stars.c)

set(furble_sources
    FurbleControl.cpp
    FurbleGPS.cpp
    FurbleSettings.cpp
    FurbleSpinValue.cpp
    FurbleUI.cpp
    FurbleUIIntervalometer.cpp
    main.cpp)

idf_component_register(SRCS ${furble_sources}
                       REQUIRES icons)
