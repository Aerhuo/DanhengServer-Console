function(find_or_install_vcpkg OUT_VAR)
    set(options QUIET NO_INSTALL)
    cmake_parse_arguments(FIV "" "${options}" "" ${ARGN})

    set(_candidate_paths
        "$ENV{VCPKG_ROOT}"
        "${CMAKE_SOURCE_DIR}/vcpkg"
        "${CMAKE_SOURCE_DIR}/../vcpkg"
        "C:/vcpkg"
        "/opt/vcpkg"
    )

    foreach(path IN LISTS _candidate_paths)
        if(EXISTS "${path}/scripts/buildsystems/vcpkg.cmake")
            set(${OUT_VAR} "${path}" PARENT_SCOPE)
            message(STATUS "找到已安装的 vcpkg 于 ${path}")
            return()
        endif()
    endforeach()

    if(FIV_NO_INSTALL)
        message(FATAL_ERROR "未找到 vcpkg，但禁用了自动安装功能")
    endif()

    set(_install_path "${CMAKE_SOURCE_DIR}/vcpkg")
    message(WARNING "未找到 vcpkg，将自动安装到：${_install_path}")

    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(FATAL_ERROR "Git 未安装，无法克隆 vcpkg")
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone https://github.com/microsoft/vcpkg.git "${_install_path}"
        RESULT_VARIABLE clone_result
    )
    if(NOT clone_result EQUAL 0)
        message(FATAL_ERROR "vcpkg 克隆失败，请检查网络和 Git 设置")
    endif()

    if(WIN32)
        if(NOT EXISTS "${_install_path}/bootstrap-vcpkg.bat")
            message(FATAL_ERROR "缺失 bootstrap-vcpkg.bat，可能克隆失败")
        endif()
        execute_process(
            COMMAND cmd /c bootstrap-vcpkg.bat
            WORKING_DIRECTORY "${_install_path}"
            RESULT_VARIABLE bootstrap_result
        )
    else()
        if(NOT EXISTS "${_install_path}/bootstrap-vcpkg.sh")
            message(FATAL_ERROR "缺失 bootstrap-vcpkg.sh，可能克隆失败")
        endif()
        execute_process(
            COMMAND ./bootstrap-vcpkg.sh
            WORKING_DIRECTORY "${_install_path}"
            RESULT_VARIABLE bootstrap_result
        )
    endif()

    if(NOT bootstrap_result EQUAL 0)
        message(FATAL_ERROR "vcpkg 安装失败，请检查 bootstrap 日志")
    endif()

    message(STATUS "vcpkg 安装成功，位于：${_install_path}")

    # 自动执行 install（仅当 vcpkg.json 存在）
    if(EXISTS "${CMAKE_SOURCE_DIR}/vcpkg.json")
        execute_process(
            COMMAND "${_install_path}/vcpkg" install --x-manifest-root="${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE install_result
        )
        if(NOT install_result EQUAL 0)
            message(FATAL_ERROR "vcpkg install 执行失败，请检查 ${CMAKE_SOURCE_DIR}/vcpkg.json")
        else()
            message(STATUS "已自动执行 vcpkg install")
        endif()
    else()
        message(WARNING "未找到 vcpkg.json，跳过自动安装包步骤")
    endif()

    set(${OUT_VAR} "${_install_path}" PARENT_SCOPE)
endfunction()