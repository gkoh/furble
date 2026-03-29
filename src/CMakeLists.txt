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
