# 🎮 Pong Game - My First C++ Project

My first game development project built entirely using **vibecode** methodology (AI-assisted development). This is a classic Pong clone created with C++ and Windows GDI+.

## 🚀 Project Status

**⚠️ Work in Progress** - This game is still under development and may contain bugs and incomplete features.

## 📋 About This Project

This project represents my journey into C++ game development with the help of AI assistance. Every line of code, bug fix, and feature was developed through AI collaboration, demonstrating the power of vibecode development.

### What is Vibecode?

Vibecode is my approach to development where I use AI assistance for the entire project lifecycle - from initial code structure to debugging and feature implementation.

## ✨ Current Features

- ✅ Main menu with background image
- ✅ Two-player paddle controls
- ✅ Smooth 60 FPS gameplay
- ✅ Double-buffered rendering (no screen flickering)
- ✅ Basic game state management

## 🎮 Controls

| Action                | Key            |
| --------------------- | -------------- |
| **Left Paddle Up**    | W              |
| **Left Paddle Down**  | S              |
| **Right Paddle Up**   | ↑ (Up Arrow)   |
| **Right Paddle Down** | ↓ (Down Arrow) |
| **Start Game**        | Any Key        |
| **Exit Game**         | ESC            |

## 🛠️ Technologies Used

- **Language:** C++
- **Graphics:** Windows GDI+
- **Platform:** Windows
- **Compiler:** MinGW (GCC)

## 📦 Installation & Running

### Prerequisites

- MinGW (GCC compiler for Windows)
- Windows OS

### Building the Game

1. Clone this repository:

```bash
git clone https://github.com/yourusername/pong-game.git
cd pong-game
```

2. Compile the game:

```bash
g++ -o game.exe main.cpp -lgdiplus -lgdi32 -luser32 -mwindows
```

3. Run the game:

```bash
./game.exe
```

## 📁 Project Structure

```
pong-game/
├── main.cpp                    # Main game code
├── assets/
│   └── background-menu.png     # Menu background image
├── game.exe                    # Compiled executable (after build)
└── README.md                   # This file
```

## 🐛 Known Issues

- [ ] No ball implementation yet
- [ ] No scoring system
- [ ] No collision detection
- [ ] No AI opponent for single player
- [ ] Paddles can move outside bounds in some edge cases
- [ ] No sound effects
- [ ] Window borders still visible (need borderless mode)

## 🗺️ Roadmap

- [ ] Add ball with physics
- [ ] Implement collision detection
- [ ] Add scoring system
- [ ] Create AI opponent
- [ ] Add sound effects
- [ ] Implement game over/restart functionality
- [ ] Add settings menu
- [ ] Improve graphics and animations
- [ ] Add particle effects
- [ ] Create different difficulty levels

## 🤖 Development Methodology

This entire project was built using AI assistance (vibecode approach):

- Code structure and implementation
- Bug fixing and debugging
- Feature additions
- Documentation

## 🎓 What I Learned

- Windows API basics
- GDI+ graphics rendering
- Game loop implementation
- Double buffering for smooth graphics
- Event-driven programming
- Asset management in C++
- Working with AI for code development

## 🤝 Contributing

This is a personal learning project, but suggestions and feedback are welcome! Feel free to:

- Open issues for bugs
- Suggest new features
- Share learning resources

## 📞 Contact

Feel free to reach out if you have questions or suggestions!

---

**Note:** This is my first C++ game project and a learning experience. Code quality may not be production-ready, but it represents my journey in learning game development with AI assistance.
