/**
 * @file ugl_glx.c
 * Implementation of universal opengl context creation interface using GLX.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <GL/glx.h>
#include "ugl.h"

/** GLX implementation of UGL type */
struct UGL {
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
    Display *display; /**< Connection to X11 display */
    int screen; /**< Screen number where UGL where created */
    int glx_major; /**< Major version of glx */
    int glx_minor; /**< Minor version of glx */
    int is_modern; /**< Is we have modern GLX version (GLX > 1.2) */
    int is_arb_context_profile; /**< Is GLX_ARB_create_context_profile there*/
    char padding[4];
};

/** GLX implementation of UGLRenderSurface */
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
    if (!ugl->is_modern) {
        XFree (config);
    }
}

void ugl_free_render_surface (const UGL *ugl, UGLRenderSurface *surface)
{
    glXDestroyContext (ugl->display, surface->context);
    if (ugl->is_modern) {
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
    if (ugl->is_modern) {
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
    if (ugl->is_modern) {
        GLXFBConfig glx_config = (GLXFBConfig)config;
        /* GLX1.3 requires to create GLXWindow associated with window */
        drawable = glXCreateWindow (ugl->display, glx_config, window, NULL);
        /* First try to create OpenGL context using GLX_ARB_create_context
         * extension */
        if (ugl->glXCreateContextAttribsARB) {
            int context_attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                None
            };
            context = ugl->glXCreateContextAttribsARB (ugl->display, glx_config,
                      NULL, True, context_attribs);
        } else {
            /* If theare no GLX_ARB_create_context extension, then fall back to
             * standard GLX1.3 way */
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

/** Convert UGL framebuffer attribute to GLX attribute
 * @param attribute UGL framebuffer attribute
 * @returns GLX framebuffer attribute
 */
static int ugl_attribute_convert_to_glx (unsigned int attribute)
{
    switch (attribute) {
        case UGL_RED_SIZE:
            return GLX_RED_SIZE;
        case UGL_GREEN_SIZE:
            return GLX_GREEN_SIZE;
        case UGL_BLUE_SIZE:
            return GLX_BLUE_SIZE;
        case UGL_ALPHA_SIZE:
            return GLX_ALPHA_SIZE;
        case UGL_DEPTH_SIZE:
            return GLX_DEPTH_SIZE;
        case UGL_STENCIL_SIZE:
            return GLX_STENCIL_SIZE;
        default:
            return -1;
    }
}

int ugl_get_config_attribute (const UGL *ugl, UGLFrameBufferConfig *config,
                              unsigned int attribute, void *value)
{
    int native_attribute = -1;
    if (attribute == UGL_NATIVE_VISUAL_ID) {
        if (ugl->is_modern) {
            GLXFBConfig fb_config = (GLXFBConfig)config;
            return glXGetFBConfigAttrib (ugl->display, fb_config, GLX_VISUAL_ID,
                                         (int *) value) == Success;
        }
        * ((VisualID *)value) = ((XVisualInfo *)config)->visualid;
        return 1;
    }

    native_attribute = ugl_attribute_convert_to_glx (attribute);

    if (ugl->is_modern) {
        GLXFBConfig fb_config = (GLXFBConfig)config;
        return glXGetFBConfigAttrib (ugl->display, fb_config, native_attribute,
                                     (int *) value) == Success;
    }
    return glXGetConfig (ugl->display, (XVisualInfo *)config, native_attribute,
                         (int *) value) == Success;
}

/** Default attributes for GLX1.3 framebuffer */
static int default_fb_attrs[] = {
    GLX_X_RENDERABLE, True,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DOUBLEBUFFER, True
};

/** Default attributes for GLX1.2 visual */
static int legacy_attribs[] = {
    GLX_USE_GL, True,
    GLX_RGBA, True,
    GLX_DOUBLEBUFFER, True,
};

#define MAX_NUMBER_OF_DEFAULT_ATTRIBUTES 4

/** Convert array of UGL attributes to array of GLX FB attributes
 * @param fb_attribute output array
 * @param attributes array of ugl attributes
 * @returns non-zero if all attributes have converted, 0 otherwise
 */
static int ugl_convert_to_fb_attributes (int *fb_attribute,
        const int *attributes)
{
    size_t index = 0;
    for (index = 0; attributes[index] != None; index += 2) {
        switch (attributes[index]) {
            case UGL_RED_SIZE:
                fb_attribute[index] = GLX_RED_SIZE;
                break;
            case UGL_GREEN_SIZE:
                fb_attribute[index] = GLX_GREEN_SIZE;
                break;
            case UGL_BLUE_SIZE:
                fb_attribute[index] = GLX_BLUE_SIZE;
                break;
            case UGL_ALPHA_SIZE:
                fb_attribute[index] = GLX_ALPHA_SIZE;
                break;
            case UGL_DEPTH_SIZE:
                fb_attribute[index] = GLX_DEPTH_SIZE;
                break;
            case UGL_STENCIL_SIZE:
                fb_attribute[index] = GLX_STENCIL_SIZE;
                break;
            default:
                return 0;
        };
        fb_attribute[index + 1] = attributes[index + 1];
    }
    return 1;
}

UGLFrameBufferConfig *ugl_choose_framebuffer_config (const UGL *ugl,
        const int *attributes)
{
    UGLFrameBufferConfig *config = NULL;
    int fb_attributes[ (MAX_NUMBER_OF_DEFAULT_ATTRIBUTES +
                        UGL_NUMBER_OF_ATTRIBUTES) * 2] = {None};
    if (ugl->is_modern) {
        int fbcount = 0;
        GLXFBConfig *fbc = NULL;
        memcpy (fb_attributes, default_fb_attrs, sizeof (default_fb_attrs));
        ugl_convert_to_fb_attributes (&fb_attributes[8], attributes);
        fbc = glXChooseFBConfig (ugl->display, ugl->screen, fb_attributes,
                                 &fbcount);
        if (fbc != NULL) {
            config = (UGLFrameBufferConfig *) fbc[0];
            XFree (fbc);
        }
    } else {
        memcpy (fb_attributes, legacy_attribs, sizeof (legacy_attribs));
        ugl_convert_to_fb_attributes (&fb_attributes[6], attributes);
        config = (UGLFrameBufferConfig *)
                 glXChooseVisual (ugl->display, ugl->screen, fb_attributes);
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
    UGL *ugl = NULL;
    int major_version = 0;
    int minor_version = 0;

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
    if (ugl) {
        const char *extensions = NULL;
        memset (ugl, 0, sizeof (UGL));
        ugl->display = display;
        ugl->screen = DefaultScreen (display);
        ugl->glx_major = major_version;
        ugl->glx_minor = minor_version;
        ugl->is_modern = ((major_version == 1) && (minor_version > 2)) ||
                         (major_version > 1);
        /* Check available GLX extensions */
        extensions = glXQueryExtensionsString (ugl->display, ugl->screen);

        if (is_extension_supported (extensions, "GLX_ARB_create_context")) {
            ugl->glXCreateContextAttribsARB =
                (PFNGLXCREATECONTEXTATTRIBSARBPROC)
                glXGetProcAddressARB ((const GLubyte *)"glXCreateContextAttribsARB");
            ugl->is_arb_context_profile = is_extension_supported (extensions,
                                          "GLX_ARB_create_context_profile");
        }
    } else {
        XCloseDisplay (display);
    }
    return ugl;
}
