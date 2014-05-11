/**
 * @file main_x11.c
 * This module contains entry point and initialization for X11 variant
 * of application.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>

/** Window type */
typedef struct game_window_t {
    Display *display; /**< X11 connection for this window */
    Atom wm_delete_window; /**< Atom to receive "window closed" message */
    Window xwindow; /**< Native X11 window */
    int is_closed; /**< true if window is closed */
    int width; /**< Width of window's client area */
    int height; /**< Height of window's client area */
    char padding[4];
} game_window_t;

/** Single application's main window */
static game_window_t *main_window = NULL;

/** The name the program was run with */
static const char *program_name;

/** Flag that indicates to be verbose as possible */
static int verbose = 0;

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
    {"verbose", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};


/** Print attribute of framebuffer configuration as unsigned int
 * @param egl_display EGL display object
 * @param config EGL framebuffer configuration
 * @param attribute framebuffer's attribute to print
 * @param attribute_name human readable name of attribute
 */
static void print_framebuffer_attribute_int (const EGLDisplay egl_display,
        EGLConfig config, EGLint attribute,
        const char *attribute_name)
{
    EGLint value;
    if (eglGetConfigAttrib (egl_display, config, attribute, &value) == EGL_TRUE) {
        printf ("  %s:\t %u\n", attribute_name, (unsigned int) value);
    } else {
        printf ("  %s:\t Unknown\n", attribute_name);
    }
}

/** Print usage information */
static void print_usage (void)
{
    printf ("Usage: %s [OPTION]...\n"
            "Displays OpenGL animation in X11 window\n\n"
            "Options:\n"
            "  -h, --help     display this help and exit\n"
            "  -V, --version  output version information and exit\n"
            "  --verbose      be verbose\n"
            "\nReport bugs to: <" PACKAGE_BUGREPORT ">\n", program_name);
}

/** Process all pending events
 * @param window events of this window should be processed
 */
static void window_process_events (game_window_t *window)
{
    int n_events = XPending (window->display);
    while (n_events > 0) {
        XEvent event;
        XNextEvent (window->display, &event);
        if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == window->wm_delete_window) {
                window->is_closed = 1;
            }
        } else if (event.type == ConfigureNotify) {
            XConfigureEvent xce;
            xce = event.xconfigure;
            if ((xce.width != window->width)
                    || (xce.height != window->height)) {
                window->width = xce.width;
                window->height = xce.height;
                /*game_resize (window->width, window->height);*/
            }
        }
        n_events--;
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
        free (window);
    }
}

/** Get native window handle of game window
 * @param window target game window object
 */
static Window window_get_native (game_window_t *window)
{
    return window->xwindow;
}

/** Create and display new window
 * @param display The display where window should be created
 * @param caption The caption of window in Host Portable Character Encoding
 * @param width The width of the window's client area
 * @param height The height of the window's client area
 * @param visual_id VisualID of Visual that should be used to create a window
 * @returns new window object, NULL otherwise
 */
static game_window_t *window_create (Display *display, const char *caption,
                                     unsigned int width, unsigned int height,
                                     VisualID visual_id)
{
    int n_visuals;
    XVisualInfo *info = NULL;
    game_window_t *window = NULL;
    XVisualInfo info_template;
    info_template.visualid = visual_id;
    info = XGetVisualInfo (display, VisualIDMask, &info_template, &n_visuals);
    if (info == NULL) {
        return NULL;
    }

    window = (game_window_t *)malloc (sizeof (game_window_t));
    if (window != NULL) {
        const unsigned long attributes_mask = CWBorderPixel | CWColormap |
                                              CWEventMask;
        XSetWindowAttributes window_attributes;
        Window root = RootWindow (display, info->screen);
        window->display = display;
        window->is_closed = 0;
        window->width = 0;
        window->height = 0;
        window_attributes.colormap = XCreateColormap (display, root,
                                     info->visual, AllocNone);
        window_attributes.background_pixmap = None;
        window_attributes.border_pixel = 0;
        window_attributes.event_mask = StructureNotifyMask;
        window->xwindow = XCreateWindow (display, root, 0, 0, width, height,
                                         0, info->depth, InputOutput,
                                         info->visual, attributes_mask,
                                         &window_attributes);
        XStoreName (display, window->xwindow, caption);
        XMapWindow (display, window->xwindow);
        window->wm_delete_window = XInternAtom (display, "WM_DELETE_WINDOW",
                                                False);
        XSetWMProtocols (display, window->xwindow,
                         &window->wm_delete_window, 1);
    }
    XFree (info);
    return window;
}

/** Print configuration of selected framebuffer
 * @param egl_display EGL display object
 * @param config EGL framebuffer configuration
 */
static void print_framebuffer_configuration (const EGLDisplay egl_display,
        EGLConfig config)
{
    EGLint value;
    printf ("Framebuffer configuration:\n");
    if (eglGetConfigAttrib (egl_display, config, EGL_NATIVE_VISUAL_ID,
                            &value) == EGL_TRUE) {
        printf ("  VisualID:\t 0x%03X\n", value);
    } else {
        printf ("  VisualID:\t Unknown\n");
    }
    print_framebuffer_attribute_int (egl_display, config, EGL_RED_SIZE,
                                     "Red Size");
    print_framebuffer_attribute_int (egl_display, config, EGL_GREEN_SIZE,
                                     "Green Size");
    print_framebuffer_attribute_int (egl_display, config, EGL_BLUE_SIZE,
                                     "Blue Size");
    print_framebuffer_attribute_int (egl_display, config, EGL_ALPHA_SIZE,
                                     "Alpha Size");
    print_framebuffer_attribute_int (egl_display, config, EGL_DEPTH_SIZE,
                                     "Depth Size");
    print_framebuffer_attribute_int (egl_display, config, EGL_STENCIL_SIZE,
                                     "Stencil Size");
}

/** Print full list of available configurations
 * @param egl_display EGL display where configurations should be retreived
 */
static void print_available_configurations (const EGLDisplay egl_display)
{
    EGLint n_configs = 0;
    EGLConfig *configs = NULL;
    if (eglGetConfigs (egl_display, NULL, 0, &n_configs) == EGL_FALSE) {
        return;
    }
    configs = (EGLConfig *)calloc ((size_t)n_configs, sizeof (EGLConfig));
    if (configs) {
        eglGetConfigs (egl_display, configs, n_configs, &n_configs);
        printf ("Available configurations:\n");
        while (n_configs > 0) {
            n_configs--;
            print_framebuffer_configuration (egl_display, configs[n_configs]);
        }
        free (configs);
    }
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
                print_usage ();
                exit (EXIT_SUCCESS);
            case 'V':
                printf ("%s\n", version_text);
                exit (EXIT_SUCCESS);
            case 'v':
                verbose = 1;
                break;
            default:
                print_usage ();
                exit (EXIT_FAILURE);
        }
    }
}

int main (int argc, char *const *argv)
{
    static const EGLint egl_attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_NATIVE_RENDERABLE, EGL_TRUE,
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
    VisualID visual_id = 0;
    Display *display = NULL;

    parse_args (argc, argv);

    egl_display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf (stderr, "%s: no matching EGL_DEFAULT_DISPLAY is available\n",
                 program_name);
        return EXIT_FAILURE;
    }
    if (eglInitialize (egl_display, &egl_major, &egl_minor) != EGL_TRUE) {
        fprintf (stderr, "%s: can't initialize EGL on a display\n",
                 program_name);
        return EXIT_FAILURE;
    }

    if (verbose) {
        print_available_configurations (egl_display);
    }

    err = eglChooseConfig (egl_display, egl_attributes, &config, 1, &n_configs);
    if (err != EGL_TRUE) {
        fprintf (stderr, "%s: can't retrieve a framebuffer config\n",
                 program_name);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }
    err = eglGetConfigAttrib (egl_display, config, EGL_NATIVE_VISUAL_ID,
                              (EGLint *)&visual_id);
    if (err == EGL_FALSE) {
        fprintf (stderr, "%s: can't retrieve a visual\n", program_name);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }

    if (verbose) {
        print_framebuffer_configuration (egl_display, config);
    }

    display = XOpenDisplay (NULL);
    if (display == NULL) {
        fprintf (stderr, "%s: can't connect to X server\n", program_name);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }

    main_window = window_create (display, "OpenGL Window", 640, 480, visual_id);
    if (main_window == NULL) {
        fprintf (stderr, "%s: can't create game window\n", program_name);
        XCloseDisplay (display);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }

    err = eglBindAPI (EGL_OPENGL_API);
    if (err != EGL_TRUE) {
        fprintf (stderr, "%s: can't bind OpenGL API\n", program_name);
        window_destroy (main_window);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }
    context = eglCreateContext (egl_display, config, EGL_NO_CONTEXT, NULL);
    if (context == EGL_NO_CONTEXT) {
        fprintf (stderr, "%s: can't create OpenGL context\n", program_name);
        window_destroy (main_window);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }
    window_surface = eglCreateWindowSurface (egl_display, config,
                     (NativeWindowType) window_get_native (main_window), NULL);
    if (window_surface == EGL_NO_SURFACE) {
        fprintf (stderr, "%s: can't create rendering surface\n", program_name);
        eglDestroyContext (egl_display, context);
        window_destroy (main_window);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }
    err = eglMakeCurrent (egl_display, window_surface, window_surface, context);
    if (err == EGL_FALSE) {
        fprintf (stderr, "%s: can't make OpenGL context be current\n",
                 program_name);
        eglDestroyContext (egl_display, context);
        window_destroy (main_window);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }

    while (window_is_exists (main_window)) {
        window_process_events (main_window);
        /*game_tick();*/
        eglSwapBuffers (egl_display, window_surface);
    }
    eglMakeCurrent (egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface (egl_display, window_surface);
    eglDestroyContext (egl_display, context);
    window_destroy (main_window);
    XCloseDisplay (display);
    eglTerminate (egl_display);
    return EXIT_SUCCESS;
}
