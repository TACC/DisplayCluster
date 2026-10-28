# cmake -DFROM=<dir> -DTO=<dir> -P CopyDlls.cmake
# copies every DLL in FROM into TO, skipping ones that are already up to date
# (see the post-build step in CMakeLists.txt for why)
file(GLOB DLLS "${FROM}/*.dll")

foreach(DLL ${DLLS})
    get_filename_component(NAME "${DLL}" NAME)
    if(NOT EXISTS "${TO}/${NAME}" OR "${DLL}" IS_NEWER_THAN "${TO}/${NAME}")
        file(COPY "${DLL}" DESTINATION "${TO}")
    endif()
endforeach()
