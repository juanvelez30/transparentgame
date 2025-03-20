#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <set>
#include <cstdlib>
#include <ctime>
#include <fstream>   // Para guardar/cargar en archivo

static const int GRID_ROWS = 5;
static const int GRID_COLS = 5;
static const float CELL_SIZE = 100.0f;

struct Color {
    float r, g, b;
};

struct Circle {
    int row, col;
    float radius;
    Color color;
};

struct Path {
    Color color;
    std::vector<std::pair<int, int>> cells;
};

// ---------- Variables globales ----------
GLFWwindow* window = nullptr;
std::vector<Circle> circles;                
std::vector<Path> confirmedPaths;           
Path activePath;                            
bool isDrawing = false;
int lastGridRow = -1, lastGridCol = -1;
Color activeColor;                          

// Para bloquear celdas y conexiones ya usadas:
std::set<std::pair<int, int>> occupiedCells;
std::set<std::pair<std::pair<int, int>, std::pair<int, int>>> usedPaths;

// Para no iniciar nueva línea desde círculos ya conectados:
std::set<std::pair<int, int>> connectedCircles;

// Colores posibles (cada uno se usará para un par de círculos)
std::vector<Color> colorPool = {
    {1.0f, 0.0f, 0.0f},  // rojo
    {0.0f, 1.0f, 0.0f},  // verde
    {0.0f, 0.5f, 1.0f}   // azul claro
};

// ---------- Funciones de ayuda ----------
float cellCenterX(int col) {
    return col * CELL_SIZE + CELL_SIZE / 2.0f;
}

float cellCenterY(int row) {
    return row * CELL_SIZE + CELL_SIZE / 2.0f;
}

bool isInsideCircle(float mx, float my, const Circle& c) {
    float cx = cellCenterX(c.col);
    float cy = cellCenterY(c.row);
    float dx = mx - cx, dy = my - cy;
    return (dx * dx + dy * dy) <= (c.radius * c.radius);
}

bool isOrthogonalMove(int lastR, int lastC, int r, int c) {
    return (lastR == r && std::abs(lastC - c) == 1) ||
           (lastC == c && std::abs(lastR - r) == 1);
}

std::pair<int, int> getGridCell(float x, float y) {
    return {
        static_cast<int>(y / CELL_SIZE),
        static_cast<int>(x / CELL_SIZE)
    };
}

// ---------- Generar nuevo nivel sin repetir colores ----------
void generateNewLevel() {
    circles.clear();
    confirmedPaths.clear();
    occupiedCells.clear();
    usedPaths.clear();
    connectedCircles.clear();

    std::set<std::pair<int, int>> takenPositions;

    // Un par (2 círculos) por cada color en colorPool
    for (size_t i = 0; i < colorPool.size(); i++) {
        Color color = colorPool[i];
        for (int j = 0; j < 2; j++) {
            int row, col;
            do {
                row = rand() % GRID_ROWS;
                col = rand() % GRID_COLS;
            } while (takenPositions.count({row, col}));
            takenPositions.insert({row, col});
            circles.push_back({row, col, 25.0f, color});
        }
    }
    std::cout << "Nuevo nivel generado (sin repetir colores)\n";
}

// ---------- Revisión de final de nivel ----------
void checkCompletion() {
    // Si TODAS las celdas están ocupadas, se pasa de nivel
    if (occupiedCells.size() == GRID_ROWS * GRID_COLS) {
        std::cout << "¡Nivel completado!\n";
        generateNewLevel();
    }
}

// ---------- Guardar el tablero actual en un archivo ----------
void saveCurrentBoard(const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "No se pudo abrir el archivo para guardar: " << filename << "\n";
        return;
    }

    out << circles.size() << "\n";
    for (auto& c : circles) {
        out << c.row << " " << c.col << " "
            << c.color.r << " " << c.color.g << " " << c.color.b << "\n";
    }

    out.close();
    std::cout << "Tablero guardado en " << filename << "\n";
}

// ---------- Cargar un tablero desde archivo ----------
void loadBoardFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "No se pudo abrir el archivo para cargar: " << filename << "\n";
        return;
    }

    circles.clear();
    confirmedPaths.clear();
    occupiedCells.clear();
    usedPaths.clear();
    connectedCircles.clear();

    size_t n;
    in >> n;
    for (size_t i = 0; i < n; i++) {
        Circle c;
        in >> c.row >> c.col >> c.color.r >> c.color.g >> c.color.b;
        c.radius = 25.0f;
        circles.push_back(c);
    }
    in.close();
    std::cout << "Tablero cargado desde " << filename << "\n";
}

// ---------- Callbacks de GLFW ----------
void mouse_button_callback(GLFWwindow* win, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(win, &mx, &my);

            for (auto& c : circles) {
                if (isInsideCircle(static_cast<float>(mx), static_cast<float>(my), c)) {
                    // Evita iniciar en un círculo ya conectado
                    if (connectedCircles.count({c.row, c.col})) {
                        return;
                    }
                    isDrawing = true;
                    activeColor = c.color;
                    activePath.color = activeColor;
                    activePath.cells.clear();
                    lastGridRow = c.row;
                    lastGridCol = c.col;
                    // Agrega la celda inicial a la ruta
                    activePath.cells.push_back({c.row, c.col});
                    return;
                }
            }
        }
        else if (action == GLFW_RELEASE && isDrawing) {
            double mx, my;
            glfwGetCursorPos(win, &mx, &my);

            bool matchedColor = false;
            for (auto& c : circles) {
                if (isInsideCircle(static_cast<float>(mx), static_cast<float>(my), c)) {
                    // ¿Es círculo del mismo color?
                    if (c.color.r == activeColor.r &&
                        c.color.g == activeColor.g &&
                        c.color.b == activeColor.b) {
                        
                        // Confirmar la ruta
                        confirmedPaths.push_back(activePath);

                        // Marcar celdas y conexiones usadas
                        for (size_t i = 0; i < activePath.cells.size(); i++) {
                            if (i > 0) {
                                usedPaths.insert({activePath.cells[i - 1], activePath.cells[i]});
                                usedPaths.insert({activePath.cells[i], activePath.cells[i - 1]});
                            }
                            occupiedCells.insert(activePath.cells[i]);
                        }

                        // Marca los círculos como conectados
                        connectedCircles.insert(activePath.cells.front());
                        connectedCircles.insert({c.row, c.col});

                        matchedColor = true;
                        checkCompletion();
                        break;
                    }
                }
            }
            // Si no coincidió el color, la ruta se descarta
            activePath.cells.clear();
            isDrawing = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* win, double mx, double my) {
    if (isDrawing) {
        auto [r, c] = getGridCell(static_cast<float>(mx), static_cast<float>(my));
        if (r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS) {
            // Moverse solo a celdas adyacentes
            if (isOrthogonalMove(lastGridRow, lastGridCol, r, c)) {
                // Evitar cruzar celdas ya ocupadas o rutas ya trazadas
                if (occupiedCells.count({r, c}) ||
                    usedPaths.count({{lastGridRow, lastGridCol}, {r, c}})) {
                    return;
                }
                lastGridRow = r;
                lastGridCol = c;
                activePath.cells.push_back({r, c});
            }
        }
    }
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_R:
                generateNewLevel();
                break;
            case GLFW_KEY_S:
                saveCurrentBoard("miTablero.txt");
                break;
            case GLFW_KEY_L:
                loadBoardFromFile("miTablero.txt");
                break;
            default:
                break;
        }
    }
}

// ---------- Funciones de dibujo ----------
void drawGrid() {
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    glBegin(GL_LINES);
    for (int r = 0; r <= GRID_ROWS; r++) {
        glVertex2f(0.0f, r * CELL_SIZE);
        glVertex2f(GRID_COLS * CELL_SIZE, r * CELL_SIZE);
    }
    for (int c = 0; c <= GRID_COLS; c++) {
        glVertex2f(c * CELL_SIZE, 0.0f);
        glVertex2f(c * CELL_SIZE, GRID_ROWS * CELL_SIZE);
    }
    glEnd();
}

void drawCircles() {
    for (auto& c : circles) {
        float cx = cellCenterX(c.col);
        float cy = cellCenterY(c.row);
        glColor4f(c.color.r, c.color.g, c.color.b, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= 36; i++) {
            float angle = i * (2.0f * 3.14159f / 36);
            glVertex2f(cx + std::cos(angle) * c.radius,
                       cy + std::sin(angle) * c.radius);
        }
        glEnd();
    }
}

void drawPath(const Path& path) {
    glColor3f(path.color.r, path.color.g, path.color.b);
    glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    for (auto& cell : path.cells) {
        float x = cellCenterX(cell.second);
        float y = cellCenterY(cell.first);
        glVertex2f(x, y);
    }
    glEnd();
}

// ---------- Programa principal ----------
int main() {
    srand(static_cast<unsigned>(time(0)));
    if (!glfwInit()) {
        std::cerr << "Error al inicializar GLFW\n";
        return -1;
    }

    // Habilitar framebuffer transparente
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

    window = glfwCreateWindow(
        GRID_COLS * CELL_SIZE, 
        GRID_ROWS * CELL_SIZE, 
        "Juego de Conectar - Fondo Translúcido", 
        nullptr, 
        nullptr
    );
    if (!window) {
        std::cerr << "Error al crear la ventana\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    // Restablecemos todos los callbacks para que se mantengan las funcionalidades
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // Configurar blending para transparencia
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Fondo translúcido (negro 30% de opacidad)
    glClearColor(0.0f, 0.0f, 0.0f, 0.3f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, GRID_COLS * CELL_SIZE, GRID_ROWS * CELL_SIZE, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    generateNewLevel();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        drawGrid();
        drawCircles();

        for (auto& path : confirmedPaths) {
            drawPath(path);
        }
        if (!activePath.cells.empty()) {
            drawPath(activePath);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
