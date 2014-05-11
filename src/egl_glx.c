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

typedef struct EGL_GLXDisplay {
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;
    Display *display; /**< Connection to X11 display */
    int screen; /**< Screen number where UGL where created */
    int glx_major; /**< Major version of glx */
    int glx_minor; /**< Minor version of glx */
    int is_modern; /**< Is we have modern GLX version (GLX > 1.2) */
    int is_arb_context_profile; /**< Is GLX_ARB_create_context_profile there*/
    char padding[4];
} EGL_GLXDisplay;

#define DEFAULT_DISPLAY 1
#define DISPLAY_TABLE_SIZE 1
static EGL_GLXDisplay *display_table[DISPLAY_TABLE_SIZE] = { NULL };

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
    UNUSED (dpy);
    UNUSED (config);
    UNUSED (attribute);
    UNUSED (value);
    /*TODO: Set last EGL error for this thread */
    return EGL_FALSE;
}

EGLDisplay EGLAPIENTRY eglGetDisplay (EGLNativeDisplayType display_id)
{
    if (display_id != EGL_DEFAULT_DISPLAY) {
        return EGL_NO_DISPLAY;
    }
    eglSetError (EGL_SUCCESS);
    /* Return index within display_table array */
    return (EGLDisplay)1;
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

EGLBoolean EGLAPIENTRY eglInitialize (EGLDisplay dpy, EGLint *major,
                                      EGLint *minor)
{
    if ((dpy < (EGLDisplay)DEFAULT_DISPLAY) ||
            (dpy > (EGLDisplay)DISPLAY_TABLE_SIZE)) {
        eglSetError (EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    if (display_table[ (unsigned long)dpy] == NULL) {
        const char *extensions = NULL;
        int major_version = 0;
        int minor_version = 0;

        Display *display = XOpenDisplay ((char *)NULL);
        if (display == NULL) {
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
        /* Check the glx version */
        if (glXQueryVersion (display, &major_version, &minor_version) == False) {
            XCloseDisplay (display);
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
        if ((major_version < 1) || ((major_version == 1) && (minor_version < 2))) {
            XCloseDisplay (display);
            eglSetError (EGL_NOT_INITIALIZED);
            return EGL_FALSE;
        }
        EGL_GLXDisplay *egl_display = (EGL_GLXDisplay*)calloc (1, sizeof (EGL_GLXDisplay));
        if (egl_display) {
            egl_display->display = display;
            egl_display->screen = DefaultScreen (display);
            egl_display->glx_major = major_version;
            egl_display->glx_minor = minor_version;
            egl_display->is_modern = ((major_version == 1) && (minor_version > 2)) ||
                                     (major_version > 1);
            /* Check available GLX extensions */
            extensions = glXQueryExtensionsString (egl_display->display,
                                                   egl_display->screen);

            if (is_extension_supported (extensions, "GLX_ARB_create_context")) {
                egl_display->glXCreateContextAttribsARB =
                    (PFNGLXCREATECONTEXTATTRIBSARBPROC)
                    glXGetProcAddressARB ((const GLubyte *)"glXCreateContextAttribsARB");
                egl_display->is_arb_context_profile = is_extension_supported (extensions,
                                                      "GLX_ARB_create_context_profile");
            }
            display_table[ (unsigned long)dpy] = egl_display;
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
    if ((dpy < (EGLDisplay)DEFAULT_DISPLAY) ||
            (dpy > (EGLDisplay)DISPLAY_TABLE_SIZE)) {
        eglSetError (EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }
    if (display_table[ (unsigned long)dpy] == NULL) {
        eglSetError (EGL_NOT_INITIALIZED);
        return EGL_FALSE;
    }
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
    if ((dpy < (EGLDisplay)DEFAULT_DISPLAY) ||
            (dpy > (EGLDisplay)DISPLAY_TABLE_SIZE)) {
        eglSetError (EGL_BAD_DISPLAY);
        return EGL_FALSE;
    }

    if (display_table[ (unsigned long)dpy] != NULL) {
        EGL_GLXDisplay *egl_display = display_table[(unsigned long)dpy];
        XCloseDisplay(egl_display->display);
        free (egl_display);
        display_table[ (unsigned long)dpy] = NULL;
    }
    eglSetError (EGL_SUCCESS);
    return EGL_TRUE;
}

EGLint EGLAPIENTRY eglGetError (void)
{
    return EGL_CONTEXT_LOST;
}

