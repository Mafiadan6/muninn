# Core library sources
set(MUNINN_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/muninn.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/usb_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/odin_protocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware/firmware_package.cpp
    PARENT_SCOPE)