# glad - OpenGL 3.3 core function loader.
#
# Generated with glad 2.0.8 (https://github.com/Dav1dde/glad):
#     glad --api gl:core=3.3 --extensions "" --out-path third_party/glad c
# Generated code license: (WTFPL OR CC0-1.0) AND Apache-2.0 (khrplatform.h).
# It is third-party code, so the project warning level is not applied.

add_library(glad STATIC src/gl.c)
target_include_directories(glad PUBLIC include)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
