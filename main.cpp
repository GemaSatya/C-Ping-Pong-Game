#include <windows.h>
#include <gdiplus.h>
#include <string>

using namespace Gdiplus;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_CLASS_NAME = "GameWindow";
const char* WINDOW_TITLE = "Game";
const wchar_t* BACKGROUND_IMAGE = L"assets/background-menu.png";

ULONG_PTR gdiplusToken;
Image* backgroundImage = nullptr;

// Game states
enum GameState {
    MENU,
    DIFFICULTY_SELECT,
    PLAYING
};
GameState gameState = MENU;
int selectedDifficulty = -1; // -1: none, 0: easy, 1: medium, 2: hard

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

// Key state tracking
bool wKeyPressed = false;
bool sKeyPressed = false;
bool upKeyPressed = false;
bool downKeyPressed = false;

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
                }
            } else if (wparam == VK_RIGHT && gameState == DIFFICULTY_SELECT) {
                if (selectedDifficulty < 2) {
                    selectedDifficulty++;
                }
            } else if (wparam == VK_RETURN && gameState == DIFFICULTY_SELECT) {
                // Start game with selected difficulty
                gameState = PLAYING;
                // Set difficulty parameters
                if (selectedDifficulty == 0) { // Easy
                    currentSpeedFactor = 1.25f;
                    currentPaddleSpeed = PADDLE_SPEED;
                } else if (selectedDifficulty == 1) { // Medium
                    currentSpeedFactor = 1.75f;
                    currentPaddleSpeed = PADDLE_SPEED + 3;
                } else if (selectedDifficulty == 2) { // Hard
                    currentSpeedFactor = 2.10f;
                    currentPaddleSpeed = PADDLE_SPEED + 6;
                }
            }
            
            if (gameState == MENU) {
                gameState = DIFFICULTY_SELECT;
                selectedDifficulty = 0; // Default to easy
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

            if (gameState == MENU) {
                // Draw background image
                if (backgroundImage) {
                    graphics.DrawImage(backgroundImage, 0, 0, clientWidth, clientHeight);
                } else {
                    // Fallback to black if image fails to load
                    SolidBrush brush(Color(255, 0, 0, 0));
                    graphics.FillRectangle(&brush, 0, 0, clientWidth, clientHeight);
                }

                // Draw text
                FontFamily fontFamily(L"Arial");
                Font font(&fontFamily, 48, FontStyleBold, UnitPixel);
                SolidBrush textBrush(Color(255, 255, 255, 255)); // White text
                StringFormat stringFormat;
                stringFormat.SetAlignment(StringAlignmentCenter);
                stringFormat.SetLineAlignment(StringAlignmentCenter);

                RectF layoutRect(0, clientHeight / 2 - 50, clientWidth, 100);
                graphics.DrawString(L"Press Any Button to Start", -1, &font, layoutRect, &stringFormat, &textBrush);
            } else if (gameState == DIFFICULTY_SELECT) {
                // Draw background image for difficulty selection
                if (backgroundImage) {
                    graphics.DrawImage(backgroundImage, 0, 0, clientWidth, clientHeight);
                } else {
                    // Fallback to black if image fails to load
                    SolidBrush brush(Color(255, 0, 0, 0));
                    graphics.FillRectangle(&brush, 0, 0, clientWidth, clientHeight);
                }

                // Draw difficulty selection menu
                FontFamily fontFamily(L"Arial");
                Font titleFont(&fontFamily, 56, FontStyleBold, UnitPixel);
                Font optionFont(&fontFamily, 40, FontStyleRegular, UnitPixel);
                SolidBrush titleBrush(Color(255, 255, 255, 255)); // White text
                StringFormat stringFormat;
                stringFormat.SetAlignment(StringAlignmentCenter);
                stringFormat.SetLineAlignment(StringAlignmentCenter);

                // Draw title
                RectF titleRect(0, 100, clientWidth, 80);
                graphics.DrawString(L"Select Difficulty", -1, &titleFont, titleRect, &stringFormat, &titleBrush);

                // Draw difficulty options
                int optionY = 250;
                int optionSpacing = 100;

                for (int i = 0; i < 3; i++) {
                    const wchar_t* difficultyNames[] = {L"EASY", L"MEDIUM", L"HARD"};
                    
                    // Highlight selected difficulty
                    if (i == selectedDifficulty) {
                        SolidBrush highlightBrush(Color(255, 255, 200, 0)); // Orange highlight
                        RectF highlightRect((clientWidth / 2 - 150) + i * 300, optionY - 20, 150, 80);
                        graphics.FillRectangle(&highlightBrush, highlightRect);
                        
                        // Draw white text for selected
                        SolidBrush selectedBrush(Color(255, 255, 255, 255));
                        RectF optionRect((clientWidth / 2 - 150) + i * 300, optionY, 150, 60);
                        graphics.DrawString(difficultyNames[i], -1, &optionFont, optionRect, &stringFormat, &selectedBrush);
                    } else {
                        // Draw gray text for non-selected
                        SolidBrush normalBrush(Color(255, 150, 150, 150));
                        RectF optionRect((clientWidth / 2 - 150) + i * 300, optionY, 150, 60);
                        graphics.DrawString(difficultyNames[i], -1, &optionFont, optionRect, &stringFormat, &normalBrush);
                    }
                }

                // Draw instructions
                Font instructionFont(&fontFamily, 20, FontStyleRegular, UnitPixel);
                SolidBrush instructionBrush(Color(255, 200, 200, 200));
                RectF instructionRect(0, 550, clientWidth, 100);
                graphics.DrawString(L"Use LEFT and RIGHT arrows to select, ENTER to confirm", -1, &instructionFont, instructionRect, &stringFormat, &instructionBrush);
            } else {
                // Game is playing - black background only
                SolidBrush blackBrush(Color(255, 0, 0, 0));
                graphics.FillRectangle(&blackBrush, 0, 0, clientWidth, clientHeight);

                // Update paddle positions
                if (wKeyPressed && leftPaddleY > 0) {
                    leftPaddleY -= currentPaddleSpeed;
                }
                if (sKeyPressed && leftPaddleY < clientHeight - PADDLE_HEIGHT) {
                    leftPaddleY += currentPaddleSpeed;
                }
                if (upKeyPressed && rightPaddleY > 0) {
                    rightPaddleY -= currentPaddleSpeed;
                }
                if (downKeyPressed && rightPaddleY < clientHeight - PADDLE_HEIGHT) {
                    rightPaddleY += currentPaddleSpeed;
                }

                // Update ball position
                ballX += ballVelocityX;
                ballY += ballVelocityY;

                // Ball collision with top and bottom (screen boundaries)
                if (ballY - BALL_RADIUS <= 0 || ballY + BALL_RADIUS >= clientHeight) {
                    ballVelocityY = -ballVelocityY;
                    // Clamp ball to screen
                    if (ballY - BALL_RADIUS < 0) {
                        ballY = BALL_RADIUS;
                    } else {
                        ballY = clientHeight - BALL_RADIUS;
                    }
                }

                // Ball collision with left paddle
                if (ballX - BALL_RADIUS <= 30 && ballX + BALL_RADIUS >= 20 &&
                    ballY >= leftPaddleY && ballY <= leftPaddleY + PADDLE_HEIGHT) {
                    hitCount++;
                    if (hitCount <= MAX_HITS_FOR_SPEED_INCREASE) {
                        ballVelocityX = -ballVelocityX * currentSpeedFactor;
                        ballVelocityY *= currentSpeedFactor; // Increase Y velocity too
                    } else {
                        ballVelocityX = -ballVelocityX;
                    }
                    ballX = 30 + BALL_RADIUS;
                    // Add some trajectory based on where the ball hit the paddle
                    float hitPos = (ballY - leftPaddleY) / PADDLE_HEIGHT; // 0 to 1
                    ballVelocityY += (hitPos - 0.5f) * 4.0f; // Add angle based on hit position
                }

                // Ball collision with right paddle
                if (ballX + BALL_RADIUS >= clientWidth - 30 && ballX - BALL_RADIUS <= clientWidth - 20 &&
                    ballY >= rightPaddleY && ballY <= rightPaddleY + PADDLE_HEIGHT) {
                    hitCount++;
                    if (hitCount <= MAX_HITS_FOR_SPEED_INCREASE) {
                        ballVelocityX = -ballVelocityX * currentSpeedFactor;
                        ballVelocityY *= currentSpeedFactor; // Increase Y velocity too
                    } else {
                        ballVelocityX = -ballVelocityX;
                    }
                    ballX = clientWidth - 30 - BALL_RADIUS;
                    // Add some trajectory based on where the ball hit the paddle
                    float hitPos = (ballY - rightPaddleY) / PADDLE_HEIGHT; // 0 to 1
                    ballVelocityY += (hitPos - 0.5f) * 4.0f; // Add angle based on hit position
                }

                // Draw paddles (vertical lines)
                Pen paddlePen(Color(255, 255, 255, 255), PADDLE_WIDTH); // White color
                graphics.DrawLine(&paddlePen, 20, (int)leftPaddleY, 20, (int)(leftPaddleY + PADDLE_HEIGHT));
                graphics.DrawLine(&paddlePen, clientWidth - 20, (int)rightPaddleY, clientWidth - 20, (int)(rightPaddleY + PADDLE_HEIGHT));

                // Draw ball
                SolidBrush ballBrush(Color(255, 255, 255, 255)); // White ball
                graphics.FillEllipse(&ballBrush, (int)(ballX - BALL_RADIUS), (int)(ballY - BALL_RADIUS), BALL_RADIUS * 2, BALL_RADIUS * 2);
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
            if (gameState == PLAYING) {
                InvalidateRect(hwnd, NULL, FALSE);
            } else if (gameState == DIFFICULTY_SELECT || gameState == MENU) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
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