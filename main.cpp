#include "raylib.h"
#include "font_data.h"
#include "icon_data.h"
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring> // For memcpy

// --- 定義常數 ---
const int VAL_0 = 0;
const int VAL_1 = 1;
const int VAL_X = 2; // Don't Care

// --- 資料結構 ---
struct KMapGroup {
    int r, c, h, w;
    Color color;
    bool operator==(const KMapGroup& other) const {
        return r == other.r && c == other.c && h == other.h && w == other.w;
    }
};

// 用於 Undo 的狀態快照
struct GridState {
    int data[4][4];
};

const int GRAY_CODES[] = { 0, 1, 3, 2 }; 
const char* ROW_LABELS[] = { "00", "01", "11", "10" };
const char* COL_LABELS[] = { "00", "01", "11", "10" };

Color GROUP_COLORS[] = {
    { 255, 0, 127, 255 }, { 0, 255, 255, 255 },
    { 255, 255, 0, 255 }, { 155, 89, 182, 255 },
    { 255, 128, 0, 255 }, { 0, 255, 128, 255 }
};

// --- 輔助函數 ---
bool IsCovered(const KMapGroup& g, int r, int c) {
    int dr = (r - g.r + 4) % 4;
    int dc = (c - g.c + 4) % 4;
    return (dr < g.h && dc < g.w);
}

bool IsSubset(const KMapGroup& sub, const KMapGroup& super) {
    for (int i = 0; i < sub.h; i++) {
        for (int j = 0; j < sub.w; j++) {
            int r = (sub.r + i) % 4;
            int c = (sub.c + j) % 4;
            if (!IsCovered(super, r, c)) return false;
        }
    }
    return true;
}

// --- 框框核心演算法 ---3
std::vector<KMapGroup> SolveKMap(int data[4][4], int targetVal) {
    std::vector<KMapGroup> candidates;
    int shapes[][2] = { {4,4}, {2,4}, {4,2}, {1,4}, {4,1}, {2,2}, {1,2}, {2,1}, {1,1} };
    for (auto& shape : shapes) {
        int h = shape[0];
        int w = shape[1];
        int maxR = (h == 4) ? 1 : 4;
        int maxC = (w == 4) ? 1 : 4;
        for (int r = 0; r < maxR; r++) {
            for (int c = 0; c < maxC; c++) {
                bool isValidGroup = true;
                bool containsTarget = false;
                for (int i = 0; i < h; i++) {
                    for (int j = 0; j < w; j++) {
                        int val = data[(r + i) % 4][(c + j) % 4];
                        if (val != targetVal && val != VAL_X) { isValidGroup = false; break; }
                        if (val == targetVal) containsTarget = true;
                    }
                    if (!isValidGroup) break;
                }
                if (isValidGroup && containsTarget) candidates.push_back({r, c, h, w, WHITE});
            }
        }
    }

    std::vector<KMapGroup> PIs;
    for (size_t i = 0; i < candidates.size(); i++) {
        bool shouldRemove = false;
        for (size_t j = 0; j < candidates.size(); j++) {
            if (i == j) continue;
            bool i_in_j = IsSubset(candidates[i], candidates[j]);
            bool j_in_i = IsSubset(candidates[j], candidates[i]);
            if (i_in_j && j_in_i) { if (i > j) { shouldRemove = true; break; } } 
            else if (i_in_j) { shouldRemove = true; break; }
        }
        if (!shouldRemove) PIs.push_back(candidates[i]);
    }

    std::vector<KMapGroup> solution;
    bool cellCovered[4][4] = {false};
    int targetCount = 0;
    for(int r=0; r<4; r++) for(int c=0; c<4; c++) 
        if(data[r][c] != targetVal) cellCovered[r][c] = true; else targetCount++;

    if (targetCount == 0) return solution;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (data[r][c] == targetVal) {
                KMapGroup* uniqueCover = nullptr;
                int coverCount = 0;
                for (auto& pi : PIs) {
                    if (IsCovered(pi, r, c)) { coverCount++; uniqueCover = &pi; }
                }
                if (coverCount == 1 && uniqueCover != nullptr) {
                    bool alreadyAdded = false;
                    for(auto& s : solution) if(s == *uniqueCover) alreadyAdded = true;
                    if (!alreadyAdded) solution.push_back(*uniqueCover);
                }
            }
        }
    }

    for (auto& s : solution) {
        for (int i = 0; i < s.h; i++) for (int j = 0; j < s.w; j++)
            cellCovered[(s.r + i) % 4][(s.c + j) % 4] = true;
    }

    while (true) {
        int maxUncovered = 0;
        int bestPIIdx = -1;
        for (size_t i = 0; i < PIs.size(); i++) {
            bool inSol = false;
            for(auto& s : solution) if(s == PIs[i]) inSol = true;
            if(inSol) continue;
            int newCover = 0;
            for (int row = 0; row < PIs[i].h; row++) {
                for (int col = 0; col < PIs[i].w; col++) {
                    int r = (PIs[i].r + row) % 4;
                    int c = (PIs[i].c + col) % 4;
                    if (!cellCovered[r][c] && data[r][c] == targetVal) newCover++;
                }
            }
            if (newCover > maxUncovered) { maxUncovered = newCover; bestPIIdx = i; }
        }
        if (maxUncovered == 0) break;
        solution.push_back(PIs[bestPIIdx]);
        for (int i = 0; i < PIs[bestPIIdx].h; i++) for (int j = 0; j < PIs[bestPIIdx].w; j++)
            cellCovered[(PIs[bestPIIdx].r + i) % 4][(PIs[bestPIIdx].c + j) % 4] = true;
    }

    for(size_t i=0; i<solution.size(); i++) solution[i].color = GROUP_COLORS[i % 6];
    return solution;
}

// --- 字串生成 ---
std::string GetTerm(const KMapGroup& g, bool isPOS) {
    if (g.h == 4 && g.w == 4) return isPOS ? "0" : "1";
    int rowAnd = 0b11, rowOr = 0b00;
    for (int i = 0; i < g.h; i++) {
        int code = GRAY_CODES[(g.r + i) % 4]; 
        rowAnd &= code; rowOr |= code;
    }
    int colAnd = 0b11, colOr = 0b00;
    for (int j = 0; j < g.w; j++) {
        int code = GRAY_CODES[(g.c + j) % 4]; 
        colAnd &= code; colOr |= code;
    }
    std::vector<std::string> literals;
    if ((rowAnd & 2) != 0) literals.push_back(isPOS ? "A'" : "A");
    else if ((rowOr & 2) == 0) literals.push_back(isPOS ? "A" : "A'");
    if ((rowAnd & 1) != 0) literals.push_back(isPOS ? "B'" : "B");
    else if ((rowOr & 1) == 0) literals.push_back(isPOS ? "B" : "B'");
    if ((colAnd & 2) != 0) literals.push_back(isPOS ? "C'" : "C");
    else if ((colOr & 2) == 0) literals.push_back(isPOS ? "C" : "C'");
    if ((colAnd & 1) != 0) literals.push_back(isPOS ? "D'" : "D");
    else if ((colOr & 1) == 0) literals.push_back(isPOS ? "D" : "D'");

    std::string term = "";
    if (isPOS) {
        term += "(";
        for (size_t i = 0; i < literals.size(); i++) {
            term += literals[i];
            if (i < literals.size() - 1) term += "+";
        }
        term += ")";
    } else {
        for (const auto& s : literals) term += s;
    }
    return term;
}

std::string GenerateFormula(const std::vector<KMapGroup>& groups, bool isPOS) {
    if (groups.empty()) return isPOS ? "F = 1" : "F = 0";
    std::string formula = "F = ";
    for (size_t i = 0; i < groups.size(); i++) {
        formula += GetTerm(groups[i], isPOS);
        if (i < groups.size() - 1) formula += (isPOS ? "" : " + "); 
    }
    return formula;
}

// --- 繪圖函數 ---
void DrawWrappedGroup(KMapGroup g, int startX, int startY, int cellSize, float alpha, bool isPOS) {
    std::vector<std::pair<int, int>> hSegments; 
    if (g.c + g.w <= 4) hSegments.push_back({g.c, g.w});
    else { hSegments.push_back({g.c, 4 - g.c}); hSegments.push_back({0, g.w - (4 - g.c)}); }

    std::vector<std::pair<int, int>> vSegments; 
    if (g.r + g.h <= 4) vSegments.push_back({g.r, g.h});
    else { vSegments.push_back({g.r, 4 - g.r}); vSegments.push_back({0, g.h - (4 - g.r)}); }

    for (auto& hSeg : hSegments) {
        for (auto& vSeg : vSegments) {
            Rectangle rect = { (float)startX + hSeg.first * cellSize + 5, (float)startY + vSeg.first * cellSize + 5, (float)hSeg.second * cellSize - 10, (float)vSeg.second * cellSize - 10 };
            DrawRectangleRoundedLines(rect, 0.2f, 6, Fade(g.color, alpha + 0.2f));
            DrawRectangleRounded(rect, 0.2f, 6, Fade(g.color, 0.1f));
        }
    }
    Rectangle mainRect = { (float)startX + g.c * cellSize + 5, (float)startY + g.r * cellSize + 5, 40, 30 };
    std::string term = GetTerm(g, isPOS);
    DrawText(term.c_str(), (int)mainRect.x + 5, (int)mainRect.y + 5, 20, g.color);
}

void DrawNeonCell(int r, int c, int startX, int startY, int cellSize, int value, Font font, bool isHovered, bool showIndex, bool isPOS) {
    int x = startX + c * cellSize;
    int y = startY + r * cellSize;
    
    bool isTarget = (isPOS) ? (value == VAL_0) : (value == VAL_1);
    bool isX = (value == VAL_X);

    Color baseColor;
    if (isTarget) baseColor = {0, 255, 100, 255}; 
    else if (isX) baseColor = {255, 100, 50, 255}; 
    else baseColor = {80, 80, 80, 255}; 

    if (isHovered && !isTarget && !isX) baseColor = LIGHTGRAY;

    if (isTarget) { 
        DrawRectangleLinesEx(Rectangle{(float)x-4, (float)y-4, (float)cellSize+8, (float)cellSize+8}, 4, Fade(baseColor, 0.1f));
        DrawRectangleLinesEx(Rectangle{(float)x-2, (float)y-2, (float)cellSize+4, (float)cellSize+4}, 3, Fade(baseColor, 0.3f));
    }
    if (isX) {
        DrawRectangleLinesEx(Rectangle{(float)x-2, (float)y-2, (float)cellSize+4, (float)cellSize+4}, 2, Fade(baseColor, 0.2f));
    }
    DrawRectangleLinesEx(Rectangle{(float)x, (float)y, (float)cellSize, (float)cellSize}, 2, baseColor);

    const char* text;
    if (showIndex) {
        int index = GRAY_CODES[r] * 4 + GRAY_CODES[c];
        text = TextFormat("%d", index);
    } else {
        if (value == VAL_1) text = "1";
        else if (value == VAL_0) text = "0";
        else text = "X";
    }

    float fontSize = 40.0f;
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 0);
    Vector2 textPos = { x + cellSize/2 - textSize.x/2, y + cellSize/2 - textSize.y/2 };

    if (isTarget) {
        DrawTextEx(font, text, textPos, fontSize, 0, WHITE);
        DrawTextEx(font, text, textPos, fontSize, 0, Fade(baseColor, 0.6f));
    } else if (isX) {
        DrawTextEx(font, text, textPos, fontSize, 0, baseColor);
    } else {
        Color inactiveTextColor = showIndex ? Color{100, 100, 100, 255} : Fade(baseColor, 0.5f);
        DrawTextEx(font, text, textPos, fontSize, 0, inactiveTextColor);
    }
}

void DrawFormulaLine(Font font, std::string text, float x, float y, float fontSize, bool useBar) {
    if (!useBar) {
        DrawTextEx(font, text.c_str(), {x, y}, fontSize, 1.0f, WHITE);
        return;
    }
    float currentX = x;
    float charWidth = MeasureTextEx(font, "A", fontSize, 1.0f).x;
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        bool hasBar = (i + 1 < text.length() && text[i+1] == '\'');
        char tempStr[2] = {c, '\0'};
        DrawTextEx(font, tempStr, {currentX, y}, fontSize, 1.0f, WHITE);
        if (hasBar) {
            float barThickness = std::max(2.0f, fontSize / 15.0f);
            float barY = y - (fontSize * 0.1f); 
            DrawRectangle((int)currentX, (int)barY, (int)charWidth, (int)barThickness, WHITE);
            i++; 
        }
        currentX += charWidth;
    }
}

void DrawFormulaSmart(Font font, std::string formula, float x, float y, float widthLimit, bool useBar) {
    std::string measureStr = formula;
    if (useBar) {
        measureStr = "";
        for(size_t i=0; i<formula.length(); i++) if (formula[i] != '\'') measureStr += formula[i];
    }
    float fontSize = 45.0f; 
    Vector2 size = MeasureTextEx(font, measureStr.c_str(), fontSize, 1.0f);
    int splitPos = -1;
    bool needsSplit = false;
    if (size.x > widthLimit) {
        float scale = widthLimit / size.x;
        if (scale < 0.7f) needsSplit = true;
        else fontSize *= scale;
    }
    if (!needsSplit) {
        DrawFormulaLine(font, formula, x, y + 25, fontSize, useBar);
        return;
    }
    int mid = formula.length() / 2;
    int rightPlus = formula.find(" + ", mid);
    int leftPlus = formula.rfind(" + ", mid);
    if (rightPlus == -1 && leftPlus == -1) {
         rightPlus = formula.find(")(", mid);
         leftPlus = formula.rfind(")(", mid);
         if (rightPlus != -1) rightPlus += 1; 
         if (leftPlus != -1) leftPlus += 1;
    }
    if (rightPlus == -1) splitPos = leftPlus;
    else if (leftPlus == -1) splitPos = rightPlus;
    else splitPos = (abs(mid - rightPlus) < abs(mid - leftPlus)) ? rightPlus : leftPlus;
    if (splitPos != -1) {
        std::string line1 = formula.substr(0, splitPos + (formula[splitPos] == '+' ? 3 : 0)); 
        std::string line2 = "      " + formula.substr(splitPos + (formula[splitPos] == '+' ? 3 : 0)); 
        float lineFontSize = 35.0f; 
        std::string measureLine2 = line2;
        if (useBar) {
            measureLine2 = "";
            for(char c : line2) if (c != '\'') measureLine2 += c;
        }
        Vector2 size2 = MeasureTextEx(font, measureLine2.c_str(), lineFontSize, 1.0f);
        if (size2.x > widthLimit) lineFontSize *= (widthLimit / size2.x);
        DrawFormulaLine(font, line1, x, y + 10, lineFontSize, useBar);
        DrawFormulaLine(font, line2, x, y + 10 + lineFontSize + 5, lineFontSize, useBar);
    } else {
        DrawFormulaLine(font, formula, x, y + 25, fontSize, useBar);
    }
}

// --- History Helpers ---
void SaveHistory(std::vector<GridState>& history, int data[4][4]) {
    GridState state;
    memcpy(state.data, data, sizeof(int) * 16);
    history.push_back(state);
    // Limit history size if needed (e.g., 50 steps)
    if (history.size() > 50) history.erase(history.begin());
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1000, 800, "K-Map Solver");
    
    Image icon = LoadImageFromMemory(".png", icon_data, icon_data_len);
    if (icon.width > 0) { // 確保圖片有載入成功
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        ImageResize(&icon, 64, 64); 
        SetWindowIcon(icon);
        UnloadImage(icon);
    }

    SetTargetFPS(60);

    Font techFont = LoadFontFromMemory(".ttf", consola_ttf_data, consola_ttf_data_len, 64, 0, 0);
    SetTextureFilter(techFont.texture, TEXTURE_FILTER_BILINEAR);

    int data[4][4] = {0}; 
    std::vector<KMapGroup> groups; 
    
    // 歷史紀錄堆疊
    std::vector<GridState> history;

    int startX = 250, startY = 200, cellSize = 100;
    
    bool showIndexMode = false;
    bool showBarMode = false;
    bool isPOSMode = false;

    float copyFeedbackTimer = 0.0f;
    float undoFeedbackTimer = 0.0f; // 顯示 Undo 提示
    
    bool isDragging = false;
    int dragStartR = -1;
    int dragStartC = -1;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();
        if (copyFeedbackTimer > 0) copyFeedbackTimer -= GetFrameTime();
        if (undoFeedbackTimer > 0) undoFeedbackTimer -= GetFrameTime();
        
        bool triggerClear = false;
        bool triggerCopy = false;
        bool triggerUndo = false;
        bool saveStateNeeded = false; // 是否需要存檔

        bool ctrlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

        // --- 鍵盤輸入邏輯區分 ---
        
        // C vs Ctrl+C
        if (IsKeyPressed(KEY_C)) {
            if (ctrlDown) triggerCopy = true;
            else triggerClear = true;
        }

        // Z vs Ctrl+Z (Both trigger undo for convenience, but following Ctrl+Z standard)
        if (IsKeyPressed(KEY_Z) && ctrlDown) triggerUndo = true;

        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_V)) showIndexMode = !showIndexMode;

        bool needSolve = false;

        int hoverR = -1, hoverC = -1;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                Rectangle cellRect = { (float)startX + c*cellSize, (float)startY + r*cellSize, (float)cellSize, (float)cellSize };
                if (CheckCollisionPointRec(mousePos, cellRect)) {
                    hoverR = r; hoverC = c;
                }
            }
        }

        bool isShift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        bool isXKey = IsKeyDown(KEY_X);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mousePos, { 800, 100, 150, 40 })) showIndexMode = !showIndexMode;
            else if (CheckCollisionPointRec(mousePos, { 800, 160, 150, 40 })) triggerClear = true;
            else if (CheckCollisionPointRec(mousePos, { 800, 220, 150, 40 })) showBarMode = !showBarMode;
            else if (CheckCollisionPointRec(mousePos, { 800, 280, 150, 40 })) triggerCopy = true;
            else if (CheckCollisionPointRec(mousePos, { 800, 340, 150, 40 })) {
                // 存檔 (Mode Switch)
                SaveHistory(history, data);
                
                isPOSMode = !isPOSMode;
                for(int r=0; r<4; r++) for(int c=0; c<4; c++) {
                    if (data[r][c] == VAL_0) data[r][c] = VAL_1;
                    else if (data[r][c] == VAL_1) data[r][c] = VAL_0;
                }
                needSolve = true;
            }
            else if (hoverR != -1) {
                // 開始拖曳/點擊前，存檔
                SaveHistory(history, data);
                dragStartR = hoverR;
                dragStartC = hoverC;
                isDragging = false; 
            }
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && dragStartR != -1) {
            if (hoverR != -1 && (hoverR != dragStartR || hoverC != dragStartC)) {
                isDragging = true;
            }

            if (isDragging && hoverR != -1) {
                int paintVal = VAL_1;
                if (isShift) paintVal = VAL_0;
                if (isXKey) paintVal = VAL_X;

                if (data[hoverR][hoverC] != paintVal) {
                    data[hoverR][hoverC] = paintVal;
                    needSolve = true;
                }
                if (data[dragStartR][dragStartC] != paintVal) {
                    data[dragStartR][dragStartC] = paintVal;
                    needSolve = true;
                }
            }
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            if (dragStartR != -1 && !isDragging) {
                int r = dragStartR;
                int c = dragStartC;
                
                if (isXKey) data[r][c] = (data[r][c] == VAL_X) ? VAL_0 : VAL_X;
                else if (isShift) data[r][c] = VAL_0;
                else data[r][c] = (data[r][c] == VAL_1) ? VAL_0 : VAL_1;
                needSolve = true;
            }
            dragStartR = -1;
            dragStartC = -1;
            isDragging = false;
        }

        // --- Execute Actions ---

        if (triggerCopy) {
            std::string f = GenerateFormula(groups, isPOSMode);
            SetClipboardText(f.c_str());
            copyFeedbackTimer = 1.5f;
        }

        if (triggerClear) {
            SaveHistory(history, data); // 存檔 (Clear)
            int fillValue = isPOSMode ? VAL_1 : VAL_0;
            for(int r=0; r<4; r++) for(int c=0; c<4; c++) data[r][c] = fillValue;
            needSolve = true;
        }

        if (triggerUndo) {
            if (!history.empty()) {
                GridState lastState = history.back();
                history.pop_back();
                memcpy(data, lastState.data, sizeof(int) * 16);
                undoFeedbackTimer = 1.0f;
                needSolve = true;
            }
        }

        if (needSolve) groups = SolveKMap(data, isPOSMode ? VAL_0 : VAL_1);

        // --- Drawing ---
        BeginDrawing();
            ClearBackground(Color{20, 20, 20, 255});

            for(int i=0; i<4; i++) {
                DrawTextEx(techFont, COL_LABELS[i], {(float)startX + i*cellSize + 30, (float)startY - 40}, 30, 0, SKYBLUE);
                DrawTextEx(techFont, ROW_LABELS[i], {(float)startX - 50, (float)startY + i*cellSize + 35}, 30, 0, ORANGE);
            }
            DrawTextEx(techFont, "AB \\ CD", {(float)startX - 120, (float)startY - 40}, 30, 0, YELLOW);

            if (!groups.empty()) {
                DrawTextEx(techFont, TextFormat("Groups: %d", (int)groups.size()), {800, 50}, 35, 0, YELLOW);
            }

            Color btnModeColor = showIndexMode ? SKYBLUE : DARKGRAY;
            DrawRectangleRounded({ 800, 100, 150, 40 }, 0.3f, 4, Fade(btnModeColor, 0.3f));
            DrawRectangleRoundedLines({ 800, 100, 150, 40 }, 0.3f, 4, btnModeColor);
            DrawTextEx(techFont, showIndexMode ? "Mode: Index" : "Mode: Value", {810, 108}, 20, 0, WHITE);
            DrawTextEx(techFont, "[V] / [TAB]", {835, 145}, 15, 0, GRAY);

            Color btnClearColor = RED;
            DrawRectangleRounded({ 800, 160, 150, 40 }, 0.3f, 4, Fade(btnClearColor, 0.3f));
            DrawRectangleRoundedLines({ 800, 160, 150, 40 }, 0.3f, 4, btnClearColor);
            DrawTextEx(techFont, "Clear [C]", {830, 168}, 20, 0, WHITE);

            Color btnStyleColor = PURPLE;
            DrawRectangleRounded({ 800, 220, 150, 40 }, 0.3f, 4, Fade(btnStyleColor, 0.3f));
            DrawRectangleRoundedLines({ 800, 220, 150, 40 }, 0.3f, 4, btnStyleColor);
            DrawTextEx(techFont, showBarMode ? "Style: Bar" : "Style: Text", {825, 228}, 20, 0, WHITE);

            Color btnCopyColor = GREEN;
            DrawRectangleRounded({ 800, 280, 150, 40 }, 0.3f, 4, Fade(btnCopyColor, 0.3f));
            DrawRectangleRoundedLines({ 800, 280, 150, 40 }, 0.3f, 4, btnCopyColor);
            if (copyFeedbackTimer > 0) DrawTextEx(techFont, "Copied!", {835, 288}, 20, 0, WHITE);
            else DrawTextEx(techFont, "Copy[Ctrl+C]", {815, 288}, 20, 0, WHITE);

            Color btnFmtColor = ORANGE;
            DrawRectangleRounded({ 800, 340, 150, 40 }, 0.3f, 4, Fade(btnFmtColor, 0.3f));
            DrawRectangleRoundedLines({ 800, 340, 150, 40 }, 0.3f, 4, btnFmtColor);
            DrawTextEx(techFont, isPOSMode ? "Format: POS" : "Format: SOP", {820, 348}, 20, 0, WHITE);
            
            // Undo Tip / Feedback
            if (undoFeedbackTimer > 0) {
                DrawTextEx(techFont, "Undo!", {850, 400}, 20, 0, SKYBLUE);
            } else {
                DrawTextEx(techFont, "Undo: [Ctrl+Z]", {810, 400}, 15, 0, GRAY);
            }
            DrawTextEx(techFont, "Drag: 1 / [Shift]: 0", {805, 430}, 15, 0, GRAY);
            DrawTextEx(techFont, "[X]+Drag: X", {805, 450}, 15, 0, GRAY);

            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    int x = startX + c * cellSize;
                    int y = startY + r * cellSize;
                    bool isHovered = (dragStartR == -1) && CheckCollisionPointRec(mousePos, Rectangle{(float)x, (float)y, (float)cellSize, (float)cellSize});
                    DrawNeonCell(r, c, startX, startY, cellSize, data[r][c], techFont, isHovered, showIndexMode, isPOSMode);
                }
            }

            float alpha = (sinf(GetTime() * 3.0f) + 1.0f) / 2.0f * 0.4f + 0.3f; 
            for (auto& g : groups) {
                DrawWrappedGroup(g, startX, startY, cellSize, alpha, isPOSMode);
            }
            
            DrawRectangle(0, 700, 1000, 100, Fade(BLACK, 0.9f));
            std::string formula = GenerateFormula(groups, isPOSMode);
            DrawFormulaSmart(techFont, formula, 30, 700, 940.0f, showBarMode);

        EndDrawing();
    }

    UnloadFont(techFont);
    CloseWindow();
    return 0;
}