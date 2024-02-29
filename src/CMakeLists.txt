cmake_minimum_required(VERSION 3.13)

add_library(cgame_obj OBJECT
  bbox.c
  bg_misc.c
  bg_pmove.c
  cg_ammo.c
  cg_cgaz.c
  cg_consolecmds.c
  cg_cvar.c
  cg_draw.c
  cg_entity.c
  cg_gl.c
  cg_hud.c
  cg_jump.c
  cg_main.c
  cg_marks.c
  cg_rl.c
  cg_snap.c
  cg_syscall.c
  cg_timer.c
  cg_utils.c
  cg_view.c
  cg_vm.c
  compass.c
  defrag.c
  g_missile.c
  g_weapon.c
  help.c
  pitch.c
  dynamic_pitch.c
  q_math.c
  q_shared.c
)

target_include_directories(cgame_obj
  PUBLIC ../include
)

target_compile_definitions(cgame_obj PRIVATE BUILDING_SHARED)

target_compile_features(cgame_obj PUBLIC c_std_11)

if(MSVC)
  target_compile_options(cgame_obj PRIVATE
    /WX
    /W4
    /wd4996
    /wd5105 # disable Windows SDK 10.0.19041.0 warnings with C11
  )
  target_link_options(cgame_obj PRIVATE /WX)
else()
  target_compile_options(cgame_obj PRIVATE
    -Werror
    -Wall
    -Wextra
    -pedantic-errors
    -Wmissing-prototypes
    -Wshadow
    -Wstrict-prototypes

    $<$<C_COMPILER_ID:Clang>:-Wunreachable-code-return>
    $<$<C_COMPILER_ID:GNU>:-Wunreachable-code>
  )
endif()

set_target_properties(cgame_obj PROPERTIES
  C_VISIBILITY_PRESET hidden
  POSITION_INDEPENDENT_CODE ON
)

add_library(${BINARY_NAME} SHARED $<TARGET_OBJECTS:cgame_obj>)
set_target_properties(${BINARY_NAME} PROPERTIES
  PREFIX "" IMPORT_PREFIX "" # remove "lib" prefix
)
install(TARGETS ${BINARY_NAME} DESTINATION .)
