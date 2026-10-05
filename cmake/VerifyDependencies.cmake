function(island_read_lock name output)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock" _lock_lines
         REGEX "^${name}=[0-9a-f]+$")
    list(LENGTH _lock_lines _lock_count)
    if(NOT _lock_count EQUAL 1)
        message(FATAL_ERROR "dependencies.lock must contain exactly one ${name}=<commit> entry")
    endif()
    list(GET _lock_lines 0 _lock_line)
    string(REPLACE "${name}=" "" _lock_revision "${_lock_line}")
    set(${output} "${_lock_revision}" PARENT_SCOPE)
endfunction()

function(island_verify_git_dependency name root required_branch required_revision)
    if(NOT EXISTS "${root}/CMakeLists.txt")
        message(FATAL_ERROR "${name} dependency was not found at '${root}'")
    endif()
    execute_process(
        COMMAND git -C "${root}" rev-parse HEAD
        RESULT_VARIABLE _revision_result
        OUTPUT_VARIABLE _revision
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT _revision_result EQUAL 0)
        message(FATAL_ERROR "${name} at '${root}' is not a readable Git checkout")
    endif()
    execute_process(
        COMMAND git -C "${root}" branch --show-current
        OUTPUT_VARIABLE _branch
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT "${_branch}" STREQUAL "${required_branch}")
        message(FATAL_ERROR
            "${name} must use branch '${required_branch}', but '${root}' is on '${_branch}'. "
            "Do not switch a shared dependency worktree from this project; provide a separate "
            "read-only checkout through the corresponding CMake path variable.")
    endif()
    if(ISLAND_STRICT_DEPENDENCY_REVISIONS AND NOT "${_revision}" STREQUAL "${required_revision}")
        message(FATAL_ERROR
            "${name} revision ${_revision} does not match dependencies.lock "
            "(${required_revision}). Update and requalify the baseline deliberately.")
    endif()
    message(STATUS "island-simulator: ${name} ${_branch} @ ${_revision}")
endfunction()

function(island_verify_cna root required_revision)
    if(NOT EXISTS "${root}/CMakeLists.txt")
        return()
    endif()

    if(EXISTS "${root}/.island-cna-revision")
        file(STRINGS "${root}/.island-cna-revision" _revision LIMIT_COUNT 1)
        if(NOT "${_revision}" STREQUAL "${required_revision}")
            message(FATAL_ERROR
                "The materialized CNA revision ${_revision} does not match dependencies.lock "
                "(${required_revision}).")
        endif()
        message(STATUS "island-simulator: CNA next export @ ${_revision}")
        return()
    endif()

    island_verify_git_dependency("CNA" "${root}" "next" "${required_revision}")
endfunction()

function(island_verify_dependencies)
    island_read_lock(CNA _cna_revision)
    island_read_lock(SHARP_RUNTIME _sharp_revision)
    island_read_lock(EASY_GL _easy_revision)
    island_read_lock(META_GL _meta_revision)

    island_verify_cna("${CNA_ROOT_DIR}" "${_cna_revision}")
    island_verify_git_dependency("Sharp Runtime" "${ISLAND_SHARP_RUNTIME_ROOT}"
                                 "next" "${_sharp_revision}")
    island_verify_git_dependency("EasyGL" "${ISLAND_EASY_GL_ROOT}"
                                 "develop" "${_easy_revision}")
    island_verify_git_dependency("MetaGL" "${ISLAND_META_GL_ROOT}"
                                 "develop" "${_meta_revision}")
endfunction()
