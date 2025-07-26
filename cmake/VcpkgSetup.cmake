function(find_or_install_vcpkg OUT_VAR)
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
            return()
        endif()
    endforeach()

    # 没找到已安装的 vcpkg，执行自动安装到 ${CMAKE_SOURCE_DIR}/vcpkg
    set(_install_path "${CMAKE_SOURCE_DIR}/vcpkg")
    message(WARNING "未找到 vcpkg，将自动安装到：${_install_path}")

    execute_process(
        COMMAND git clone https://github.com/microsoft/vcpkg.git "${_install_path}"
        RESULT_VARIABLE clone_result
    )
    if(NOT clone_result EQUAL 0)
        message(FATAL_ERROR "vcpkg 克隆失败，请检查 git 是否安装，并具备网络访问权限")
    endif()

    # 执行 bootstrap（适配不同平台）
    if(WIN32)
        execute_process(
            COMMAND cmd /c bootstrap-vcpkg.bat
            WORKING_DIRECTORY "${_install_path}"
            RESULT_VARIABLE bootstrap_result
        )
    else()
        execute_process(
            COMMAND ./bootstrap-vcpkg.sh
            WORKING_DIRECTORY "${_install_path}"
            RESULT_VARIABLE bootstrap_result
        )
    endif()

    if(NOT bootstrap_result EQUAL 0)
        message(FATAL_ERROR "vcpkg 安装失败，请手动检查 ${_install_path} 下的 bootstrap 日志")
    endif()

    set(${OUT_VAR} "${_install_path}" PARENT_SCOPE)
endfunction()
