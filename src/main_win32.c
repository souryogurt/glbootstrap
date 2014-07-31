#pragma warning( push )
#pragma warning( disable: 4668 )
#pragma warning( disable: 4820 )
#include <tchar.h>
#include <Windows.h>
#include <strsafe.h>
#pragma warning( pop )

#pragma warning( disable: 4710 )
#define EGLAPI
#include <EGL/egl.h>
#include <GL/GL.h>

static const TCHAR *MainWindowClassName = _T ("BOOTSTRAP Window Class");

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage (0);
            break;
        case WM_CREATE:
            break;
        default:
            return DefWindowProc (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int CALLBACK WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine, int nCmdShow)
{
    static const EGLint egl_attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLDisplay egl_display;
    EGLint egl_major, egl_minor;
    EGLBoolean err;
    EGLConfig config;
    EGLint n_configs;
    EGLContext context;
    EGLSurface window_surface;
    WNDCLASSEX wc;
    DWORD dwStyle;
    HWND main_window;
    MSG msg;
    RECT window_size = {0, 0, 640, 480};
    BOOL window_closed = FALSE;

    egl_display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY) {
        MessageBox (NULL, _T ("No matching EGL_DEFAULT_DISPLAY is available"),
                    0, MB_OK + MB_ICONINFORMATION);
        return 1;
    }
    if (eglInitialize (egl_display, &egl_major, &egl_minor) != EGL_TRUE) {
        MessageBox (NULL, _T ("Can't initialize EGL on a display"), 0,
                    MB_OK + MB_ICONINFORMATION);
        return 1;
    }

    err = eglChooseConfig (egl_display, egl_attributes, &config, 1, &n_configs);
    if ((err != EGL_TRUE) || (n_configs == 0)) {
        MessageBox (NULL, _T ("No matching framebuffer configuration"), 0,
                    MB_OK + MB_ICONINFORMATION);
        eglTerminate (egl_display);
        return 1;
    }

    err = eglBindAPI (EGL_OPENGL_API);
    if (err != EGL_TRUE) {
        MessageBox (NULL, _T ("Can't bind OpenGL API"), 0,
                    MB_OK + MB_ICONINFORMATION);
        eglTerminate (egl_display);
        return 1;
    }

    context = eglCreateContext (egl_display, config, EGL_NO_CONTEXT, NULL);
    if (context == EGL_NO_CONTEXT) {
        MessageBox (NULL, _T ("Can't create OpenGL context"), 0,
                    MB_OK + MB_ICONINFORMATION);
        eglTerminate (egl_display);
        return 1;
    }

    memset (&wc, 0, sizeof (wc));
    wc.cbSize = sizeof (WNDCLASSEX);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WindowProcedure;
    wc.lpszClassName = MainWindowClassName;
    wc.style = CS_OWNDC;

    if ( RegisterClassEx (&wc) == (ATOM)NULL ) {
        MessageBox (NULL, _T ("Can't register a window class"), 0,
                    MB_OK + MB_ICONINFORMATION);
        eglDestroyContext (egl_display, context);
        eglTerminate (egl_display);
        return 1;
    }
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    AdjustWindowRect (&window_size, dwStyle & ~WS_OVERLAPPED, FALSE);
    main_window = CreateWindowEx (0, MainWindowClassName, _T ("OpenGL Window"),
                                  dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                                  window_size.right - window_size.left,
                                  window_size.bottom - window_size.top,
                                  0, 0, hInstance, 0);
    ShowWindow (main_window, nCmdShow);


    window_surface = eglCreateWindowSurface (egl_display, config,
                     (NativeWindowType) main_window, NULL);
    if (window_surface == EGL_NO_SURFACE) {
        MessageBox (NULL, _T ("Can't create window OpenGL surface"), 0,
                    MB_OK + MB_ICONINFORMATION);
        DestroyWindow (main_window);
        UnregisterClass (MainWindowClassName, hInstance);
        eglDestroyContext (egl_display, context);
        eglTerminate (egl_display);
        return 1;
    }

    err = eglMakeCurrent (egl_display, window_surface, window_surface, context);
    if (err == EGL_FALSE) {
        MessageBox (NULL, _T ("Can't make OpenGL context be current"), 0,
                    MB_OK + MB_ICONINFORMATION);
        eglDestroySurface (egl_display, window_surface);
        DestroyWindow (main_window);
        UnregisterClass (MainWindowClassName, hInstance);
        eglDestroyContext (egl_display, context);
        eglTerminate (egl_display);
        return 1;
    }

    window_closed = FALSE;
    while (window_closed == FALSE) {
        while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                window_closed = TRUE;
                break;
            }
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
        eglSwapBuffers (egl_display, window_surface);
    }
    eglMakeCurrent (egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                    EGL_NO_CONTEXT);
    eglDestroySurface (egl_display, window_surface);
    DestroyWindow (main_window);
    UnregisterClass (MainWindowClassName, hInstance);
    eglDestroyContext (egl_display, context);
    eglTerminate (egl_display);
    return 0;
}
