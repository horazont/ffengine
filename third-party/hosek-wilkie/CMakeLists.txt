set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/")

set(HOSEK_WILKIE_HEADERS
    ArHosekSkyModel.h
    ArHosekSkyModelData_Spectral.h
    ArHosekSkyModelData_RGB.h
    ArHosekSkyModelData_CIEXYZ.h
    )

set(HOSEK_WILKIE_SRC
    ArHosekSkyModel.c)

add_library(hosek-wilkie-skydome STATIC ${HOSEK_WILKIE_SRC} ${HOSEK_WILKIE_HEADERS})
setup_scc_target(hosek-wilkie-skydome)

target_include_directories(
  hosek-wilkie-skydome
  PUBLIC ${INCLUDE_DIRS})
