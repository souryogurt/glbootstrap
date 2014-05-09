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
#include "ugl.h"

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
 * @param ugl pointer to universal OpenGL interface object
 * @param ugl_config pointer to framebuffer configuration object
 * @param attribute framebuffer's attribute to print
 * @param attribute_name human readable name of attribute
 */
static void print_framebuffer_attribute_int (const UGL *ugl,
        UGLFrameBufferConfig *ugl_config, unsigned int attribute,
        const char *attribute_name)
{
    unsigned int value;
    if (ugl_get_config_attribute (ugl, ugl_config, attribute, &value) ) {
        printf ("  %s:\t %u\n", attribute_name, value);
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
 * @param ugl pointer to universal OpenGL interface object
 * @param ugl_config pointer to framebuffer configuration object
 */
static void print_framebuffer_configuration (const UGL *ugl,
        UGLFrameBufferConfig *ugl_config)
{
    int success;
    unsigned int value;
    printf ("Framebuffer configuration:\n");
    success = ugl_get_config_attribute (ugl, ugl_config, UGL_NATIVE_VISUAL_ID,
                                        &value);
    if (success) {
        printf ("  VisualID:\t 0x%03X\n", value);
    } else {
        printf ("  VisualID:\t Unknown\n");
    }
    print_framebuffer_attribute_int (ugl, ugl_config, UGL_RED_SIZE, "Red Size");
    print_framebuffer_attribute_int (ugl, ugl_config, UGL_GREEN_SIZE,
                                     "Green Size");
    print_framebuffer_attribute_int (ugl, ugl_config, UGL_BLUE_SIZE,
                                     "Blue Size");
    print_framebuffer_attribute_int (ugl, ugl_config, UGL_ALPHA_SIZE,
                                     "Alpha Size");
    print_framebuffer_attribute_int (ugl, ugl_config, UGL_DEPTH_SIZE,
                                     "Depth Size");
    print_framebuffer_attribute_int (ugl, ugl_config, UGL_STENCIL_SIZE,
                                     "Stencil Size");
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
    static const int attribs[] = {
        UGL_RED_SIZE, 8,
        UGL_GREEN_SIZE, 8,
        UGL_BLUE_SIZE, 8,
        UGL_ALPHA_SIZE, 8,
        UGL_DEPTH_SIZE, 16,
        UGL_STENCIL_SIZE, 8,
        None
    };
    UGL *ugl = NULL;
    UGLFrameBufferConfig *ugl_config = NULL;
    UGLRenderSurface *surface = NULL;
    VisualID visual_id = 0;
    Display *display = NULL;

    parse_args (argc, argv);

    if ((ugl = ugl_create (NULL)) == NULL) {
        return EXIT_FAILURE;
    }

    if ((ugl_config = ugl_choose_framebuffer_config (ugl, attribs)) == NULL) {
        fprintf (stderr, "%s: can't retrieve a framebuffer config\n",
                 program_name);
        ugl_free (ugl);
        return EXIT_FAILURE;
    }
    if (!ugl_get_config_attribute (ugl, ugl_config, UGL_NATIVE_VISUAL_ID,
                                   &visual_id)) {
        fprintf (stderr, "%s: can't retrieve a visual\n", program_name);
        ugl_free_framebuffer_config (ugl, ugl_config);
        ugl_free (ugl);
        return EXIT_FAILURE;
    }

    if (verbose) {
        print_framebuffer_configuration (ugl, ugl_config);
    }

    display = XOpenDisplay (NULL);
    if (display == NULL) {
        fprintf (stderr, "%s: can't connect to X server\n", program_name);
        return EXIT_FAILURE;
    }

    main_window = window_create (display, "OpenGL Window", 640, 480, visual_id);
    if (main_window == NULL) {
        fprintf (stderr, "%s: can't create game window\n", program_name);
        ugl_free_framebuffer_config (ugl, ugl_config);
        ugl_free (ugl);
        XCloseDisplay (display);
        return EXIT_FAILURE;
    }

    surface = ugl_create_window_render_surface (ugl, ugl_config,
              (UGLNativeWindow)window_get_native (main_window));
    if (surface == NULL) {
        fprintf (stderr, "%s: can't create rendering surface\n", program_name);
        window_destroy (main_window);
        ugl_free_framebuffer_config (ugl, ugl_config);
        ugl_free (ugl);
        XCloseDisplay (display);
        return EXIT_FAILURE;
    }
    ugl_make_current (ugl, surface);

    while (window_is_exists (main_window)) {
        window_process_events (main_window);
        /*game_tick();*/
        ugl_swap_buffers (ugl, surface);
    }
    ugl_make_current (ugl, NULL);
    ugl_free_render_surface (ugl, surface);
    window_destroy (main_window);
    ugl_free_framebuffer_config (ugl, ugl_config);
    ugl_free (ugl);
    XCloseDisplay (display);
    return EXIT_SUCCESS;
}
