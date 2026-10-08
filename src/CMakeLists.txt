if(IDF_TARGET STREQUAL "esp32c3")
    set(furble_sources
        FurbleControl.cpp
        FurbleSettings.cpp
        FurbleSpinValue.cpp
        FurbleIntervalometer.cpp
        x3/FurbleX3App.cpp
        x3/FurbleX3Platform.cpp
        x3/main.cpp)
    idf_component_register(SRCS ${furble_sources}
                           INCLUDE_DIRS "../include"
                           PRIV_REQUIRES driver esp_adc esp_pm esp_timer nvs_flash)
    return()
endif()

set(furble_sources
    FurbleCalibrate.cpp
    FurbleControl.cpp
    FurbleGPS.cpp
    FurblePlatform.cpp
    FurbleSettings.cpp
    FurbleSpinValue.cpp
    FurbleUI.cpp
    FurbleUIIntervalometer.cpp
    main.cpp)

idf_component_register(SRCS ${furble_sources}
                       PRIV_REQUIRES esp_pm
                       REQUIRES icons)
