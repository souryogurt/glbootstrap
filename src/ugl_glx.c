#include "ugl.h"
#include <GL/glx.h>
#include <string.h>
#include <stdlib.h>

struct UGL {
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
    Display *display; /**< Connection to X11 display */
    int screen; /**< Screen number where UGL where created */
    int glx_major; /**< Major version of glx */
    int glx_minor; /**< Minor version of glx */
    int is_legacy; /**< Is we have legacy GLX version (GLX <=1.2) */
    int is_arb_context_profile; /**< Is GLX_ARB_create_context_profile there*/
    char padding[4];
};

struct UGLRenderSurface {
    GLXContext context; /**<Native OpenGL context */
    GLXDrawable drawable; /**<Native drawable */
};

void ugl_free (UGL *ugl)
{
    XCloseDisplay (ugl->display);
    free (ugl);
}

void ugl_free_framebuffer_config (const UGL *ugl, UGLFrameBufferConfig *config)
{
    if (ugl->is_legacy) {
        XFree (config);
    }
}

void ugl_free_render_surface (const UGL *ugl, UGLRenderSurface *surface)
{
    glXDestroyContext (ugl->display, surface->context);
    if (!ugl->is_legacy) {
        glXDestroyWindow (ugl->display, surface->drawable);
    }
    free (surface);
}

void ugl_swap_buffers (const UGL *ugl, const UGLRenderSurface *surface)
{
    glXSwapBuffers (ugl->display, surface->drawable);
}

int ugl_make_current (const UGL *ugl, const UGLRenderSurface *surface)
{
    GLXDrawable drawable = (GLXDrawable)NULL;
    GLXContext context = NULL;
    if (surface != NULL) {
        drawable = surface->drawable;
        context = surface->context;
    }
    /* Make context current */
    if (!ugl->is_legacy) {
        return glXMakeContextCurrent (ugl->display, drawable, drawable,
                                      context) == True;
    }
    return glXMakeCurrent (ugl->display, drawable, context) == True;
}

UGLRenderSurface *ugl_create_window_render_surface (const UGL *ugl,
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

int ugl_get_config_attribute (const UGL *ugl, UGLFrameBufferConfig *config,
                              enum UGLConfigAttribute attribute, void *value)
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

UGLFrameBufferConfig *ugl_choose_framebuffer_config (const UGL *ugl,
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
            GLX_DEPTH_SIZE, 16,
            GLX_STENCIL_SIZE, 8,
            GLX_DOUBLEBUFFER, True,
            None
        };
        config = (UGLFrameBufferConfig *)
                 glXChooseVisual (ugl->display, ugl->screen, legacy_attribs);
    }
    return config;
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

UGL *ugl_create (void *display_id)
{
    UGL *ugl;
    int major_version;
    int minor_version;

    Display *display = XOpenDisplay ((char *)display_id);
    if (display == NULL) {
        return NULL;
    }
    /* Check the glx version */
    if (glXQueryVersion (display, &major_version, &minor_version) == False) {
        XCloseDisplay (display);
        return NULL;
    }
    if ((major_version < 1) || ((major_version == 1) && (minor_version < 2))) {
        XCloseDisplay (display);
        return NULL;
    }

    ugl = (UGL *) malloc (sizeof (UGL));
    if (ugl != NULL) {
        const char *extensions;
        ugl->display = display;
        ugl->screen = DefaultScreen (display);
        ugl->glx_major = major_version;
        ugl->glx_minor = minor_version;
        ugl->is_legacy = ((minor_version == 2) && (major_version == 1));
        /* Check available GLX extensions */
        extensions = glXQueryExtensionsString (ugl->display, ugl->screen);

        if (is_extension_supported (extensions, "GLX_ARB_create_context") != 0) {
            ugl->glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
                                              glXGetProcAddressARB ((const GLubyte *)"glXCreateContextAttribsARB");
            ugl->is_arb_context_profile = is_extension_supported (extensions,
                                          "GLX_ARB_create_context_profile");
        } else {
            ugl->glXCreateContextAttribsARB = NULL;
            ugl->is_arb_context_profile = 0;
        }
    } else {
        XCloseDisplay (display);
    }

    return ugl;
}
