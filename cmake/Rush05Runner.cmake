# SPDX-License-Identifier: GPL-3.0-or-later

target_sources(nds_runner PRIVATE src/sonic_launcher.cpp src/sonic_pc_settings.cpp)
target_link_libraries(nds_runner PRIVATE comdlg32 shell32)
set_target_properties(nds_runner PROPERTIES OUTPUT_NAME SonicRush WIN32_EXECUTABLE TRUE)
