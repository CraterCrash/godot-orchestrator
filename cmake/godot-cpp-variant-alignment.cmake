## This file is part of the Godot Orchestrator project.
##
## Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##		http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
## Workaround for a godot-cpp defect (GH-954).
##
## `godot::Variant` stores its payload in a bare `uint8_t opaque[]` with no
## alignment, so `alignof(godot::Variant)` is 1, while the engine's own Variant
## is 8-byte aligned. Any by-value Variant member may therefore land on an
## unaligned offset. The engine still reads those bytes through the GDExtension
## interface using 8-byte accesses, which x86_64 and arm64 tolerate but
## armeabi-v7a reports as SIGBUS / BUS_ADRALN.
##
## godot-cpp already fixes this for every generated built-in type in
## binding_generator.py, but `Variant` is hand-written and was missed.
##
## Rather than carry a commit inside the godot-cpp submodule, a corrected copy
## of variant.hpp is generated at configure time and its directory is placed
## ahead of godot-cpp's on the include path. The copy is derived from the
## submodule's own header, so it follows submodule bumps automatically.
##
## Remove this module, and its INCLUDE() in CMakeLists.txt, once the fix lands
## upstream. Until then the shim disables itself when it detects the fix.

SET(VARIANT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/extern/godot-cpp/include/godot_cpp/variant/variant.hpp")
SET(VARIANT_SHIM_DIR "${CMAKE_BINARY_DIR}/godot-cpp-align-shim")

# Re-run configuration when the submodule's header changes, so a submodule bump
# regenerates the copy instead of silently reusing a stale one.
SET_PROPERTY(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${VARIANT_HEADER}")

IF (NOT EXISTS "${VARIANT_HEADER}")
    MESSAGE(FATAL_ERROR "Cannot apply the Variant alignment shim, ${VARIANT_HEADER} does not exist. "
            "Has the godot-cpp submodule been checked out?")
ENDIF ()

FILE(READ "${VARIANT_HEADER}" VARIANT_SOURCE)

STRING(FIND "${VARIANT_SOURCE}" "alignas(8) uint8_t opaque[GODOT_CPP_VARIANT_SIZE]" VARIANT_ALREADY_ALIGNED)
IF (VARIANT_ALREADY_ALIGNED GREATER -1)
    MESSAGE(STATUS "godot-cpp aligns Variant upstream, alignment shim skipped - cmake/godot-cpp-variant-alignment.cmake can be removed")
ELSE ()
    STRING(REPLACE
            "uint8_t opaque[GODOT_CPP_VARIANT_SIZE]"
            "alignas(8) uint8_t opaque[GODOT_CPP_VARIANT_SIZE]"
            VARIANT_PATCHED "${VARIANT_SOURCE}")

    # Fail loudly rather than silently building an unshimmed, crash-prone binary
    # if godot-cpp ever restructures the declaration.
    IF (VARIANT_PATCHED STREQUAL VARIANT_SOURCE)
        MESSAGE(FATAL_ERROR "The Variant alignment shim did not match anything in ${VARIANT_HEADER}. "
                "The declaration changed upstream, so cmake/godot-cpp-variant-alignment.cmake must be "
                "updated or removed.")
    ENDIF ()

    FILE(WRITE "${VARIANT_SHIM_DIR}/variant.hpp.staged" "${VARIANT_PATCHED}")
    EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${VARIANT_SHIM_DIR}/variant.hpp.staged"
            "${VARIANT_SHIM_DIR}/godot_cpp/variant/variant.hpp")

    # BEFORE, and at directory scope prior to godot-cpp being added, so that the
    # shim precedes godot-cpp's own include directory for every target. godot-cpp
    # itself must see it too, otherwise the static library and the extension
    # disagree on the layout of anything holding a Variant.
    INCLUDE_DIRECTORIES(BEFORE "${VARIANT_SHIM_DIR}")

    MESSAGE(STATUS "Applied godot-cpp Variant alignment shim (GH-954)")
ENDIF ()