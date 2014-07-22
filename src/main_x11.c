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
#include <string.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <GL/gl.h>

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
 * @param d EGL display object
 * @param c EGL framebuffer configuration
 */
static void print_framebuffer_configuration (const EGLDisplay d, EGLConfig c)
{
    EGLBoolean bind_to_texture_rgb, bind_to_texture_rgba, native_renderable;
    EGLint buffer_size, red_size, green_size, blue_size, luminance_size,
           alpha_size, alpha_mask_size, color_buffer_type, config_caveat,
           config_id, conformant, depth_size, level, max_pbuffer_width,
           max_pbuffer_height, max_pbuffer_pixels, max_swap_interval,
           min_swap_interval, native_visual_id, native_visual_type,
           renderable_type, sample_buffers, samples, stencil_size, surface_type,
           transparent_type, transparent_red_value, transparent_green_value,
           transparent_blue_value;
    static const char *vnames[] = { "SG", "GS", "SC", "PC", "TC", "DC" };
    char string_buffer[100] = {0};
    eglGetConfigAttrib (d, c, EGL_BUFFER_SIZE, &buffer_size);
    eglGetConfigAttrib (d, c, EGL_RED_SIZE, &red_size);
    eglGetConfigAttrib (d, c, EGL_GREEN_SIZE, &green_size);
    eglGetConfigAttrib (d, c, EGL_BLUE_SIZE, &blue_size);
    eglGetConfigAttrib (d, c, EGL_LUMINANCE_SIZE, &luminance_size);
    eglGetConfigAttrib (d, c, EGL_ALPHA_SIZE, &alpha_size);
    eglGetConfigAttrib (d, c, EGL_ALPHA_MASK_SIZE, &alpha_mask_size);
    eglGetConfigAttrib (d, c, EGL_BIND_TO_TEXTURE_RGB,
                        (EGLint *)&bind_to_texture_rgb);
    eglGetConfigAttrib (d, c, EGL_BIND_TO_TEXTURE_RGBA,
                        (EGLint *)&bind_to_texture_rgba);
    eglGetConfigAttrib (d, c, EGL_COLOR_BUFFER_TYPE, &color_buffer_type);
    eglGetConfigAttrib (d, c, EGL_CONFIG_CAVEAT, &config_caveat);
    eglGetConfigAttrib (d, c, EGL_CONFIG_ID, &config_id);
    eglGetConfigAttrib (d, c, EGL_CONFORMANT, &conformant);
    eglGetConfigAttrib (d, c, EGL_DEPTH_SIZE, &depth_size);
    eglGetConfigAttrib (d, c, EGL_LEVEL, &level);
    eglGetConfigAttrib (d, c, EGL_MAX_PBUFFER_WIDTH, &max_pbuffer_width);
    eglGetConfigAttrib (d, c, EGL_MAX_PBUFFER_HEIGHT, &max_pbuffer_height);
    eglGetConfigAttrib (d, c, EGL_MAX_PBUFFER_PIXELS, &max_pbuffer_pixels);
    eglGetConfigAttrib (d, c, EGL_MAX_SWAP_INTERVAL, &max_swap_interval);
    eglGetConfigAttrib (d, c, EGL_MIN_SWAP_INTERVAL, &min_swap_interval);
    eglGetConfigAttrib (d, c, EGL_NATIVE_RENDERABLE,
                        (EGLint *)&native_renderable);
    eglGetConfigAttrib (d, c, EGL_NATIVE_VISUAL_ID, &native_visual_id);
    eglGetConfigAttrib (d, c, EGL_NATIVE_VISUAL_TYPE, &native_visual_type);
    eglGetConfigAttrib (d, c, EGL_RENDERABLE_TYPE, &renderable_type);
    eglGetConfigAttrib (d, c, EGL_SAMPLE_BUFFERS, &sample_buffers);
    eglGetConfigAttrib (d, c, EGL_SAMPLES, &samples);
    eglGetConfigAttrib (d, c, EGL_STENCIL_SIZE, &stencil_size);
    eglGetConfigAttrib (d, c, EGL_SURFACE_TYPE, &surface_type);
    eglGetConfigAttrib (d, c, EGL_TRANSPARENT_TYPE, &transparent_type);
    eglGetConfigAttrib (d, c, EGL_TRANSPARENT_RED_VALUE,
                        &transparent_red_value);
    eglGetConfigAttrib (d, c, EGL_TRANSPARENT_GREEN_VALUE,
                        &transparent_green_value);
    eglGetConfigAttrib (d, c, EGL_TRANSPARENT_BLUE_VALUE,
                        &transparent_blue_value);

    if (surface_type & EGL_WINDOW_BIT) {
        strcat (string_buffer, "win,");
    }
    if (surface_type & EGL_PBUFFER_BIT) {
        strcat (string_buffer, "pb,");
    }
    if (surface_type & EGL_PIXMAP_BIT) {
        strcat (string_buffer, "pix,");
    }
    if (strlen (string_buffer) > 0) {
        string_buffer[strlen (string_buffer) - 1] = 0;
    }

    printf ("0x%03x %3d %2d %2d %2d %2d %2d %2d %2d %2d%2d 0x%02x%s ",
            config_id, buffer_size, level,
            red_size, green_size, blue_size, alpha_size,
            depth_size, stencil_size,
            samples, sample_buffers, native_visual_id,
            native_visual_type < 6 ? vnames[native_visual_type] : "--");
    printf ("  %c  %c  %c  %c  %c   %c %s\n",
            (config_caveat != EGL_NONE) ? 'y' : ' ',
            (bind_to_texture_rgba) ? 'a' : (bind_to_texture_rgb) ? 'y' : ' ',
            (renderable_type & EGL_OPENGL_BIT) ? 'y' : ' ',
            (renderable_type & EGL_OPENGL_ES_BIT) ? 'y' : ' ',
            (renderable_type & EGL_OPENGL_ES2_BIT) ? 'y' : ' ',
            (renderable_type & EGL_OPENVG_BIT) ? 'y' : ' ',
            string_buffer);
}

/** Print full list of available configurations
 * @param egl_display EGL display where configurations should be retreived
 */
static void print_available_configurations (const EGLDisplay egl_display)
{
    EGLint n_configs = 0;
    EGLConfig *configs = NULL;
    static const EGLint egl_attributes[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };
    if (eglGetConfigs (egl_display, NULL, 0, &n_configs) == EGL_FALSE) {
        return;
    }
    configs = (EGLConfig *)calloc ((size_t)n_configs, sizeof (EGLConfig));
    if (configs) {
        eglChooseConfig (egl_display, egl_attributes, configs, n_configs, &n_configs);
        printf ("Configurations:\n");
        printf ("      bf  lv colorbuffer dp st  ms    vis   cav bi  renderable  supported\n");
        printf ("  id  sz   l  r  g  b  a th cl ns b    id   eat nd gl es es2 vg surfaces \n");
        printf ("-------------------------------------------------------------------------\n");
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
    if ((err != EGL_TRUE) || (n_configs == 0)) {
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
        printf ("Selected configuration:\n");
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
        eglDestroySurface (egl_display, window_surface);
        window_destroy (main_window);
        eglTerminate (egl_display);
        return EXIT_FAILURE;
    }
    printf ("OpenGL %s\n", glGetString (GL_VERSION));
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
