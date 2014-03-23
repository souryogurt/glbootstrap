#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#define PACKAGE_STRING "glbootstrap 0.1"
#define PACKAGE_BUGREPORT "egor.artemov@gmail.com"

typedef struct glx_context_info_t {
    /* common glx data */
    GLXContext context;
    XVisualInfo *visual_info;

    /* modern glx data */
    GLXFBConfig best_fbconfig;
    GLXWindow glx_window;
} glx_context_info_t;

/** Window type */
typedef struct game_window_t {
    Display *display;
    Atom wm_delete_window;
    Window xwindow;
    Colormap cmap;
    int is_closed;
    int width;
    int height;
    char padding[4];
} game_window_t;

static game_window_t *main_window = NULL;

static glx_context_info_t context_info = {NULL, NULL, NULL, (GLXWindow)NULL};

/** Version of glx */
static int glx_major, glx_minor;

/* GLX_ARB_create_context */
static int arb_create_context_extension = 0;

/* GLX_ARB_create_context_profile */
static int arb_context_profile = 0;
static PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = NULL;

/** The name the program was run with */
static const char *program_name;

/** License text to show when application is runned with --version flag */
static const char *version_text =
    PACKAGE_STRING "\n\n"
    "Copyright (C) 2014 Egor Artemov <egor.artemov@gmail.com>\n"
    "This work is free. You can redistribute it and/or modify it under the\n"
    "terms of the Do What The Fuck You Want To Public License, Version 2,\n"
    "as published by Sam Hocevar. See http://www.wtfpl.net for more details.\n";

/* Option flags and variables */
static struct option const long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {NULL, 0, NULL, 0}
};

/** Process all pending events
 * @param window events of this window should be processed
 */
static void window_process_events (game_window_t *window)
{
    XConfigureEvent xce;

    int count = XPending (window->display);
    while (count > 0) {
        XEvent event;
        XNextEvent (window->display, &event);
        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == window->wm_delete_window) {
                    window->is_closed = 1;
                }
                break;
            case ConfigureNotify:
                xce = event.xconfigure;
                if ((xce.width != window->width)
                        || (xce.height != window->height)) {
                    window->width = xce.width;
                    window->height = xce.height;
                    /*game_resize (window->width, window->height);*/
                }
                break;
            default:
                break;
        }
        count--;
    }
}

/** Check is window isn't closed
 * @returns non-zero if closed, 0 otherwise
 */
static int window_is_exists (game_window_t *window)
{
    return window->is_closed == 0;
}

/** Destroy window and free all related resources
 * @param window window to destroy
 */
static void window_destroy (game_window_t *window)
{
    if (window != NULL) {
        XDestroyWindow (window->display, window->xwindow);
        XFreeColormap (window->display, window->cmap);
        free (window);
    }
}

/** Create and display new window
 * @param display The display where window should be created
 * @param vi VisualInfo that should be used to create a window
 * @returns new window object, NULL otherwise
 */
static game_window_t *window_create (Display *display, XVisualInfo *vi)
{
    const unsigned long valuemask = CWBorderPixel | CWColormap | CWEventMask;
    XSetWindowAttributes swa;
    game_window_t *window = (game_window_t *)malloc (sizeof (game_window_t));
    if (window != NULL) {
        Window root = RootWindow (display, vi->screen);
        window->display = display;
        window->is_closed = 0;
        window->width = 0;
        window->height = 0;
        window->cmap = XCreateColormap (display, root, vi->visual, AllocNone);
        swa.colormap = window->cmap;
        swa.background_pixmap = None;
        swa.border_pixel = 0;
        swa.event_mask = StructureNotifyMask;
        window->xwindow = XCreateWindow (display,
                                         RootWindow (display, vi->screen),
                                         0, 0, 640, 480, 0, vi->depth,
                                         InputOutput, vi->visual, valuemask,
                                         &swa);
        XStoreName (display, window->xwindow, "OpenGL Window");
        XMapWindow (display, window->xwindow);
        window->wm_delete_window = XInternAtom (display, "WM_DELETE_WINDOW",
                                                False);
        XSetWMProtocols (display, window->xwindow,
                         &window->wm_delete_window, 1);
    }
    return window;
}

/** Check that extension is in a list
 * @param ext_string list of extensions separated by a space
 * @param ext the extension to check in a list
 * @returns 1 if extension is present in extensions list, 0 otherwise
 */
static int is_extension_supported (const char *ext_string, const char *ext)
{
    const char *start = ext_string;
    const char *where = NULL;
    size_t ext_length = strlen (ext);
    while ((where = strstr (start, ext)) != NULL) {
        const char *end = where + ext_length;
        /* Check that is single word */
        if (((*end == ' ') || (*end == '\0'))
                && ((where == start) || * (where - 1) == ' ')) {
            return 1;
        }
        start = end;
    }
    return 0;
}

/** Print usage information */
static void print_usage (void)
{
    printf ("Usage: %s [OPTION]...\n"
            "Displays OpenGL animation in X11 window\n\n"
            "Options:\n"
            "  -h, --help     display this help and exit\n"
            "  -V, --version  output version information and exit\n"
            "\nReport bugs to: <" PACKAGE_BUGREPORT ">\n", program_name);
}

/** Run OpenGL application on a system with a GLX >= 1.3
 * @param display The display that where application should work
 * @param screen The number of screen where application should run
 * @returns exit code of application
 */
static int modern_run (Display *display, int screen)
{
    static int attribs[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
    int fbcount;
    GLXFBConfig *fbc = glXChooseFBConfig (display, screen, attribs, &fbcount);
    if (fbc == NULL) {
        fprintf (stderr, "%s: can't retrieve a framebuffer config\n",
                 program_name);
        return EXIT_FAILURE;
    }
    context_info.best_fbconfig = fbc[0];
    XFree (fbc);
    context_info.visual_info = glXGetVisualFromFBConfig (display,
                               context_info.best_fbconfig);
    if (context_info.visual_info == NULL) {
        fprintf (stderr, "%s: can't retrieve a visual\n", program_name);
        return EXIT_FAILURE;
    }
    main_window = window_create (display, context_info.visual_info);
    XFree (context_info.visual_info);

    context_info.glx_window = glXCreateWindow (display,
                              context_info.best_fbconfig, main_window->xwindow,
                              NULL);
    if (arb_create_context_extension != 0) {
        int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            None
        };
        context_info.context = glXCreateContextAttribsARB (display,
                               context_info.best_fbconfig, NULL, True,
                               context_attribs);
    } else {
        context_info.context = glXCreateNewContext (display,
                               context_info.best_fbconfig, GLX_RGBA_TYPE, NULL,
                               True);
    }

    if (context_info.context == NULL) {
        fprintf (stderr, "%s: can't create OpenGL context\n", program_name);
        glXDestroyWindow (display, context_info.glx_window);
        window_destroy (main_window);
        return EXIT_FAILURE;
    }

    glXMakeContextCurrent (display, context_info.glx_window,
                           context_info.glx_window, context_info.context);

    /*if (!game_init()) {
        glXMakeContextCurrent(display, 0, 0, 0);
        glXDestroyContext(display, context);
        glXDestroyWindow(display, glx_window);
        window_destroy(main_window);
        return EXIT_FAILURE;
    }
    */
    while (window_is_exists (main_window)) {
        window_process_events (main_window);
        /*game_tick();*/
        glXSwapBuffers (display, context_info.glx_window);
    }

    glXMakeContextCurrent (display, 0, 0, 0);
    glXDestroyContext (display, context_info.context);
    glXDestroyWindow (display, context_info.glx_window);
    window_destroy (main_window);
    return EXIT_SUCCESS;
}

/** Run OpenGL application on a system with a GLX version < 1.3
 * @param display The display that where application should work
 * @param screen The number of screen where application should run
 * @returns exit code of application
 */
static int legacy_run (Display *display, int screen)
{
    static int attribs[] = {
        GLX_USE_GL, True,
        GLX_RGBA, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
    context_info.visual_info = glXChooseVisual (display, screen, attribs);
    if (context_info.visual_info == NULL) {
        fprintf (stderr, "%s: can't retrieve a visual\n", program_name);
        return EXIT_FAILURE;
    }

    main_window = window_create (display, context_info.visual_info);
    context_info.context = glXCreateContext (display, context_info.visual_info,
                           NULL, True);
    XFree (context_info.visual_info);

    if (context_info.context == NULL) {
        fprintf (stderr, "%s: can't create OpenGL context\n", program_name);
        window_destroy (main_window);
        return EXIT_FAILURE;
    }
    glXMakeCurrent (display, main_window->xwindow, context_info.context);
    /*
        if (!game_init()) {
            glXMakeCurrent(display, 0, 0);
            glXDestroyContext(display, context);
            window_destroy(main_window);
            return EXIT_FAILURE;
        }
    */
    while (window_is_exists (main_window)) {
        window_process_events (main_window);
        /* game_tick(); */
        glXSwapBuffers (display, main_window->xwindow);
    }
    glXMakeCurrent (display, 0, 0);
    glXDestroyContext (display, context_info.context);
    window_destroy (main_window);
    return EXIT_SUCCESS;
}

/** Check GLX version and available extensions
 * @param display The display where application runs
 * @param screen the screen number where application runs
 * @returns 1 if minumum requred GLX version is available, 0 otherwise
 */
static int initialize_glx (Display *display, int screen)
{
    const char *extensions = NULL;

    /* Check the glx version */
    if (glXQueryVersion (display, &glx_major, &glx_minor) == False) {
        fprintf (stderr, "%s: glXQueryVersion fails\n", program_name);
        return 0;
    }
    if ((glx_major < 1) || ((glx_major == 1) && (glx_minor < 2))) {
        fprintf (stderr, "%s: atleast GLX 1.2 is required\n", program_name);
        return 0;
    }

    /* Check available GLX extensions */
    extensions = glXQueryExtensionsString (display, screen);

    if (is_extension_supported (extensions, "GLX_ARB_create_context") != 0) {
        glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
                                     glXGetProcAddressARB ((const GLubyte *)"glXCreateContextAttribsARB");
        arb_create_context_extension = glXCreateContextAttribsARB != NULL;
        arb_context_profile = is_extension_supported (extensions,
                              "GLX_ARB_create_context_profile");
    }
    return 1;
}

/** Parse command-line arguments
 * @param argc number of arguments passed to main()
 * @param argv array of arguments passed to main()
 */
static void parse_args (int argc, char *const *argv)
{
    int opt;
    program_name = argv[0];
    while ((opt = getopt_long (argc, argv, "hV", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage();
                exit (EXIT_SUCCESS);
            case 'V':
                printf ("%s\n", version_text);
                exit (EXIT_SUCCESS);
            default:
                print_usage();
                exit (EXIT_FAILURE);
        }
    }
}

int main (int argc, char *const *argv)
{
    int exit_code = EXIT_FAILURE;
    int screen = 0;
    Display *display = NULL;

    parse_args (argc, argv);

    display = XOpenDisplay (NULL);
    if (display == NULL) {
        fprintf (stderr, "%s: can't connect to X server\n", program_name);
        return EXIT_FAILURE;
    }

    screen = DefaultScreen (display);

    if (!initialize_glx (display, screen)) {
        XCloseDisplay (display);
        return EXIT_FAILURE;
    }

    if ((glx_major == 1) && (glx_minor == 2)) {
        exit_code = legacy_run (display, screen);
    } else if ((glx_major > 1) || ((glx_major == 1) && (glx_minor >= 3))) {
        exit_code = modern_run (display, screen);
    }

    XCloseDisplay (display);
    return exit_code;
}
