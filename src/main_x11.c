#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#define PACKAGE_STRING "glbootstrap 0.1"
#define PACKAGE_BUGREPORT "egor.artemov@gmail.com"

typedef struct UGL {
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
    Display *display;
    int screen;
    /** Major version of glx */
    int glx_major;
    /** Minor version of glx */
    int glx_minor;
    /** Flag that indicates that we have legacy GLX version (GLX <=1.2) */
    int is_legacy;
    /** Flag that indicates that GLX_ARB_create_context_profile is implemented*/
    int is_arb_context_profile;
    char padding[4];
} UGL;

typedef void UGLFrameBufferConfig;
typedef Window UGLNativeWindow;

typedef struct UGLRenderSurface {
    GLXContext context;
    GLXDrawable drawable;
} UGLRenderSurface;

enum UGLConfigAttribute {
    UGL_NATIVE_VISUAL_ID
};

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

/** Get native window handle of game window
 * @param window target game window object
 */
static Window window_get_native (game_window_t *window)
{
    return window->xwindow;
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

/** Frees universal OpenGL interface object
 * @param ugl pointer to the object
 */
static void ugl_free (UGL *ugl)
{
    free (ugl);
}

/** Frees universal framebuffer configuration
 * @param ugl pointer to UGL interface
 * @param config Configuration to be freed
 */
static void ugl_free_framebuffer_config (const UGL *ugl,
        UGLFrameBufferConfig *config)
{
    if (ugl->is_legacy) {
        XFree (config);
    }
}

/** Frees rendering surface
 * @param ugl pointer to universal OpenGL interface object
 * @param surface pointer to render surface object
 */
static void ugl_free_render_surface (const UGL *ugl, UGLRenderSurface *surface)
{
    glXDestroyContext (ugl->display, surface->context);
    if (!ugl->is_legacy) {
        glXDestroyWindow (ugl->display, surface->drawable);
    }
    free (surface);
}

/** Swap front and back buffers on a surface
 * @param ugl pointer to universal OpenGL interface object
 * @param surface pointer to render surface object
 */
static void ugl_swap_buffers (const UGL *ugl, const UGLRenderSurface *surface)
{
    glXSwapBuffers (ugl->display, surface->drawable);
}

/** Make render surface to be current for OpenGL API
 * @param ugl pointer to universal OpenGL interface object
 * @param surface pointer to render surface object
 * @returns non-zero on success, 0 otherwise
 */
static int ugl_make_current (const UGL *ugl, const UGLRenderSurface *surface)
{
    GLXDrawable drawable = (GLXDrawable)NULL;
    GLXContext context = NULL;
    if (surface != NULL) {
        drawable = surface->drawable;
        context = surface->context;
    }
    /* Make context current */
    if (!ugl->is_legacy) {
        glXMakeContextCurrent (ugl->display, drawable, drawable, context);
    } else {
        glXMakeCurrent (ugl->display, drawable, context);
    }
    return 1;
}

/** Create render surface assosiated with window
 * @param ugl pointer to universal OpenGL interface object
 * @param config pointer to framebuffer configuration object
 * @param window native window handle
 * @returns new render surface object, NULL otherwise
 */
static UGLRenderSurface *ugl_create_window_render_surface (const UGL *ugl,
        UGLFrameBufferConfig *config, UGLNativeWindow window)
{
    GLXDrawable drawable = (GLXDrawable)NULL;
    GLXContext context = NULL;
    UGLRenderSurface *surface = NULL;
    /* Creating openGL context */
    if (!ugl->is_legacy) {
        GLXFBConfig glx_config = (GLXFBConfig)config;
        drawable = glXCreateWindow (ugl->display, glx_config, window, NULL);
        if (ugl->glXCreateContextAttribsARB) {
            int context_attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                None
            };
            context = ugl->glXCreateContextAttribsARB (ugl->display, glx_config,
                      NULL, True, context_attribs);
        } else {
            context = glXCreateNewContext (ugl->display, glx_config,
                                           GLX_RGBA_TYPE, NULL, True);
        }
        if (context == NULL) {
            glXDestroyWindow (ugl->display, drawable);
        }
    } else {
        context = glXCreateContext (ugl->display, (XVisualInfo *) config,
                                    NULL, True);
        drawable = (GLXDrawable) window;
    }
    if (context != NULL) {
        surface = (UGLRenderSurface *) malloc (sizeof (UGLRenderSurface));
        if (surface != NULL) {
            surface->context = context;
            surface->drawable = drawable;
        }
    }
    return surface;
}

/** Get framebuffer configuration's attribute
 * @param ugl pointer to universal OpenGL interface object
 * @param config pointer to framebuffer configuration object
 * @param attribute UGL attribute
 * @param value pointer to value storage
 * @returns non-zero if attribute retreived succesfully, 0 otherwise
 */
static int ugl_get_config_attribute (const UGL *ugl,
                                     UGLFrameBufferConfig *config,
                                     enum UGLConfigAttribute attribute,
                                     void *value)
{
    if (attribute == UGL_NATIVE_VISUAL_ID) {
        XVisualInfo *info = NULL;
        VisualID *id = (VisualID *)value;
        if (!ugl->is_legacy) {
            info = glXGetVisualFromFBConfig (ugl->display, (GLXFBConfig)config);
            if (info != NULL) {
                *id = info->visualid;
                XFree (info);
            } else {
                return 0;
            }
        } else {
            info = (XVisualInfo *)config;
            *id = info->visualid;
        }
        return 1;
    }
    return 0;
}

/** Get framebuffer configuration with specific attributes
 * @param ugl pointer to universal OpenGL interface object
 * @param attributes the array of GLX 1.3 attributes
 * @returns framebuffer configuration object
 */
static UGLFrameBufferConfig *ugl_get_framebuffer_config (const UGL *ugl,
        const int *attributes)
{
    UGLFrameBufferConfig *config = NULL;
    int fbcount = 0;
    if (!ugl->is_legacy) {
        GLXFBConfig *fbc = NULL;
        fbc = glXChooseFBConfig (ugl->display, ugl->screen, attributes,
                                 &fbcount);
        if (fbc != NULL) {
            config = (UGLFrameBufferConfig *) fbc[0];
            XFree (fbc);
        }
    } else {
        /**TODO: convert attribs to GLX1.2 attribs list */
        static int legacy_attribs[] = {
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
        config = (UGLFrameBufferConfig *)
                 glXChooseVisual (ugl->display, ugl->screen, legacy_attribs);
    }
    return config;
}

/** Creates universal interface to OpenGL subsystem
 * @param display The display where application runs
 * @param screen the screen number where application runs
 * @returns interface object, NULL otherwise
 */
static UGL *ugl_create (Display *display, int screen)
{
    UGL *ugl;
    int major_version;
    int minor_version;
    /* Check the glx version */
    if (glXQueryVersion (display, &major_version, &minor_version) == False) {
        return NULL;
    }
    if ((major_version < 1) || ((major_version == 1) && (minor_version < 2))) {
        return NULL;
    }

    ugl = (UGL *) malloc (sizeof (UGL));
    if (ugl != NULL) {
        const char *extensions;
        ugl->display = display;
        ugl->screen = screen;
        ugl->glx_major = major_version;
        ugl->glx_minor = minor_version;
        ugl->is_legacy = ((minor_version == 2) && (major_version == 1));
        /* Check available GLX extensions */
        extensions = glXQueryExtensionsString (display, screen);

        if (is_extension_supported (extensions, "GLX_ARB_create_context") != 0) {
            ugl->glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
                                              glXGetProcAddressARB ((const GLubyte *)"glXCreateContextAttribsARB");
            ugl->is_arb_context_profile = is_extension_supported (extensions,
                                          "GLX_ARB_create_context_profile");
        } else {
            ugl->glXCreateContextAttribsARB = NULL;
            ugl->is_arb_context_profile = 0;
        }
    }

    return ugl;
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
    static const int attribs[] = {
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
    int screen = 0;
    UGL *ugl = NULL;
    UGLFrameBufferConfig *ugl_config = NULL;
    UGLRenderSurface *surface = NULL;
    VisualID visual_id;
    XVisualInfo *visual_info = NULL;
    XVisualInfo visual_info_template;
    int num_visuals;
    Display *display = NULL;

    parse_args (argc, argv);

    display = XOpenDisplay (NULL);
    if (display == NULL) {
        fprintf (stderr, "%s: can't connect to X server\n", program_name);
        return EXIT_FAILURE;
    }

    screen = DefaultScreen (display);

    if ((ugl = ugl_create (display, screen)) == NULL) {
        XCloseDisplay (display);
        return EXIT_FAILURE;
    }
    if ((ugl_config = ugl_get_framebuffer_config (ugl, attribs)) == NULL) {
        fprintf (stderr, "%s: can't retrieve a framebuffer config\n",
                 program_name);
        ugl_free (ugl);
        return EXIT_FAILURE;
    }
    if (ugl_get_config_attribute (ugl, ugl_config, UGL_NATIVE_VISUAL_ID,
                                  &visual_id)) {
        memset (&visual_info_template, 0, sizeof (visual_info_template));
        visual_info_template.visualid = visual_id;
        visual_info = XGetVisualInfo (display, VisualIDMask,
                                      &visual_info_template, &num_visuals);
    }
    if (visual_info == NULL) {
        fprintf (stderr, "%s: can't retrieve a visual\n", program_name);
        ugl_free_framebuffer_config (ugl, ugl_config);
        ugl_free (ugl);
        return EXIT_FAILURE;
    }

    main_window = window_create (display, visual_info);
    XFree (visual_info);

    surface = ugl_create_window_render_surface (ugl, ugl_config,
              (UGLNativeWindow)window_get_native (main_window));
    if (surface == NULL) {
        fprintf (stderr, "%s: can't create rendering surface\n", program_name);
        window_destroy (main_window);
        ugl_free_framebuffer_config (ugl, ugl_config);
        ugl_free (ugl);
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
