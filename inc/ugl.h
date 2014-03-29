#ifndef UGL_H
#define UGL_H

/** Universal OpenGL interface type */
typedef struct UGL UGL;

/** Type that encapsulates framebuffer attributes */
typedef void UGLFrameBufferConfig;

/** Type that encapsulates rendering surface */
typedef struct UGLRenderSurface UGLRenderSurface;

#if defined(__unix__)
#include <X11/Xlib.h>
typedef Window UGLNativeWindow;
#endif

enum UGLConfigAttribute {
    UGL_NATIVE_VISUAL_ID
};

#ifdef __cplusplus
extern "C" {
#endif

/** Creates universal interface to OpenGL subsystem
 * @param display_id The display where application runs
 * @returns interface object, NULL otherwise
 */
UGL *ugl_create (void *display_id);

/** Choose framebuffer configuration based on specified attributes
 * @param ugl pointer to universal OpenGL interface object
 * @param attributes the array of GLX 1.3 attributes
 * @returns framebuffer configuration object
 */
UGLFrameBufferConfig *ugl_choose_framebuffer_config (const UGL *ugl,
        const int *attributes);

/** Get framebuffer configuration's attribute
 * @param ugl pointer to universal OpenGL interface object
 * @param config pointer to framebuffer configuration object
 * @param attribute UGL attribute
 * @param value pointer to value storage
 * @returns non-zero if attribute retreived succesfully, 0 otherwise
 */
int ugl_get_config_attribute (const UGL *ugl, UGLFrameBufferConfig *config,
                              enum UGLConfigAttribute attribute, void *value);

/** Create render surface assosiated with window
 * @param ugl pointer to universal OpenGL interface object
 * @param config pointer to framebuffer configuration object
 * @param window native window handle
 * @returns new render surface object, NULL otherwise
 */
UGLRenderSurface *ugl_create_window_render_surface (const UGL *ugl,
        UGLFrameBufferConfig *config, UGLNativeWindow window);

/** Make render surface to be current for OpenGL API
 * @param ugl pointer to universal OpenGL interface object
 * @param surface pointer to render surface object
 * @returns non-zero on success, 0 otherwise
 */
int ugl_make_current (const UGL *ugl, const UGLRenderSurface *surface);

/** Swap front and back buffers on a surface
 * @param ugl pointer to universal OpenGL interface object
 * @param surface pointer to render surface object
 */
void ugl_swap_buffers (const UGL *ugl, const UGLRenderSurface *surface);

/** Frees rendering surface
 * @param ugl pointer to universal OpenGL interface object
 * @param surface pointer to render surface object
 */
void ugl_free_render_surface (const UGL *ugl, UGLRenderSurface *surface);

/** Frees universal framebuffer configuration
 * @param ugl pointer to UGL interface
 * @param config Configuration to be freed
 */
void ugl_free_framebuffer_config (const UGL *ugl, UGLFrameBufferConfig *config);

/** Frees universal OpenGL interface object
 * @param ugl pointer to the object
 */
void ugl_free (UGL *ugl);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: UGL_H */
