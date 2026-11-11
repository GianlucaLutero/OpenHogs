# audio: sound effects, speech and music.
#
#   OggDecoder     - decodes .OGG music with stb_vorbis
#   Mixer          - mixes playing sounds into one stereo stream (pure CPU,
#                    no audio device, so it can be unit tested)
#   SoundLibrary   - loads sounds, sound banks and music through the VFS,
#                    so content packs can replace any of them
#   AudioSettings  - volumes from config/openhogs.ini
#
#   AudioDevice    - sends the mixer output to the speakers with SDL3. Kept
#                    in its own library so tools and tests do not need SDL.

add_library(openhogs_audio STATIC
    src/AudioSettings.cpp
    src/Mixer.cpp
    src/OggDecoder.cpp
    src/SoundLibrary.cpp)

add_library(OpenHogs::audio ALIAS openhogs_audio)

target_include_directories(openhogs_audio PUBLIC include)
target_link_libraries(openhogs_audio PUBLIC OpenHogs::core OpenHogs::formats PRIVATE stb_vorbis)
openhogs_set_warnings(openhogs_audio)

if(OPENHOGS_BUILD_VIEWER)
    add_library(openhogs_audio_device STATIC src/AudioDevice.cpp)
    add_library(OpenHogs::audio_device ALIAS openhogs_audio_device)
    target_include_directories(openhogs_audio_device PUBLIC include)
    target_link_libraries(openhogs_audio_device PUBLIC OpenHogs::audio PRIVATE SDL3::SDL3)
    openhogs_set_warnings(openhogs_audio_device)
endif()
