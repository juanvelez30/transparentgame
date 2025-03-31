cmake_minimum_required(VERSION 3.10)
project(GAMEOPENGL)

set(CMAKE_CXX_STANDARD 11)

if(APPLE)
    # Usa el SDK de Xcode (macOS 15.2) para evitar problemas con los Command Line Tools
    set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk")
    # Forzamos al compilador a usar este sysroot y la STL de libc++
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT} -stdlib=libc++")
    
    # Agrega la ruta a los headers de libc++ (la STL de Clang en macOS)
    include_directories("${CMAKE_OSX_SYSROOT}/usr/include/c++/v1")
    
    message(STATUS "Forzando sysroot: ${CMAKE_OSX_SYSROOT}")
    message(STATUS "Incluyendo: ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1")
    
    # Agregar directorio de librerías para que el linker encuentre assimp
    if(EXISTS "/usr/local/lib")
        link_directories("/usr/local/lib")
        message(STATUS "Agregando /usr/local/lib al directorio de búsqueda de librerías")
    elseif(EXISTS "/opt/homebrew/lib")
        link_directories("/opt/homebrew/lib")
        message(STATUS "Agregando /opt/homebrew/lib al directorio de búsqueda de librerías")
    endif()
    
    # Agregar ruta para GLM (ajusta la ruta según tu sistema)
    include_directories(/usr/local/include)  # O /opt/homebrew/include para Apple Silicon
    message(STATUS "Incluyendo GLM desde: /usr/local/include")
endif()

# 1. Buscar dependencias
find_package(OpenGL REQUIRED)
find_package(assimp REQUIRED)
find_package(glfw3 REQUIRED)

# 2. Incluir directorios de cabeceras
#    - El de glad, que contiene glad.h y KHR/khrplatform.h
#    - Opcionalmente, un directorio 'include' con tus propios headers
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 3. Lista de archivos fuente
#    - Ajusta las rutas reales de cube.cpp y glad.c
set(SOURCES
    src/cube.cpp
    glad/src/glad.c
)

# 4. Crear el ejecutable
add_executable(cubo ${SOURCES})

# 5. Enlazar librerías
target_link_libraries(cubo
    ${OPENGL_LIBRARIES}
    glfw
    assimp
)
