enum class WINDOW_FLAGS : unsigned int {
    FULLSCREEN          = (1 << 0),
    FULLSCREEN_DESKTOP  = (1 << 1 | FULLSCREEN),
    BORDERLESS          = (1 << 2),
    RESIZABLE           = (1 << 3),
    SHOWN               = (1 << 4),
    HIDDEN              = (1 << 5),
    MINIMIZED           = (1 << 6),
    MAXIMIZED           = (1 << 7),

    OPENGL              = (1 << 8),
    VULKAN              = (1 << 9),
    DIRECTX             = (1 << 10),
    METAL               = (1 << 11),
};

enum class WINDOW_POS : int {
    UNDEFINED = 0x1FFF0000,
    CENTERED = 0x2FFF0000,
};

class IWindow {
public:
    IWindow(const IWindow&)             = default;
    IWindow(IWindow&&)                  = default;
    IWindow& operator=(const IWindow&)  = default;
    IWindow& operator=(IWindow&&)       = default;
    virtual ~IWindow()                  = default;

    // \param title     UTF-8 encoded window title
    // \param x         X position of window or WINDOW_POS enum member
    // \param y         Y position of window or WINDOW_POS enum member
    // \param flags     
    virtual void Create(const char* title, int x, int y, int w, int h, WINDOW_FLAGS flags) = 0;
    virtual void Close() = 0;
};
