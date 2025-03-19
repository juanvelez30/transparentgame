## Compilar en MacOS Sequoia

clang++ -std=c++11 -stdlib=libc++ \
  -DGL_SILENCE_DEPRECATION \
  -I/usr/local/include \
  -I$(xcrun --show-sdk-path)/usr/include/c++/v1 \
  -isysroot $(xcrun --show-sdk-path) \
  main.cpp -o juego -L/usr/local/lib -lglfw -framework OpenGL
