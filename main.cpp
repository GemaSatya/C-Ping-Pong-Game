#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <sstream>
#include <cmath>

using namespace Gdiplus;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_CLASS_NAME = "GameWindow";
const char* WINDOW_TITLE = "Ping Pong - Classic Arcade Revival";
const wchar_t* BACKGROUND_IMAGE = L"assets/background-menu.png";

ULONG_PTR gdiplusToken;
Image* backgroundImage = nullptr;

// Game states
enum GameState {
    MENU,
    DIFFICULTY_SELECT,
    PLAYING,
    PAUSED
};
GameState gameState = MENU;
int selectedDifficulty = -1; // -1: none, 0: easy, 1: medium, 2: hard

// Pause menu variables
int pauseMenuSelection = 0; // 0: resume, 1: exit
float countdownTimer = 0.0f; // 3 second countdown before resuming
bool isCountingDown = false;
float pauseAnimTime = 0.0f;

// Game variables
const int PADDLE_WIDTH = 10;
const int PADDLE_HEIGHT = 100;
const int PADDLE_SPEED = 8;
int currentPaddleSpeed = PADDLE_SPEED;
float currentSpeedFactor = 1.25f;

float leftPaddleY = (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2.0f;
float rightPaddleY = (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2.0f;

// Ball variables
const int BALL_RADIUS = 6;
const float SPEED_INCREASE_FACTOR = 1.25f;
const int MAX_HITS_FOR_SPEED_INCREASE = 6;
int hitCount = 0;
float ballX = WINDOW_WIDTH / 2.0f;
float ballY = WINDOW_HEIGHT / 2.0f;
float ballVelocityX = -5.0f;
float ballVelocityY = 3.0f;

// Score variables
int leftScore = 0;
int rightScore = 0;

// Key state tracking
bool wKeyPressed = false;
bool sKeyPressed = false;
bool upKeyPressed = false;
bool downKeyPressed = false;

// Animation variables
float menuAnimTime = 0.0f;
float selectionAnimTime = 0.0f;

// Helper function to convert int to wstring
std::wstring IntToWString(int value) {
    std::wstringstream ss;
    ss << value;
    return ss.str();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wparam == 'W' || wparam == 'w') {
                wKeyPressed = true;
            } else if (wparam == 'S' || wparam == 's') {
                sKeyPressed = true;
            } else if (wparam == VK_UP) {
                upKeyPressed = true;
            } else if (wparam == VK_DOWN) {
                downKeyPressed = true;
            } else if (wparam == VK_ESCAPE) {
                PostQuitMessage(0);
            } else if (wparam == VK_LEFT && gameState == DIFFICULTY_SELECT) {
                if (selectedDifficulty > 0) {
                    selectedDifficulty--;
                    selectionAnimTime = 0.0f;
                }
            } else if (wparam == VK_RIGHT && gameState == DIFFICULTY_SELECT) {
                if (selectedDifficulty < 2) {
                    selectedDifficulty++;
                    selectionAnimTime = 0.0f;
                }
            } else if (wparam == 'P' || wparam == 'p') {
                if (gameState == PLAYING) {
                    gameState = PAUSED;
                    pauseMenuSelection = 0; // Default to resume
                    isCountingDown = false;
                    countdownTimer = 0.0f;
                    pauseAnimTime = 0.0f;
                }
            } else if (wparam == VK_LEFT && gameState == PAUSED && !isCountingDown) {
                if (pauseMenuSelection > 0) {
                    pauseMenuSelection--;
                }
            } else if (wparam == VK_RIGHT && gameState == PAUSED && !isCountingDown) {
                if (pauseMenuSelection < 1) {
                    pauseMenuSelection++;
                }
            } else if (wparam == VK_RETURN && gameState == PAUSED && !isCountingDown) {
                if (pauseMenuSelection == 0) { // Resume
                    isCountingDown = true;
                    countdownTimer = 2.0f;
                } else if (pauseMenuSelection == 1) { // Exit to menu
                    gameState = MENU;
                    selectedDifficulty = -1;
                    leftScore = 0;
                    rightScore = 0;
                    hitCount = 0;
                    ballX = WINDOW_WIDTH / 2.0f;
                    ballY = WINDOW_HEIGHT / 2.0f;
                    ballVelocityX = -5.0f;
                    ballVelocityY = 3.0f;
                }
            } else if (wparam == VK_RETURN && gameState == DIFFICULTY_SELECT) {
                // Start game with selected difficulty
                gameState = PLAYING;
                // Reset game state
                leftPaddleY = (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2.0f;
                rightPaddleY = (WINDOW_HEIGHT - PADDLE_HEIGHT) / 2.0f;
                ballX = WINDOW_WIDTH / 2.0f;
                ballY = WINDOW_HEIGHT / 2.0f;
                ballVelocityX = -5.0f;
                ballVelocityY = 3.0f;
                hitCount = 0;
                leftScore = 0;
                rightScore = 0;
                
                // Set difficulty parameters
                if (selectedDifficulty == 0) { // Easy
                    currentSpeedFactor = 1.25f;
                    currentPaddleSpeed = PADDLE_SPEED;
                } else if (selectedDifficulty == 1) { // Medium
                    currentSpeedFactor = 1.45f;
                    currentPaddleSpeed = PADDLE_SPEED + 3;
                } else if (selectedDifficulty == 2) { // Hard
                    currentSpeedFactor = 1.70f;
                    currentPaddleSpeed = PADDLE_SPEED + 6;
                }
            }
            
            if (gameState == MENU) {
                gameState = DIFFICULTY_SELECT;
                selectedDifficulty = 0; // Default to easy
                selectionAnimTime = 0.0f;
            }
            return 0;
        case WM_KEYUP:
            if (wparam == 'W' || wparam == 'w') {
                wKeyPressed = false;
            } else if (wparam == 'S' || wparam == 's') {
                sKeyPressed = false;
            } else if (wparam == VK_UP) {
                upKeyPressed = false;
            } else if (wparam == VK_DOWN) {
                downKeyPressed = false;
            }
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Create memory DC for double buffering (prevents flickering)
            HDC memDC = CreateCompatibleDC(hdc);
            RECT rect;
            GetClientRect(hwnd, &rect);
            int clientWidth = rect.right - rect.left;
            int clientHeight = rect.bottom - rect.top;
            
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientWidth, clientHeight);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
            
            Graphics graphics(memDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);

            // Create shared font objects
            FontFamily fontFamily(L"Arial");
            StringFormat stringFormat;
            stringFormat.SetAlignment(StringAlignmentCenter);
            stringFormat.SetLineAlignment(StringAlignmentCenter);

            if (gameState == MENU) {
                // Update animation time
                menuAnimTime += 0.03f;
                
                // Draw background image
                if (backgroundImage) {
                    graphics.DrawImage(backgroundImage, 0, 0, clientWidth, clientHeight);
                } else {
                    // Fallback to animated gradient background
                    float colorShift = sin(menuAnimTime * 0.5f) * 20;
                    LinearGradientBrush gradientBrush(
                        Point(0, 0),
                        Point(0, clientHeight),
                        Color(255, (int)(15 + colorShift), (int)(10 + colorShift), (int)(40 + colorShift)),
                        Color(255, (int)(40 + colorShift), (int)(10 + colorShift), (int)(60 + colorShift))
                    );
                    graphics.FillRectangle(&gradientBrush, 0, 0, clientWidth, clientHeight);
                }

                // Draw animated background particles
                SolidBrush particleBrush(Color(60, 255, 255, 255));
                for (int i = 0; i < 30; i++) {
                    float angle = menuAnimTime * 0.3f + (i * 3.14159f * 2.0f / 30.0f);
                    float radius = 200 + sin(menuAnimTime * 0.5f + i) * 50;
                    float x = clientWidth / 2 + cos(angle) * radius;
                    float y = clientHeight / 2 + sin(angle) * radius;
                    int size = 2 + (int)(sin(menuAnimTime + i) * 2);
                    graphics.FillEllipse(&particleBrush, (int)x - size, (int)y - size, size * 2, size * 2);
                }

                // Draw decorative corner elements
                Pen decorPen(Color(200, 100, 200, 255), 4);
                int cornerSize = 60;
                int cornerMargin = 40;
                
                // Animated corner brackets with glow
                for (int offset = 0; offset < 3; offset++) {
                    int alpha = 100 - offset * 30;
                    Pen glowPen(Color(alpha, 100, 200, 255), 4 - offset);
                    
                    // Top-left
                    graphics.DrawLine(&glowPen, cornerMargin - offset, cornerMargin - offset, 
                                     cornerMargin + cornerSize + offset, cornerMargin - offset);
                    graphics.DrawLine(&glowPen, cornerMargin - offset, cornerMargin - offset, 
                                     cornerMargin - offset, cornerMargin + cornerSize + offset);
                    
                    // Top-right
                    graphics.DrawLine(&glowPen, clientWidth - cornerMargin + offset, cornerMargin - offset, 
                                     clientWidth - cornerMargin - cornerSize - offset, cornerMargin - offset);
                    graphics.DrawLine(&glowPen, clientWidth - cornerMargin + offset, cornerMargin - offset, 
                                     clientWidth - cornerMargin + offset, cornerMargin + cornerSize + offset);
                    
                    // Bottom-left
                    graphics.DrawLine(&glowPen, cornerMargin - offset, clientHeight - cornerMargin + offset, 
                                     cornerMargin + cornerSize + offset, clientHeight - cornerMargin + offset);
                    graphics.DrawLine(&glowPen, cornerMargin - offset, clientHeight - cornerMargin + offset, 
                                     cornerMargin - offset, clientHeight - cornerMargin - cornerSize - offset);
                    
                    // Bottom-right
                    graphics.DrawLine(&glowPen, clientWidth - cornerMargin + offset, clientHeight - cornerMargin + offset, 
                                     clientWidth - cornerMargin - cornerSize - offset, clientHeight - cornerMargin + offset);
                    graphics.DrawLine(&glowPen, clientWidth - cornerMargin + offset, clientHeight - cornerMargin + offset, 
                                     clientWidth - cornerMargin + offset, clientHeight - cornerMargin - cornerSize - offset);
                }

                // Draw game title with glow effect
                Font titleFont(&fontFamily, 96, FontStyleBold, UnitPixel);

                // Title glow layers
                for (int i = 3; i > 0; i--) {
                    int alpha = 60 - i * 15;
                    SolidBrush glowBrush(Color(alpha, 100, 200, 255));
                    RectF glowRect(0, clientHeight / 2 - 150 - i * 2, clientWidth, 120);
                    graphics.DrawString(L"PONG", -1, &titleFont, glowRect, &stringFormat, &glowBrush);
                }

                // Main title with gradient
                RectF titleRect(0, clientHeight / 2 - 150, clientWidth, 120);
                LinearGradientBrush titleGradient(
                    Point(clientWidth / 2, (int)(clientHeight / 2 - 150)),
                    Point(clientWidth / 2, (int)(clientHeight / 2 - 30)),
                    Color(255, 255, 255, 255),
                    Color(255, 100, 200, 255)
                );
                graphics.DrawString(L"PONG", -1, &titleFont, titleRect, &stringFormat, &titleGradient);

                // Draw subtitle with pulse effect
                Font subtitleFont(&fontFamily, 28, FontStyleRegular, UnitPixel);
                int subtitleAlpha = (int)(180 + sin(menuAnimTime * 2.0f) * 75);
                SolidBrush subtitleBrush(Color(subtitleAlpha, 200, 200, 200));
                RectF subtitleRect(0, clientHeight / 2 - 20, clientWidth, 50);
                graphics.DrawString(L"Classic Arcade Experience", -1, &subtitleFont, subtitleRect, &stringFormat, &subtitleBrush);

                // Draw animated "Press Any Key" text with bounce effect
                Font promptFont(&fontFamily, 36, FontStyleBold, UnitPixel);
                float bounce = sin(menuAnimTime * 3.0f) * 10;
                int promptAlpha = (int)(200 + sin(menuAnimTime * 4.0f) * 55);
                
                // Glow effect for prompt
                SolidBrush promptGlowBrush(Color(promptAlpha / 2, 255, 255, 100));
                RectF promptGlowRect(0, clientHeight / 2 + 80 + bounce - 2, clientWidth, 60);
                graphics.DrawString(L"Press Any Key to Start", -1, &promptFont, promptGlowRect, &stringFormat, &promptGlowBrush);
                
                // Main prompt text
                SolidBrush promptBrush(Color(promptAlpha, 255, 255, 255));
                RectF promptRect(0, clientHeight / 2 + 80 + bounce, clientWidth, 60);
                graphics.DrawString(L"Press Any Key to Start", -1, &promptFont, promptRect, &stringFormat, &promptBrush);

                // Draw decorative lines
                Pen linePen(Color(150, 100, 200, 255), 2);
                int lineY = clientHeight / 2 + 160;
                for (int i = 0; i < 5; i++) {
                    int lineWidth = 50 + i * 30;
                    int lineX = clientWidth / 2 - lineWidth / 2;
                    int alpha = 150 - i * 20;
                    Pen currentLinePen(Color(alpha, 100, 200, 255), 2);
                    graphics.DrawLine(&currentLinePen, lineX, lineY + i * 8, lineX + lineWidth, lineY + i * 8);
                }

                // Draw version/credits at bottom
                Font creditFont(&fontFamily, 16, FontStyleRegular, UnitPixel);
                SolidBrush creditBrush(Color(120, 150, 150, 150));
                RectF creditRect(0, clientHeight - 50, clientWidth, 30);
                graphics.DrawString(L"© 2024 Classic Games Revival", -1, &creditFont, creditRect, &stringFormat, &creditBrush);

            } else if (gameState == DIFFICULTY_SELECT) {
                // Draw background image for difficulty selection
                if (backgroundImage) {
                    graphics.DrawImage(backgroundImage, 0, 0, clientWidth, clientHeight);
                } else {
                    // Fallback to gradient background
                    LinearGradientBrush gradientBrush(
                        Point(0, 0),
                        Point(0, clientHeight),
                        Color(255, 10, 10, 30),
                        Color(255, 30, 10, 50)
                    );
                    graphics.FillRectangle(&gradientBrush, 0, 0, clientWidth, clientHeight);
                }

                // Update animation time
                selectionAnimTime += 0.05f;

                // Draw decorative elements - animated corner brackets
                Pen decorPen(Color(255, 100, 200, 255), 3);
                int bracketSize = 40;
                int margin = 30;
                
                // Top-left bracket
                graphics.DrawLine(&decorPen, margin, margin, margin + bracketSize, margin);
                graphics.DrawLine(&decorPen, margin, margin, margin, margin + bracketSize);
                
                // Top-right bracket
                graphics.DrawLine(&decorPen, clientWidth - margin, margin, clientWidth - margin - bracketSize, margin);
                graphics.DrawLine(&decorPen, clientWidth - margin, margin, clientWidth - margin, margin + bracketSize);
                
                // Bottom-left bracket
                graphics.DrawLine(&decorPen, margin, clientHeight - margin, margin + bracketSize, clientHeight - margin);
                graphics.DrawLine(&decorPen, margin, clientHeight - margin, margin, clientHeight - margin - bracketSize);
                
                // Bottom-right bracket
                graphics.DrawLine(&decorPen, clientWidth - margin, clientHeight - margin, clientWidth - margin - bracketSize, clientHeight - margin);
                graphics.DrawLine(&decorPen, clientWidth - margin, clientHeight - margin, clientWidth - margin, clientHeight - margin - bracketSize);

                // Draw animated particles/dots around the screen
                SolidBrush particleBrush(Color(100, 255, 255, 255));
                for (int i = 0; i < 15; i++) {
                    float angle = selectionAnimTime + (i * 3.14159f * 2.0f / 15.0f);
                    float x = clientWidth / 2 + cos(angle) * 350;
                    float y = clientHeight / 2 + sin(angle) * 250;
                    graphics.FillEllipse(&particleBrush, (int)x - 3, (int)y - 3, 6, 6);
                }

                // Draw difficulty selection menu with enhanced styling
                Font titleFont(&fontFamily, 64, FontStyleBold, UnitPixel);
                Font optionFont(&fontFamily, 44, FontStyleBold, UnitPixel);
                Font descFont(&fontFamily, 18, FontStyleRegular, UnitPixel);
                SolidBrush titleBrush(Color(255, 255, 255, 255)); // White text

                // Draw glowing title with shadow
                SolidBrush shadowBrush(Color(150, 0, 0, 0));
                RectF titleShadowRect(3, 73, clientWidth, 80);
                graphics.DrawString(L"SELECT DIFFICULTY", -1, &titleFont, titleShadowRect, &stringFormat, &shadowBrush);
                
                RectF titleRect(0, 70, clientWidth, 80);
                LinearGradientBrush titleGradient(
                    Point(clientWidth / 2, 70),
                    Point(clientWidth / 2, 150),
                    Color(255, 255, 200, 100),
                    Color(255, 255, 255, 255)
                );
                graphics.DrawString(L"SELECT DIFFICULTY", -1, &titleFont, titleRect, &stringFormat, &titleGradient);

                // Draw decorative line under title
                Pen linePen(Color(255, 100, 200, 255), 2);
                graphics.DrawLine(&linePen, clientWidth / 2 - 200, 170, clientWidth / 2 + 200, 170);

                // Draw difficulty options with cards
                int cardWidth = 280;
                int cardHeight = 220;
                int optionY = 280;
                int totalWidth = cardWidth * 3 + 100; // 3 cards with 50px spacing
                int startX = (clientWidth - totalWidth) / 2;

                const wchar_t* difficultyNames[] = {L"EASY", L"MEDIUM", L"HARD"};
                const wchar_t* difficultyDescs[] = {
                    L"Relaxed pace\nPerfect for beginners",
                    L"Balanced challenge\nFor experienced players",
                    L"Lightning fast\nUltimate test of skill"
                };
                Color cardColors[] = {
                    Color(255, 50, 200, 100),   // Cyan for Easy
                    Color(255, 255, 200, 50),   // Yellow for Medium
                    Color(255, 255, 50, 50)     // Red for Hard
                };

                for (int i = 0; i < 3; i++) {
                    int cardX = startX + i * (cardWidth + 50);
                    
                    // Calculate pulse effect for selected card
                    float pulse = 0.0f;
                    float scale = 1.0f;
                    if (i == selectedDifficulty) {
                        pulse = sin(selectionAnimTime * 5.0f) * 0.15f + 0.85f;
                        scale = 1.05f + sin(selectionAnimTime * 3.0f) * 0.02f;
                    } else {
                        pulse = 0.5f;
                        scale = 0.95f;
                    }

                    // Draw card background with glow effect
                    if (i == selectedDifficulty) {
                        // Outer glow
                        SolidBrush glowBrush(Color((int)(100 * pulse), cardColors[i].GetR(), cardColors[i].GetG(), cardColors[i].GetB()));
                        graphics.FillRectangle(&glowBrush, 
                            cardX - 10, 
                            optionY - 10, 
                            cardWidth + 20, 
                            cardHeight + 20);
                    }

                    // Card background
                    SolidBrush cardBrush(Color((int)(150 * pulse), 20, 20, 40));
                    graphics.FillRectangle(&cardBrush, cardX, optionY, cardWidth, cardHeight);

                    // Card border
                    Pen borderPen(Color((int)(255 * pulse), cardColors[i].GetR(), cardColors[i].GetG(), cardColors[i].GetB()), 
                                  i == selectedDifficulty ? 4 : 2);
                    graphics.DrawRectangle(&borderPen, cardX, optionY, cardWidth, cardHeight);

                    // Draw difficulty icon/symbol
                    SolidBrush iconBrush(Color((int)(200 * pulse), cardColors[i].GetR(), cardColors[i].GetG(), cardColors[i].GetB()));
                    int iconY = optionY + 30;
                    
                    if (i == 0) { // Easy - single bar
                        graphics.FillRectangle(&iconBrush, cardX + cardWidth / 2 - 10, iconY, 20, 40);
                    } else if (i == 1) { // Medium - two bars
                        graphics.FillRectangle(&iconBrush, cardX + cardWidth / 2 - 25, iconY + 10, 20, 40);
                        graphics.FillRectangle(&iconBrush, cardX + cardWidth / 2 + 5, iconY, 20, 50);
                    } else { // Hard - three bars
                        graphics.FillRectangle(&iconBrush, cardX + cardWidth / 2 - 35, iconY + 20, 20, 30);
                        graphics.FillRectangle(&iconBrush, cardX + cardWidth / 2 - 10, iconY + 10, 20, 40);
                        graphics.FillRectangle(&iconBrush, cardX + cardWidth / 2 + 15, iconY, 20, 50);
                    }

                    // Draw difficulty name
                    Font cardTitleFont(&fontFamily, 36, FontStyleBold, UnitPixel);
                    SolidBrush textBrush(Color((int)(255 * pulse), 255, 255, 255));
                    RectF nameRect(cardX, optionY + 90, cardWidth, 50);
                    graphics.DrawString(difficultyNames[i], -1, &cardTitleFont, nameRect, &stringFormat, &textBrush);

                    // Draw difficulty description
                    SolidBrush descBrush(Color((int)(200 * pulse), 200, 200, 200));
                    RectF descRect(cardX + 10, optionY + 145, cardWidth - 20, 60);
                    graphics.DrawString(difficultyDescs[i], -1, &descFont, descRect, &stringFormat, &descBrush);

                    // Draw selection arrow above selected card
                    if (i == selectedDifficulty) {
                        SolidBrush arrowBrush(Color(255, 255, 255, 100));
                        Point arrowPoints[3];
                        int arrowX = cardX + cardWidth / 2;
                        int arrowY = optionY - 30;
                        arrowPoints[0] = Point(arrowX, arrowY + (int)(sin(selectionAnimTime * 4.0f) * 5.0f));
                        arrowPoints[1] = Point(arrowX - 15, arrowY - 20 + (int)(sin(selectionAnimTime * 4.0f) * 5.0f));
                        arrowPoints[2] = Point(arrowX + 15, arrowY - 20 + (int)(sin(selectionAnimTime * 4.0f) * 5.0f));
                        graphics.FillPolygon(&arrowBrush, arrowPoints, 3);
                    }
                }

                // Draw instructions at bottom with icon hints
                Font instructionFont(&fontFamily, 22, FontStyleBold, UnitPixel);
                SolidBrush instructionBrush(Color(255, 200, 200, 200));
                
                // Draw keyboard icons
                SolidBrush keyBrush(Color(255, 60, 60, 80));
                Pen keyPen(Color(255, 150, 150, 150), 2);
                int keySize = 35;
                int keyY = clientHeight - 100;
                
                // Left arrow key
                graphics.FillRectangle(&keyBrush, clientWidth / 2 - 150, keyY, keySize, keySize);
                graphics.DrawRectangle(&keyPen, clientWidth / 2 - 150, keyY, keySize, keySize);
                RectF leftArrowRect(clientWidth / 2 - 150, keyY, keySize, keySize);
                Font arrowFont(&fontFamily, 20, FontStyleBold, UnitPixel);
                graphics.DrawString(L"◄", -1, &arrowFont, leftArrowRect, &stringFormat, &instructionBrush);
                
                // Right arrow key
                graphics.FillRectangle(&keyBrush, clientWidth / 2 + 115, keyY, keySize, keySize);
                graphics.DrawRectangle(&keyPen, clientWidth / 2 + 115, keyY, keySize, keySize);
                RectF rightArrowRect(clientWidth / 2 + 115, keyY, keySize, keySize);
                graphics.DrawString(L"►", -1, &arrowFont, rightArrowRect, &stringFormat, &instructionBrush);
                
                // Enter key (wider)
                graphics.FillRectangle(&keyBrush, clientWidth / 2 - 40, keyY, keySize * 2, keySize);
                graphics.DrawRectangle(&keyPen, clientWidth / 2 - 40, keyY, keySize * 2, keySize);
                RectF enterRect(clientWidth / 2 - 40, keyY, keySize * 2, keySize);
                Font enterFont(&fontFamily, 14, FontStyleBold, UnitPixel);
                graphics.DrawString(L"ENTER", -1, &enterFont, enterRect, &stringFormat, &instructionBrush);

                // Instruction text
                RectF instructionRect(0, clientHeight - 50, clientWidth, 40);
                graphics.DrawString(L"Navigate with ARROWS  •  Confirm with ENTER  •  ESC to Quit", -1, &instructionFont, instructionRect, &stringFormat, &instructionBrush);

            } else if (gameState == PAUSED) {
                // Update animation time
                pauseAnimTime += 0.05f;

                // Draw the game background (frozen state)
                SolidBrush blackBrush(Color(255, 0, 0, 0));
                graphics.FillRectangle(&blackBrush, 0, 0, clientWidth, clientHeight);

                // Draw center line
                Pen centerLinePen(Color(50, 255, 255, 255), 2);
                for (int y = 0; y < clientHeight; y += 20) {
                    graphics.DrawLine(&centerLinePen, clientWidth / 2, y, clientWidth / 2, y + 10);
                }

                // Draw paddles (dimmed)
                SolidBrush paddleBrush(Color(100, 255, 255, 255));
                graphics.FillRectangle(&paddleBrush, 15, (int)leftPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);
                graphics.FillRectangle(&paddleBrush, clientWidth - 15 - PADDLE_WIDTH, (int)rightPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);

                // Draw ball (dimmed with glow)
                SolidBrush ballGlowBrush(Color(50, 255, 255, 255));
                graphics.FillEllipse(&ballGlowBrush, (int)(ballX - BALL_RADIUS - 2), (int)(ballY - BALL_RADIUS - 2), (BALL_RADIUS + 2) * 2, (BALL_RADIUS + 2) * 2);
                
                SolidBrush ballBrush(Color(100, 255, 255, 255));
                graphics.FillEllipse(&ballBrush, (int)(ballX - BALL_RADIUS), (int)(ballY - BALL_RADIUS), BALL_RADIUS * 2, BALL_RADIUS * 2);

                // Draw scores (dimmed)
                Font scoreFont(&fontFamily, 48, FontStyleBold, UnitPixel);
                SolidBrush scoreBrush(Color(100, 255, 255, 255));

                std::wstring leftScoreStr = IntToWString(leftScore);
                RectF leftScoreRect(0, 30, clientWidth / 2 - 50, 80);
                graphics.DrawString(leftScoreStr.c_str(), -1, &scoreFont, leftScoreRect, &stringFormat, &scoreBrush);

                std::wstring rightScoreStr = IntToWString(rightScore);
                RectF rightScoreRect(clientWidth / 2 + 50, 30, clientWidth / 2 - 50, 80);
                graphics.DrawString(rightScoreStr.c_str(), -1, &scoreFont, rightScoreRect, &stringFormat, &scoreBrush);

                // Draw enhanced semi-transparent overlay with gradient
                LinearGradientBrush overlayGradient(
                    Point(0, 0),
                    Point(0, clientHeight),
                    Color(220, 0, 0, 20),
                    Color(220, 20, 0, 40)
                );
                graphics.FillRectangle(&overlayGradient, 0, 0, clientWidth, clientHeight);

                // Draw animated particles in pause screen
                SolidBrush particleBrush(Color(80, 100, 200, 255));
                for (int i = 0; i < 20; i++) {
                    float angle = pauseAnimTime * 0.5f + (i * 3.14159f * 2.0f / 20.0f);
                    float radius = 150 + sin(pauseAnimTime + i) * 30;
                    float x = clientWidth / 2 + cos(angle) * radius;
                    float y = clientHeight / 2 + sin(angle) * radius;
                    int size = 2 + (int)(sin(pauseAnimTime * 2 + i) * 1.5f);
                    graphics.FillEllipse(&particleBrush, (int)x - size, (int)y - size, size * 2, size * 2);
                }

                // Draw decorative frame around pause menu
                int frameMargin = 80;
                int frameWidth = 600;
                int frameHeight = 500;
                int frameX = (clientWidth - frameWidth) / 2;
                int frameY = (clientHeight - frameHeight) / 2;

                // Outer glow layers
                for (int i = 4; i > 0; i--) {
                    int alpha = 40 - i * 8;
                    int offset = i * 4;
                    Pen glowPen(Color(alpha, 100, 200, 255), 3);
                    graphics.DrawRectangle(&glowPen, frameX - offset, frameY - offset, frameWidth + offset * 2, frameHeight + offset * 2);
                }

                // Main frame
                SolidBrush frameBrush(Color(180, 10, 10, 30));
                graphics.FillRectangle(&frameBrush, frameX, frameY, frameWidth, frameHeight);
                
                Pen framePen(Color(255, 100, 200, 255), 4);
                graphics.DrawRectangle(&framePen, frameX, frameY, frameWidth, frameHeight);

                // Draw corner accents
                int accentSize = 30;
                Pen accentPen(Color(255, 255, 255, 100), 6);
                
                // Top-left corner
                graphics.DrawLine(&accentPen, frameX, frameY, frameX + accentSize, frameY);
                graphics.DrawLine(&accentPen, frameX, frameY, frameX, frameY + accentSize);
                
                // Top-right corner
                graphics.DrawLine(&accentPen, frameX + frameWidth, frameY, frameX + frameWidth - accentSize, frameY);
                graphics.DrawLine(&accentPen, frameX + frameWidth, frameY, frameX + frameWidth, frameY + accentSize);
                
                // Bottom-left corner
                graphics.DrawLine(&accentPen, frameX, frameY + frameHeight, frameX + accentSize, frameY + frameHeight);
                graphics.DrawLine(&accentPen, frameX, frameY + frameHeight, frameX, frameY + frameHeight - accentSize);
                
                // Bottom-right corner
                graphics.DrawLine(&accentPen, frameX + frameWidth, frameY + frameHeight, frameX + frameWidth - accentSize, frameY + frameHeight);
                graphics.DrawLine(&accentPen, frameX + frameWidth, frameY + frameHeight, frameX + frameWidth, frameY + frameHeight - accentSize);

                // Draw pause title with glow and pulsing effect
                Font pauseTitleFont(&fontFamily, 80, FontStyleBold, UnitPixel);
                float titlePulse = 0.9f + sin(pauseAnimTime * 3.0f) * 0.1f;
                
                // Multiple glow layers for title
                for (int i = 5; i > 0; i--) {
                    int alpha = (int)((60 - i * 10) * titlePulse);
                    SolidBrush glowBrush(Color(alpha, 255, 100, 100));
                    RectF glowRect(frameX - i * 3, frameY + 40 - i * 2, frameWidth + i * 6, 100);
                    graphics.DrawString(L"⏸ PAUSED", -1, &pauseTitleFont, glowRect, &stringFormat, &glowBrush);
                }
                
                // Main title with gradient
                RectF pauseTitleRect(frameX, frameY + 40, frameWidth, 100);
                LinearGradientBrush titleGradient(
                    Point(frameX + frameWidth / 2, frameY + 40),
                    Point(frameX + frameWidth / 2, frameY + 140),
                    Color((int)(255 * titlePulse), 255, 150, 150),
                    Color((int)(255 * titlePulse), 255, 100, 100)
                );
                graphics.DrawString(L"⏸ PAUSED", -1, &pauseTitleFont, pauseTitleRect, &stringFormat, &titleGradient);

                // Draw decorative line under title
                Pen dividerPen(Color(200, 100, 200, 255), 2);
                int dividerY = frameY + 160;
                graphics.DrawLine(&dividerPen, frameX + 50, dividerY, frameX + frameWidth - 50, dividerY);

                if (isCountingDown) {
                    // Update countdown timer
                    countdownTimer -= 0.016f; // Decrease by ~60FPS
                    if (countdownTimer <= 0.0f) {
                        countdownTimer = 0.0f;
                        gameState = PLAYING;
                        isCountingDown = false;
                    }

                    // Draw countdown with elaborate effects
                    int countdown = (int)countdownTimer + 1;
                    if (countdown > 3) countdown = 3;
                    
                    Font countdownFont(&fontFamily, 180, FontStyleBold, UnitPixel);
                    std::wstring countdownStr = IntToWString(countdown);
                    
                    // Countdown animation effects
                    float countdownScale = 1.0f + (1.0f - (countdownTimer - (int)countdownTimer)) * 0.3f;
                    int countdownAlpha = (int)(255 * (0.3f + (countdownTimer - (int)countdownTimer) * 0.7f));
                    
                    // Outer glow rings
                    for (int ring = 5; ring > 0; ring--) {
                        int ringAlpha = (int)((100 - ring * 15) * (countdownTimer - (int)countdownTimer));
                        SolidBrush ringBrush(Color(ringAlpha, 100, 255, 100));
                        RectF ringRect(frameX - ring * 5, frameY + 200 - ring * 5, frameWidth + ring * 10, 200);
                        graphics.DrawString(countdownStr.c_str(), -1, &countdownFont, ringRect, &stringFormat, &ringBrush);
                    }
                    
                    // Main countdown number with gradient
                    RectF countdownRect(frameX, frameY + 200, frameWidth, 200);
                    LinearGradientBrush countdownGradient(
                        Point(frameX + frameWidth / 2, frameY + 200),
                        Point(frameX + frameWidth / 2, frameY + 400),
                        Color(countdownAlpha, 100, 255, 255),
                        Color(countdownAlpha, 100, 255, 100)
                    );
                    graphics.DrawString(countdownStr.c_str(), -1, &countdownFont, countdownRect, &stringFormat, &countdownGradient);

                    // Draw "Resuming..." text
                    Font resumingFont(&fontFamily, 28, FontStyleItalic, UnitPixel);
                    SolidBrush resumingBrush(Color(200, 200, 200, 200));
                    RectF resumingRect(frameX, frameY + 420, frameWidth, 40);
                    graphics.DrawString(L"Resuming game...", -1, &resumingFont, resumingRect, &stringFormat, &resumingBrush);

                } else {
                    // Draw menu options with cards
                    int optionY = frameY + 220;
                    int optionWidth = 400;
                    int optionHeight = 90;
                    int optionX = frameX + (frameWidth - optionWidth) / 2;
                    int optionSpacing = 120;

                    const wchar_t* optionTexts[] = {L"▶ RESUME", L"🏠 MAIN MENU"};
                    Color optionColors[] = {
                        Color(255, 100, 255, 100),  // Green for Resume
                        Color(255, 255, 100, 100)   // Red for Menu
                    };

                    for (int i = 0; i < 2; i++) {
                        int currentY = optionY + i * optionSpacing;
                        bool isSelected = (pauseMenuSelection == i);
                        
                        // Calculate pulse effect
                        float pulse = isSelected ? (0.85f + sin(pauseAnimTime * 5.0f) * 0.15f) : 0.4f;
                        
                        // Draw option glow
                        if (isSelected) {
                            for (int glow = 3; glow > 0; glow--) {
                                int glowAlpha = (int)((60 - glow * 15) * pulse);
                                SolidBrush glowBrush(Color(glowAlpha, optionColors[i].GetR(), optionColors[i].GetG(), optionColors[i].GetB()));
                                graphics.FillRectangle(&glowBrush, 
                                    optionX - glow * 4, 
                                    currentY - glow * 4, 
                                    optionWidth + glow * 8, 
                                    optionHeight + glow * 8);
                            }
                        }
                        
                        // Draw option background
                        SolidBrush optionBrush(Color((int)(150 * pulse), 20, 20, 50));
                        graphics.FillRectangle(&optionBrush, optionX, currentY, optionWidth, optionHeight);
                        
                        // Draw option border
                        Pen optionPen(Color((int)(255 * pulse), optionColors[i].GetR(), optionColors[i].GetG(), optionColors[i].GetB()), 
                                     isSelected ? 5 : 2);
                        graphics.DrawRectangle(&optionPen, optionX, currentY, optionWidth, optionHeight);
                        
                        // Draw selection indicator (animated arrow)
                        if (isSelected) {
                            SolidBrush arrowBrush(Color(255, 255, 255, 200));
                            float arrowOffset = sin(pauseAnimTime * 6.0f) * 8;
                            
                            Point arrowPoints[3];
                            arrowPoints[0] = Point((int)(optionX - 25 + arrowOffset), currentY + optionHeight / 2);
                            arrowPoints[1] = Point((int)(optionX - 40 + arrowOffset), currentY + optionHeight / 2 - 12);
                            arrowPoints[2] = Point((int)(optionX - 40 + arrowOffset), currentY + optionHeight / 2 + 12);
                            graphics.FillPolygon(&arrowBrush, arrowPoints, 3);
                        }
                        
                        // Draw option text
                        Font optionFont(&fontFamily, 40, FontStyleBold, UnitPixel);
                        SolidBrush textBrush(Color((int)(255 * pulse), 255, 255, 255));
                        RectF textRect(optionX, currentY, optionWidth, optionHeight);
                        graphics.DrawString(optionTexts[i], -1, &optionFont, textRect, &stringFormat, &textBrush);
                    }

                    // Draw instructions at bottom with enhanced styling
                    Font instructionFont(&fontFamily, 20, FontStyleRegular, UnitPixel);
                    SolidBrush instructionBrush(Color(180, 200, 200, 200));
                    RectF instructionRect(frameX, frameY + frameHeight - 60, frameWidth, 40);
                    graphics.DrawString(L"Use ← → to navigate  •  Press ENTER to select  •  P to resume", 
                                      -1, &instructionFont, instructionRect, &stringFormat, &instructionBrush);

                    // Draw tip text
                    Font tipFont(&fontFamily, 16, FontStyleItalic, UnitPixel);
                    SolidBrush tipBrush(Color(150, 150, 150, 150));
                    RectF tipRect(frameX, frameY + frameHeight - 30, frameWidth, 25);
                    graphics.DrawString(L"💡 Take a break, champion!", -1, &tipFont, tipRect, &stringFormat, &tipBrush);
                }

            } else {
                // Game is playing - black background only
                SolidBrush blackBrush(Color(255, 0, 0, 0));
                graphics.FillRectangle(&blackBrush, 0, 0, clientWidth, clientHeight);

                // Update paddle positions
                if (wKeyPressed && leftPaddleY > 0) {
                    leftPaddleY -= currentPaddleSpeed;
                    if (leftPaddleY < 0) leftPaddleY = 0;
                }
                if (sKeyPressed && leftPaddleY < clientHeight - PADDLE_HEIGHT) {
                    leftPaddleY += currentPaddleSpeed;
                    if (leftPaddleY > clientHeight - PADDLE_HEIGHT) leftPaddleY = clientHeight - PADDLE_HEIGHT;
                }
                if (upKeyPressed && rightPaddleY > 0) {
                    rightPaddleY -= currentPaddleSpeed;
                    if (rightPaddleY < 0) rightPaddleY = 0;
                }
                if (downKeyPressed && rightPaddleY < clientHeight - PADDLE_HEIGHT) {
                    rightPaddleY += currentPaddleSpeed;
                    if (rightPaddleY > clientHeight - PADDLE_HEIGHT) rightPaddleY = clientHeight - PADDLE_HEIGHT;
                }

                // Store previous position for continuous collision detection
                float prevBallX = ballX;
                float prevBallY = ballY;
                
                // Update ball position
                ballX += ballVelocityX;
                ballY += ballVelocityY;

                // Ball collision with top and bottom (screen boundaries)
                if (ballY - BALL_RADIUS <= 0) {
                    ballVelocityY = abs(ballVelocityY);
                    ballY = BALL_RADIUS;
                } else if (ballY + BALL_RADIUS >= clientHeight) {
                    ballVelocityY = -abs(ballVelocityY);
                    ballY = clientHeight - BALL_RADIUS;
                }

                // Continuous collision detection for left paddle
                if (ballVelocityX < 0) { // Ball moving left
                    float paddleX = 20;
                    float paddleTop = leftPaddleY;
                    float paddleBottom = leftPaddleY + PADDLE_HEIGHT;
                    
                    // Check if ball crosses paddle X position
                    if (prevBallX - BALL_RADIUS > paddleX && ballX - BALL_RADIUS <= paddleX) {
                        // Calculate Y position when ball reaches paddle X
                        float t = (paddleX - (prevBallX - BALL_RADIUS)) / (ballVelocityX);
                        float intersectY = prevBallY + ballVelocityY * t;
                        
                        // Check if intersection Y is within paddle bounds (with some tolerance)
                        if (intersectY >= paddleTop - BALL_RADIUS && intersectY <= paddleBottom + BALL_RADIUS) {
                            // Collision detected!
                            hitCount++;
                            
                            // Apply speed increase
                            if (hitCount <= MAX_HITS_FOR_SPEED_INCREASE) {
                                ballVelocityX = abs(ballVelocityX) * currentSpeedFactor;
                                ballVelocityY *= currentSpeedFactor;
                            } else {
                                ballVelocityX = abs(ballVelocityX);
                            }
                            
                            // Position ball at paddle surface
                            ballX = paddleX + BALL_RADIUS;
                            ballY = intersectY;
                            
                            // Add trajectory variation based on hit position
                            float hitPos = (intersectY - paddleTop) / PADDLE_HEIGHT;
                            hitPos = (hitPos < 0) ? 0 : (hitPos > 1) ? 1 : hitPos;
                            ballVelocityY += (hitPos - 0.5f) * 6.0f;
                        }
                    }
                }

                // Continuous collision detection for right paddle
                if (ballVelocityX > 0) { // Ball moving right
                    float paddleX = clientWidth - 20;
                    float paddleTop = rightPaddleY;
                    float paddleBottom = rightPaddleY + PADDLE_HEIGHT;
                    
                    // Check if ball crosses paddle X position
                    if (prevBallX + BALL_RADIUS < paddleX && ballX + BALL_RADIUS >= paddleX) {
                        // Calculate Y position when ball reaches paddle X
                        float t = (paddleX - (prevBallX + BALL_RADIUS)) / (ballVelocityX);
                        float intersectY = prevBallY + ballVelocityY * t;
                        
                        // Check if intersection Y is within paddle bounds (with some tolerance)
                        if (intersectY >= paddleTop - BALL_RADIUS && intersectY <= paddleBottom + BALL_RADIUS) {
                            // Collision detected!
                            hitCount++;
                            
                            // Apply speed increase
                            if (hitCount <= MAX_HITS_FOR_SPEED_INCREASE) {
                                ballVelocityX = -abs(ballVelocityX) * currentSpeedFactor;
                                ballVelocityY *= currentSpeedFactor;
                            } else {
                                ballVelocityX = -abs(ballVelocityX);
                            }
                            
                            // Position ball at paddle surface
                            ballX = paddleX - BALL_RADIUS;
                            ballY = intersectY;
                            
                            // Add trajectory variation based on hit position
                            float hitPos = (intersectY - paddleTop) / PADDLE_HEIGHT;
                            hitPos = (hitPos < 0) ? 0 : (hitPos > 1) ? 1 : hitPos;
                            ballVelocityY += (hitPos - 0.5f) * 6.0f;
                        }
                    }
                }

                // Ball goes off the left side - right player scores
                if (ballX + BALL_RADIUS < 0) {
                    rightScore++;
                    // Reset ball to center
                    ballX = clientWidth / 2.0f;
                    ballY = clientHeight / 2.0f;
                    ballVelocityX = 5.0f; // Start towards right player
                    ballVelocityY = 3.0f;
                    hitCount = 0;
                }

                // Ball goes off the right side - left player scores
                if (ballX - BALL_RADIUS > clientWidth) {
                    leftScore++;
                    // Reset ball to center
                    ballX = clientWidth / 2.0f;
                    ballY = clientHeight / 2.0f;
                    ballVelocityX = -5.0f; // Start towards left player
                    ballVelocityY = 3.0f;
                    hitCount = 0;
                }

                // Draw center line
                Pen centerLinePen(Color(100, 255, 255, 255), 2);
                for (int y = 0; y < clientHeight; y += 20) {
                    graphics.DrawLine(&centerLinePen, clientWidth / 2, y, clientWidth / 2, y + 10);
                }

                // Draw paddles (vertical rectangles for better visual)
                SolidBrush paddleBrush(Color(255, 255, 255, 255));
                graphics.FillRectangle(&paddleBrush, 15, (int)leftPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);
                graphics.FillRectangle(&paddleBrush, clientWidth - 15 - PADDLE_WIDTH, (int)rightPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);

                // Draw ball with slight glow
                SolidBrush ballGlowBrush(Color(100, 255, 255, 255));
                graphics.FillEllipse(&ballGlowBrush, (int)(ballX - BALL_RADIUS - 2), (int)(ballY - BALL_RADIUS - 2), (BALL_RADIUS + 2) * 2, (BALL_RADIUS + 2) * 2);
                
                SolidBrush ballBrush(Color(255, 255, 255, 255));
                graphics.FillEllipse(&ballBrush, (int)(ballX - BALL_RADIUS), (int)(ballY - BALL_RADIUS), BALL_RADIUS * 2, BALL_RADIUS * 2);

                // Draw scores
                Font scoreFont(&fontFamily, 48, FontStyleBold, UnitPixel);
                SolidBrush scoreBrush(Color(255, 255, 255, 255));

                // Left player score
                std::wstring leftScoreStr = IntToWString(leftScore);
                RectF leftScoreRect(0, 30, clientWidth / 2 - 50, 80);
                graphics.DrawString(leftScoreStr.c_str(), -1, &scoreFont, leftScoreRect, &stringFormat, &scoreBrush);

                // Right player score
                std::wstring rightScoreStr = IntToWString(rightScore);
                RectF rightScoreRect(clientWidth / 2 + 50, 30, clientWidth / 2 - 50, 80);
                graphics.DrawString(rightScoreStr.c_str(), -1, &scoreFont, rightScoreRect, &stringFormat, &scoreBrush);
            }

            // Copy from memory DC to screen (eliminates flickering)
            BitBlt(hdc, 0, 0, clientWidth, clientHeight, memDC, 0, 0, SRCCOPY);
            
            // Cleanup
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            // Prevent background erasing to reduce flicker
            return 1;
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprev, PSTR cmdline, int cmdshow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Load background image
    backgroundImage = new Image(BACKGROUND_IMAGE);

    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hinstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class", "Error", MB_OK);
        return 1;
    }

    // Calculate window size to account for borders and title bar
    RECT window_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create window
    HWND hwnd = CreateWindowExA(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        NULL,
        NULL,
        hinstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_OK);
        return 1;
    }

    // Show window
    ShowWindow(hwnd, cmdshow);
    UpdateWindow(hwnd);

    // Message loop with game update
    MSG msg = {};
    while (true) {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Update game when idle
            InvalidateRect(hwnd, NULL, FALSE);
            Sleep(16); // ~60 FPS
        }
    }

    // Cleanup
    if (backgroundImage) {
        delete backgroundImage;
    }
    GdiplusShutdown(gdiplusToken);

    return 0;
}