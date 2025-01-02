set(furble_sources
    FurbleControl.cpp
    FurbleGPS.cpp
    FurbleSettings.cpp
    FurbleSpinValue.cpp
    FurbleUI.cpp
    FurbleUIIntervalometer.cpp
    main.cpp)

idf_component_register(SRCS ${furble_sources})
