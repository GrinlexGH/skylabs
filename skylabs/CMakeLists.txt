# skylabs
set(PROJECTS
    shaders
    launcher
    core
    public
)

foreach(PROJECT ${PROJECTS})
    add_subdirectory(${PROJECT})
endforeach()
