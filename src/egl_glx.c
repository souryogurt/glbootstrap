/**
 * @file egl_glx.c
 * Implementation of EGL using GLX.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <GL/glx.h>
#include <EGL/egl.h>

#ifndef GLX_ARB_create_context
#define GLX_ARB_create_context 1
#define GLX_CONTEXT_DEBUG_BIT_ARB         0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB     0x2092
#define GLX_CONTEXT_FLAGS_ARB             0x2094
typedef GLXContext ( *PFNGLXCREATECONTEXTATTRIBSARBPROC) (Display *dpy,
        GLXFBConfig config, GLXContext share_context, Bool direct,
        const int *attrib_list);
#endif /* GLX_ARB_create_context */

typedef struct EGL_GLXConfig {
    EGLint buffer_size;
    EGLint red_size;
    EGLint green_size;
    EGLint blue_size;
    EGLint luminance_size;
    EGLint alpha_size;
    EGLint alpha_mask_size;
    EGLBoolean bind_to_texture_rgb;
    EGLBoolean bind_to_texture_rgba;
    EGLint color_buffer_type;
    EGLint config_caveat;
    EGLint config_id;
    EGLint conformant;
    EGLint depth_size;
    EGLint level;
    EGLint max_pbuffer_width;
    EGLint max_pbuffer_height;
    EGLint max_pbuffer_pixels;
    EGLint max_swap_interval;
    EGLint min_swap_interval;
    EGLBoolean native_renderable;
    EGLint native_visual_id;
    EGLint native_visual_type;
    EGLint renderable_type;
    EGLint sample_buffers;
    EGLint samples;
    EGLint stencil_size;
    EGLint surface_type;
    EGLint transparent_type;
    EGLint transparent_red_value;
    EGLint transparent_green_value;
    EGLint transparent_blue_value;
} EGL_GLXConfig;

typedef struct EGL_GLXDisplay {
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
    Display *x11_display; /**< Connection to X11 display */
    EGL_GLXConfig *configs;
    EGLint n_configs;
    int screen; /**< Screen number where UGL where created */
    int glx_major; /**< Major version of glx */
    int glx_minor; /**< Minor version of glx */
    int is_modern; /**< Is we have modern GLX version (GLX > 1.2) */
    int is_arb_context_profile; /**< Is GLX_ARB_create_context_profile there*/
} EGL_GLXDisplay;

#define DISPLAY_TABLE_SIZE 1
static EGL_GLXDisplay *display_table[DISPLAY_TABLE_SIZE] = { NULL };

#define CHECK_EGLDISPLAY(dpy) { \
if (((EGL_GLXDisplay **)dpy < display_table) \
      || ((EGL_GLXDisplay **)dpy >= &display_table[DISPLAY_TABLE_SIZE])) { \
    eglSetError (EGL_BAD_DISPLAY); \
    return EGL_FALSE; \
} \
}while(0)

#define PEGLGLXDISPLAY(dpy) (*(EGL_GLXDisplay**)dpy)

#define CHECK_EGLDISPLAY_INITIALIZED(dpy) { \
if ((*(EGL_GLXDisplay**)dpy) == NULL) { \
    eglSetError (EGL_NOT_INITIALIZED); \
    return EGL_FALSE; \
} \
} while(0)

#define CHECK_EGLCONFIG(dpy, config) { \
if (((EGL_GLXConfig*)(config) < (*(EGL_GLXDisplay**)(dpy))->configs) || \
    ((EGL_GLXConfig*)(config) >= &(*(EGL_GLXDisplay**)(dpy))->configs[(*(EGL_GLXDisplay**)(dpy))->n_configs])){ \
    eglSetError (EGL_BAD_PARAMETER); \
    return EGL_FALSE; \
} \
}while(0)

#define UNUSED(x) (void)(x)

static void eglSetError (EGLint error)
{
    UNUSED (error);
}

EGLBoolean EGLAPIENTRY eglBindAPI (EGLenum api)
{
    UNUSED (api);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglChooseConfig (EGLDisplay dpy,
                                        const EGLint *attrib_list,
                                        EGLConfig *configs, EGLint config_size,
                                        EGLint *num_config)
{
    UNUSED (dpy);
    UNUSED (attrib_list);
    UNUSED (configs);
    UNUSED (config_size);
    UNUSED (num_config);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

EGLContext EGLAPIENTRY eglCreateContext (EGLDisplay dpy, EGLConfig config,
        EGLContext share_context,
        const EGLint *attrib_list)
{
    UNUSED (dpy);
    UNUSED (config);
    UNUSED (share_context);
    UNUSED (attrib_list);
    /*TODO: Set last EGL error for this thread */
    return EGL_NO_CONTEXT;
}

EGLSurface EGLAPIENTRY eglCreateWindowSurface (EGLDisplay dpy, EGLConfig config,
        EGLNativeWindowType win,
        const EGLint *attrib_list)
{
    UNUSED (dpy);
    UNUSED (config);
    UNUSED (win);
    UNUSED (attrib_list);
    /*TODO: Set last EGL error for this thread */
    return EGL_NO_SURFACE;
}

EGLBoolean EGLAPIENTRY eglDestroyContext (EGLDisplay dpy, EGLContext ctx)
{
    UNUSED (dpy);
    UNUSED (ctx);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglDestroySurface (EGLDisplay dpy, EGLSurface surface)
{
    UNUSED (dpy);
    UNUSED (surface);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglGetConfigAttrib (EGLDisplay dpy, EGLConfig config,
        EGLint attribute, EGLint *value)
{
    EGL_GLXConfig *egl_config = NULL;
    CHECK_EGLDISPLAY (dpy);
    CHECK_EGLDISPLAY_INITIALIZED (dpy);
    CHECK_EGLCONFIG (dpy, config);
    egl_config = (EGL_GLXConfig *)config;

    if (value == NULL) {
        eglSetError (EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }
    switch (attribute) {
        case EGL_BUFFER_SIZE:
            *value = egl_config->buffer_size;
            break;
        case EGL_RED_SIZE:
            *value = egl_config->red_size;
            break;
        case EGL_GREEN_SIZE:
            *value = egl_config->green_size;
            break;
        case EGL_BLUE_SIZE:
            *value = egl_config->blue_size;
            break;
        case EGL_LUMINANCE_SIZE:
            *value = egl_config->luminance_size;
            break;
        case EGL_ALPHA_SIZE:
            *value = egl_config->alpha_size;
            break;
        case EGL_ALPHA_MASK_SIZE:
            *value = egl_config->alpha_mask_size;
            break;
        case EGL_BIND_TO_TEXTURE_RGB:
            * ((EGLBoolean *)value) = egl_config->bind_to_texture_rgb;
            break;
        case EGL_BIND_TO_TEXTURE_RGBA:
            * ((EGLBoolean *)value) = egl_config->bind_to_texture_rgba;
            break;
        case EGL_COLOR_BUFFER_TYPE:
            *value = egl_config->color_buffer_type;
            break;
        case EGL_CONFIG_CAVEAT:
            *value = egl_config->config_caveat;
            break;
        case EGL_CONFIG_ID:
            *value = egl_config->config_id;
            break;
        case EGL_CONFORMANT:
            *value = egl_config->conformant;
            break;
        case EGL_DEPTH_SIZE:
            *value = egl_config->depth_size;
            break;
        case EGL_LEVEL:
            *value = egl_config->level;
            break;
        case EGL_MAX_PBUFFER_WIDTH:
            *value = egl_config->max_pbuffer_width;
            break;
        case EGL_MAX_PBUFFER_HEIGHT:
            *value = egl_config->max_pbuffer_height;
            break;
        case EGL_MAX_PBUFFER_PIXELS:
            *value = egl_config->max_pbuffer_pixels;
            break;
        case EGL_MAX_SWAP_INTERVAL:
            *value = egl_config->max_swap_interval;
            break;
        case EGL_MIN_SWAP_INTERVAL:
            *value = egl_config->min_swap_interval;
            break;
        case EGL_NATIVE_RENDERABLE:
            * ((EGLBoolean *)value) = egl_config->native_renderable;
            break;
        case EGL_NATIVE_VISUAL_ID:
            *value = egl_config->native_visual_id;
            break;
        case EGL_NATIVE_VISUAL_TYPE:
            *value = egl_config->native_visual_type;
            break;
        case EGL_RENDERABLE_TYPE:
            *value = egl_config->renderable_type;
            break;
        case EGL_SAMPLE_BUFFERS:
            *value = egl_config->sample_buffers;
            break;
        case EGL_SAMPLES:
            *value = egl_config->samples;
            break;
        case EGL_STENCIL_SIZE:
            *value = egl_config->stencil_size;
            break;
        case EGL_SURFACE_TYPE:
            *value = egl_config->surface_type;
            break;
        case EGL_TRANSPARENT_TYPE:
            *value = egl_config->transparent_type;
            break;
        case EGL_TRANSPARENT_RED_VALUE:
            *value = egl_config->transparent_red_value;
            break;
        case EGL_TRANSPARENT_GREEN_VALUE:
            *value = egl_config->transparent_green_value;
            break;
        case EGL_TRANSPARENT_BLUE_VALUE:
            *value = egl_config->transparent_blue_value;
            break;
        default:
            eglSetError (EGL_BAD_PARAMETER);
            return EGL_FALSE;
    }
    eglSetError (EGL_SUCCESS);
    return EGL_TRUE;
}

EGLDisplay EGLAPIENTRY eglGetDisplay (EGLNativeDisplayType display_id)
{
    if (display_id != EGL_DEFAULT_DISPLAY) {
        return EGL_NO_DISPLAY;
    }
    eglSetError (EGL_SUCCESS);
    return (EGLDisplay)&display_table[0];
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

static EGLBoolean fbconfig_to_eglconfig (EGL_GLXDisplay *egl_display,
        int config_id, EGL_GLXConfig *egl_config, GLXFBConfig glx_config)
{
    int value;
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_RENDER_TYPE,
                          &value);
    value &= GLX_RGBA_BIT;
    if (!value) {
        return EGL_FALSE;
    }
    egl_config->color_buffer_type = EGL_RGB_BUFFER;

    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_CONFIG_CAVEAT, &value);
    if (value == GLX_NONE) {
        egl_config->config_caveat = EGL_NONE;
    } else if (value == GLX_SLOW_CONFIG) {
        egl_config->config_caveat = EGL_SLOW_CONFIG;
    } else {
        return EGL_FALSE;
    }
    egl_config->conformant = EGL_OPENGL_BIT;
    egl_config->renderable_type = EGL_OPENGL_BIT;
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_BUFFER_SIZE,
                          (int *)&egl_config->buffer_size);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_RED_SIZE,
                          (int *)&egl_config->red_size);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_GREEN_SIZE,
                          (int *)&egl_config->green_size);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_BLUE_SIZE,
                          (int *)&egl_config->blue_size);
    egl_config->luminance_size = 0;
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_ALPHA_SIZE,
                          (int *)&egl_config->alpha_size);
    egl_config->alpha_mask_size = 0;
    egl_config->bind_to_texture_rgb = EGL_FALSE;
    egl_config->bind_to_texture_rgba = EGL_FALSE;

    egl_config->config_id = config_id;
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_DEPTH_SIZE, (int *)&egl_config->depth_size);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_LEVEL,
                          (int *)&egl_config->level);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_MAX_PBUFFER_WIDTH,
                          (int *)&egl_config->max_pbuffer_width);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_MAX_PBUFFER_HEIGHT,
                          (int *)&egl_config->max_pbuffer_height);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_MAX_PBUFFER_PIXELS,
                          (int *)&egl_config->max_pbuffer_pixels);
    /* TODO: Implement via GLX_EXT_swap_control
     *
     * From GLX_EXT_swap_control specification:
     * The current swap interval and implementation-dependent max swap
     * interval for a particular drawable can be obtained by calling
     * glXQueryDrawable with the attributes GLX_SWAP_INTERVAL_EXT and
     * GLX_MAX_SWAP_INTERVAL_EXT respectively. The value returned by
     * glXQueryDrawable is undefined if the drawable is not a GLXWindow
     * and these attributes are given.
     */
    egl_config->max_swap_interval = 1;
    egl_config->min_swap_interval = 1;

    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_X_RENDERABLE, &value);
    egl_config->native_renderable = (value == True) ? EGL_TRUE : EGL_FALSE;

    glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_VISUAL_ID,
                          (int *)&egl_config->native_visual_id);
    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_X_VISUAL_TYPE,
                          (int *)&egl_config->native_visual_type);

    /* Sample buffers are supported only in GLX 1.4 */
    if ((egl_display->glx_minor > 1) ||
            ((egl_display->glx_major == 1) && (egl_display->glx_minor >= 4))) {
        glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                              GLX_SAMPLE_BUFFERS,
                              (int *)&egl_config->sample_buffers);
        glXGetFBConfigAttrib (egl_display->x11_display, glx_config, GLX_SAMPLES,
                              (int *)&egl_config->samples);
    } else {
        egl_config->sample_buffers = 0;
        egl_config->samples = 0;
    }

    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_STENCIL_SIZE, (int *)&egl_config->stencil_size);

    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_DRAWABLE_TYPE, &value);
    egl_config->surface_type = 0;
    egl_config->surface_type |= (value & GLX_WINDOW_BIT) ? EGL_WINDOW_BIT : 0;
    egl_config->surface_type |= (value & GLX_PIXMAP_BIT) ? EGL_PIXMAP_BIT : 0;
    egl_config->surface_type |= (value & GLX_PBUFFER_BIT) ? EGL_PBUFFER_BIT : 0;

    glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                          GLX_TRANSPARENT_TYPE,
                          &value);
    if (value == GLX_TRANSPARENT_RGB) {
        egl_config->transparent_type = EGL_TRANSPARENT_RGB;
        glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                              GLX_TRANSPARENT_RED_VALUE,
                              (int *)&egl_config->transparent_red_value);
        glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                              GLX_TRANSPARENT_GREEN_VALUE,
                              (int *)&egl_config->transparent_green_value);
        glXGetFBConfigAttrib (egl_display->x11_display, glx_config,
                              GLX_TRANSPARENT_BLUE_VALUE,
                              (int *)&egl_config->transparent_blue_value);
    } else {
        egl_config->transparent_type = EGL_NONE;
    }
    return EGL_TRUE;
}

static EGLBoolean allocate_configs (EGL_GLXDisplay *egl_display)
{
    if (egl_display->is_modern) {
        int n_glx_configs = 0;
        GLXFBConfig *glx_configs = glXGetFBConfigs (egl_display->x11_display,
                                   egl_display->screen,
                                   &n_glx_configs);
        EGL_GLXConfig *configs = (EGL_GLXConfig *) calloc ((size_t)n_glx_configs,
                                 sizeof (EGL_GLXConfig));
        if (configs) {
            int n_configs = 0;
            while (n_glx_configs > 0) {
                n_glx_configs--;
                if (fbconfig_to_eglconfig (egl_display, n_configs,
                                           &configs[n_configs],
                                           glx_configs[n_glx_configs])) {
                    n_configs++;
                }
            }
            egl_display->configs = configs;
            egl_display->n_configs = n_configs;
        } else {
            return EGL_FALSE;
        }
    } else {
        /* TODO: add support for GLX 1.2 */
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglInitialize (EGLDisplay dpy, EGLint *major,
                                      EGLint *minor)
{
    CHECK_EGLDISPLAY (dpy);
    if (PEGLGLXDISPLAY (dpy) == NULL) {
        int glx_major = 0;
        int glx_minor = 0;
        EGL_GLXDisplay *egl_display = NULL;

        Display *display = XOpenDisplay ((char *)NULL);
        if (display == NULL) {
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
        /* Check the glx version */
        if (glXQueryVersion (display, &glx_major, &glx_minor) == False) {
            XCloseDisplay (display);
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
        if ((glx_major < 1) || ((glx_major == 1) && (glx_minor < 2))) {
            XCloseDisplay (display);
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
        egl_display = (EGL_GLXDisplay *)calloc (1, sizeof (EGL_GLXDisplay));
        if (egl_display) {
            const char *extensions = NULL;
            egl_display->x11_display = display;
            egl_display->screen = DefaultScreen (display);
            egl_display->glx_major = glx_major;
            egl_display->glx_minor = glx_minor;
            egl_display->is_modern = ((glx_major == 1) && (glx_minor > 2)) ||
                                     (glx_major > 1);
            /* Check available GLX extensions */
            extensions = glXQueryExtensionsString (egl_display->x11_display,
                                                   egl_display->screen);

            if (is_extension_supported (extensions, "GLX_ARB_create_context")) {
                egl_display->glXCreateContextAttribsARB =
                    (PFNGLXCREATECONTEXTATTRIBSARBPROC)
                    glXGetProcAddressARB ((const GLubyte *)"glXCreateContextAttribsARB");
                egl_display->is_arb_context_profile = is_extension_supported (extensions,
                                                      "GLX_ARB_create_context_profile");
            }
            if (allocate_configs (egl_display) != EGL_TRUE) {
                XCloseDisplay (display);
                free (egl_display);
                eglSetError (EGL_NOT_INITIALIZED);
                return EGL_FALSE;
            }
            PEGLGLXDISPLAY (dpy) = egl_display;
        } else {
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
    }

    *major = 1;
    *minor = 5;
    eglSetError (EGL_SUCCESS);
    return EGL_TRUE;
}

EGLBoolean EGLAPIENTRY eglMakeCurrent (EGLDisplay dpy, EGLSurface draw,
                                       EGLSurface read, EGLContext ctx)
{
    UNUSED (dpy);
    UNUSED (draw);
    UNUSED (read);
    UNUSED (ctx);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

const char *EGLAPIENTRY eglQueryString (EGLDisplay dpy, EGLint name)
{
    CHECK_EGLDISPLAY (dpy);
    CHECK_EGLDISPLAY_INITIALIZED (dpy);
    switch (name) {
        case EGL_CLIENT_APIS:
            return "OpenGL";
        case EGL_VENDOR:
            return "SOURYOGURT";
        case EGL_VERSION:
            return "1.4";
        case EGL_EXTENSIONS:
            return "";
        default:
            return NULL;
    }
    return NULL;
}

EGLBoolean EGLAPIENTRY eglSwapBuffers (EGLDisplay dpy, EGLSurface surface)
{
    UNUSED (dpy);
    UNUSED (surface);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

EGLBoolean EGLAPIENTRY eglTerminate (EGLDisplay dpy)
{
    EGL_GLXDisplay *egl_display = NULL;
    CHECK_EGLDISPLAY (dpy);
    egl_display = PEGLGLXDISPLAY (dpy);
    if (egl_display != NULL) {
        XCloseDisplay (egl_display->x11_display);
        free (egl_display->configs);
        free (egl_display);
        PEGLGLXDISPLAY (dpy) = NULL;
    }
    eglSetError (EGL_SUCCESS);
    return EGL_TRUE;
}

EGLint EGLAPIENTRY eglGetError (void)
{
    return EGL_CONTEXT_LOST;
}

EGLBoolean EGLAPIENTRY eglGetConfigs (EGLDisplay dpy, EGLConfig *configs,
                                      EGLint config_size, EGLint *num_config)
{
    EGL_GLXDisplay *egl_display = NULL;
    CHECK_EGLDISPLAY (dpy);
    CHECK_EGLDISPLAY_INITIALIZED (dpy);
    egl_display = PEGLGLXDISPLAY (dpy);
    if (num_config == NULL) {
        eglSetError (EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }
    if (configs == NULL) {
        *num_config = egl_display->n_configs;
        eglSetError (EGL_SUCCESS);
        return EGL_TRUE;
    }
    for (*num_config = 0; (*num_config < config_size)
            && (*num_config < egl_display->n_configs); *num_config += 1) {
        configs[*num_config] = (EGLConfig) &egl_display->configs[*num_config];
    }
    eglSetError (EGL_SUCCESS);
    return EGL_TRUE;
}
