SET(SOURCES screens/ui_Bat1CellsScreen.c
    screens/ui_FaultsScreen.c
    screens/ui_SecretScreen.c
    screens/ui_Bat2CellsScreen.c
    screens/ui_DoubleBatScreen.c
    screens/ui_SingleBatScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})
