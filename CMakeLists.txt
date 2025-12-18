cmake_minimum_required(VERSION 3.14)

project(KmapRaylib)

set(CMAKE_CXX_STANDARD 17)

# --- 設定靜態連結 (讓 exe 不依賴外部 DLL) ---
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- 下載 Raylib ---
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/heads/master.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE 
)
FetchContent_MakeAvailable(raylib)

# --- 加入原始碼 ---
add_executable(KmapApp main.cpp logo.rc)

# 設定為視窗程式 (隱藏黑色 Console)
set_target_properties(KmapApp PROPERTIES WIN32_EXECUTABLE ON)

# ★★★★★【關鍵修正】★★★★★
# 告訴編譯器：即使是視窗程式，也要從 main() 開始執行
if(MSVC)
    target_link_options(KmapApp PRIVATE "/ENTRY:mainCRTStartup")
endif()
# ★★★★★★★★★★★★★★★★★★★

target_link_libraries(KmapApp raylib)