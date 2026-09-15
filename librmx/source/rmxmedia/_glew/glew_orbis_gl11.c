/*
** GLEW on the PlayStation 4 (OpenOrbis + static Mesa, EGL platform "orbis", zink over RADV).
**
** GLEW declares the OpenGL 1.1 entry points in GL/glew.h as ordinary extern functions (everything from
** GL 1.2 on is a function pointer that glewInit() fills through glewGetProcAddress). On this target
** there is no libGL: the only GL archive with exported entry points is Mesa's libGLESv2.a, which
** defines the GL 1.1 functions that also exist in OpenGL ES (glClear, glTexImage2D, glReadPixels...).
** The desktop-only GL 1.1 functions - glGetTexImage, glClearDepth, glDepthRange, glPolygonMode,
** glDrawBuffer, the fixed-function API - have no link-time symbol at all, although the driver
** implements them: Mesa's eglGetProcAddress() hands out the shared-glapi dispatch stub for every GL
** name, desktop ones included.
**
** This file supplies exactly those missing symbols, as trampolines that resolve through
** eglGetProcAddress() on first call. The list is the set difference between GL_VERSION_1_1 in
** GL/glew.h and the symbols defined by mesa-ps4 build-orbis/src/mesa/glapi/es2api/libGLESv2.a
** (2026-09-15): defining a function here that libGLESv2.a also exports would be a duplicate
** definition. Regenerate if either side changes.
**
** Compiled only when GLEW_EGL and __ORBIS__ are both defined (glew.c turns GLEW_EGL on for __ORBIS__);
** on every other platform this translation unit is empty.
*/

#if defined(__ORBIS__) && !defined(GLEW_EGL)
#  define GLEW_EGL
#endif
#if defined(__ORBIS__) && !defined(GLEW_STATIC)
#  define GLEW_STATIC
#endif
#if defined(__ORBIS__) && !defined(GLEW_NO_GLU)
#  define GLEW_NO_GLU
#endif

#if defined(GLEW_EGL) && defined(__ORBIS__)

#include <GL/glew.h>
#include <GL/eglew.h>
#include <stddef.h>
#include <stdlib.h>
#include <orbis_log.h>

static void* glewOrbisResolve (const char* name)
{
  void* fn = (void*)eglGetProcAddress(name);
  if (fn == NULL)
  {
    /* Cannot happen with Mesa's shared glapi (it has a stub for every GL name); if it does, the build
     * is linked against something else and calling through NULL would only obscure that. */
    orbis_log_fatal("GLEW(orbis): eglGetProcAddress(\"%s\") returned NULL - aborting", name);
    abort();
  }
  return fn;
}

void GLAPIENTRY glAccum (GLenum op, GLfloat value)
{
  static void (GLAPIENTRY *fn)(GLenum op, GLfloat value) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum op, GLfloat value))glewOrbisResolve("glAccum");
  fn(op, value);
}

void GLAPIENTRY glAlphaFunc (GLenum func, GLclampf ref)
{
  static void (GLAPIENTRY *fn)(GLenum func, GLclampf ref) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum func, GLclampf ref))glewOrbisResolve("glAlphaFunc");
  fn(func, ref);
}

GLboolean GLAPIENTRY glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences)
{
  static GLboolean (GLAPIENTRY *fn)(GLsizei n, const GLuint *textures, GLboolean *residences) = NULL;
  if (fn == NULL) fn = (GLboolean (GLAPIENTRY *)(GLsizei n, const GLuint *textures, GLboolean *residences))glewOrbisResolve("glAreTexturesResident");
  return fn(n, textures, residences);
}

void GLAPIENTRY glArrayElement (GLint i)
{
  static void (GLAPIENTRY *fn)(GLint i) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint i))glewOrbisResolve("glArrayElement");
  fn(i);
}

void GLAPIENTRY glBegin (GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum mode))glewOrbisResolve("glBegin");
  fn(mode);
}

void GLAPIENTRY glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
  static void (GLAPIENTRY *fn)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap))glewOrbisResolve("glBitmap");
  fn(width, height, xorig, yorig, xmove, ymove, bitmap);
}

void GLAPIENTRY glCallList (GLuint list)
{
  static void (GLAPIENTRY *fn)(GLuint list) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint list))glewOrbisResolve("glCallList");
  fn(list);
}

void GLAPIENTRY glCallLists (GLsizei n, GLenum type, const void *lists)
{
  static void (GLAPIENTRY *fn)(GLsizei n, GLenum type, const void *lists) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei n, GLenum type, const void *lists))glewOrbisResolve("glCallLists");
  fn(n, type, lists);
}

void GLAPIENTRY glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
  static void (GLAPIENTRY *fn)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))glewOrbisResolve("glClearAccum");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glClearDepth (GLclampd depth)
{
  static void (GLAPIENTRY *fn)(GLclampd depth) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLclampd depth))glewOrbisResolve("glClearDepth");
  fn(depth);
}

void GLAPIENTRY glClearIndex (GLfloat c)
{
  static void (GLAPIENTRY *fn)(GLfloat c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat c))glewOrbisResolve("glClearIndex");
  fn(c);
}

void GLAPIENTRY glClipPlane (GLenum plane, const GLdouble *equation)
{
  static void (GLAPIENTRY *fn)(GLenum plane, const GLdouble *equation) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum plane, const GLdouble *equation))glewOrbisResolve("glClipPlane");
  fn(plane, equation);
}

void GLAPIENTRY glColor3b (GLbyte red, GLbyte green, GLbyte blue)
{
  static void (GLAPIENTRY *fn)(GLbyte red, GLbyte green, GLbyte blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLbyte red, GLbyte green, GLbyte blue))glewOrbisResolve("glColor3b");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3bv (const GLbyte *v)
{
  static void (GLAPIENTRY *fn)(const GLbyte *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLbyte *v))glewOrbisResolve("glColor3bv");
  fn(v);
}

void GLAPIENTRY glColor3d (GLdouble red, GLdouble green, GLdouble blue)
{
  static void (GLAPIENTRY *fn)(GLdouble red, GLdouble green, GLdouble blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble red, GLdouble green, GLdouble blue))glewOrbisResolve("glColor3d");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glColor3dv");
  fn(v);
}

void GLAPIENTRY glColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
  static void (GLAPIENTRY *fn)(GLfloat red, GLfloat green, GLfloat blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat red, GLfloat green, GLfloat blue))glewOrbisResolve("glColor3f");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glColor3fv");
  fn(v);
}

void GLAPIENTRY glColor3i (GLint red, GLint green, GLint blue)
{
  static void (GLAPIENTRY *fn)(GLint red, GLint green, GLint blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint red, GLint green, GLint blue))glewOrbisResolve("glColor3i");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glColor3iv");
  fn(v);
}

void GLAPIENTRY glColor3s (GLshort red, GLshort green, GLshort blue)
{
  static void (GLAPIENTRY *fn)(GLshort red, GLshort green, GLshort blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort red, GLshort green, GLshort blue))glewOrbisResolve("glColor3s");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glColor3sv");
  fn(v);
}

void GLAPIENTRY glColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
  static void (GLAPIENTRY *fn)(GLubyte red, GLubyte green, GLubyte blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLubyte red, GLubyte green, GLubyte blue))glewOrbisResolve("glColor3ub");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3ubv (const GLubyte *v)
{
  static void (GLAPIENTRY *fn)(const GLubyte *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLubyte *v))glewOrbisResolve("glColor3ubv");
  fn(v);
}

void GLAPIENTRY glColor3ui (GLuint red, GLuint green, GLuint blue)
{
  static void (GLAPIENTRY *fn)(GLuint red, GLuint green, GLuint blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint red, GLuint green, GLuint blue))glewOrbisResolve("glColor3ui");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3uiv (const GLuint *v)
{
  static void (GLAPIENTRY *fn)(const GLuint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLuint *v))glewOrbisResolve("glColor3uiv");
  fn(v);
}

void GLAPIENTRY glColor3us (GLushort red, GLushort green, GLushort blue)
{
  static void (GLAPIENTRY *fn)(GLushort red, GLushort green, GLushort blue) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLushort red, GLushort green, GLushort blue))glewOrbisResolve("glColor3us");
  fn(red, green, blue);
}

void GLAPIENTRY glColor3usv (const GLushort *v)
{
  static void (GLAPIENTRY *fn)(const GLushort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLushort *v))glewOrbisResolve("glColor3usv");
  fn(v);
}

void GLAPIENTRY glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
  static void (GLAPIENTRY *fn)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha))glewOrbisResolve("glColor4b");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4bv (const GLbyte *v)
{
  static void (GLAPIENTRY *fn)(const GLbyte *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLbyte *v))glewOrbisResolve("glColor4bv");
  fn(v);
}

void GLAPIENTRY glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
  static void (GLAPIENTRY *fn)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha))glewOrbisResolve("glColor4d");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glColor4dv");
  fn(v);
}

void GLAPIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
  static void (GLAPIENTRY *fn)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))glewOrbisResolve("glColor4f");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glColor4fv");
  fn(v);
}

void GLAPIENTRY glColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{
  static void (GLAPIENTRY *fn)(GLint red, GLint green, GLint blue, GLint alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint red, GLint green, GLint blue, GLint alpha))glewOrbisResolve("glColor4i");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glColor4iv");
  fn(v);
}

void GLAPIENTRY glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
  static void (GLAPIENTRY *fn)(GLshort red, GLshort green, GLshort blue, GLshort alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort red, GLshort green, GLshort blue, GLshort alpha))glewOrbisResolve("glColor4s");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glColor4sv");
  fn(v);
}

void GLAPIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
  static void (GLAPIENTRY *fn)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha))glewOrbisResolve("glColor4ub");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4ubv (const GLubyte *v)
{
  static void (GLAPIENTRY *fn)(const GLubyte *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLubyte *v))glewOrbisResolve("glColor4ubv");
  fn(v);
}

void GLAPIENTRY glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
  static void (GLAPIENTRY *fn)(GLuint red, GLuint green, GLuint blue, GLuint alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint red, GLuint green, GLuint blue, GLuint alpha))glewOrbisResolve("glColor4ui");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4uiv (const GLuint *v)
{
  static void (GLAPIENTRY *fn)(const GLuint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLuint *v))glewOrbisResolve("glColor4uiv");
  fn(v);
}

void GLAPIENTRY glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
  static void (GLAPIENTRY *fn)(GLushort red, GLushort green, GLushort blue, GLushort alpha) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLushort red, GLushort green, GLushort blue, GLushort alpha))glewOrbisResolve("glColor4us");
  fn(red, green, blue, alpha);
}

void GLAPIENTRY glColor4usv (const GLushort *v)
{
  static void (GLAPIENTRY *fn)(const GLushort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLushort *v))glewOrbisResolve("glColor4usv");
  fn(v);
}

void GLAPIENTRY glColorMaterial (GLenum face, GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum mode))glewOrbisResolve("glColorMaterial");
  fn(face, mode);
}

void GLAPIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLint size, GLenum type, GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint size, GLenum type, GLsizei stride, const void *pointer))glewOrbisResolve("glColorPointer");
  fn(size, type, stride, pointer);
}

void GLAPIENTRY glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type))glewOrbisResolve("glCopyPixels");
  fn(x, y, width, height, type);
}

void GLAPIENTRY glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border))glewOrbisResolve("glCopyTexImage1D");
  fn(target, level, internalFormat, x, y, width, border);
}

void GLAPIENTRY glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width))glewOrbisResolve("glCopyTexSubImage1D");
  fn(target, level, xoffset, x, y, width);
}

void GLAPIENTRY glDeleteLists (GLuint list, GLsizei range)
{
  static void (GLAPIENTRY *fn)(GLuint list, GLsizei range) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint list, GLsizei range))glewOrbisResolve("glDeleteLists");
  fn(list, range);
}

void GLAPIENTRY glDepthRange (GLclampd zNear, GLclampd zFar)
{
  static void (GLAPIENTRY *fn)(GLclampd zNear, GLclampd zFar) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLclampd zNear, GLclampd zFar))glewOrbisResolve("glDepthRange");
  fn(zNear, zFar);
}

void GLAPIENTRY glDisableClientState (GLenum array)
{
  static void (GLAPIENTRY *fn)(GLenum array) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum array))glewOrbisResolve("glDisableClientState");
  fn(array);
}

void GLAPIENTRY glDrawBuffer (GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum mode))glewOrbisResolve("glDrawBuffer");
  fn(mode);
}

void GLAPIENTRY glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
  static void (GLAPIENTRY *fn)(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels))glewOrbisResolve("glDrawPixels");
  fn(width, height, format, type, pixels);
}

void GLAPIENTRY glEdgeFlag (GLboolean flag)
{
  static void (GLAPIENTRY *fn)(GLboolean flag) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLboolean flag))glewOrbisResolve("glEdgeFlag");
  fn(flag);
}

void GLAPIENTRY glEdgeFlagPointer (GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei stride, const void *pointer))glewOrbisResolve("glEdgeFlagPointer");
  fn(stride, pointer);
}

void GLAPIENTRY glEdgeFlagv (const GLboolean *flag)
{
  static void (GLAPIENTRY *fn)(const GLboolean *flag) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLboolean *flag))glewOrbisResolve("glEdgeFlagv");
  fn(flag);
}

void GLAPIENTRY glEnableClientState (GLenum array)
{
  static void (GLAPIENTRY *fn)(GLenum array) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum array))glewOrbisResolve("glEnableClientState");
  fn(array);
}

void GLAPIENTRY glEnd (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glEnd");
  fn();
}

void GLAPIENTRY glEndList (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glEndList");
  fn();
}

void GLAPIENTRY glEvalCoord1d (GLdouble u)
{
  static void (GLAPIENTRY *fn)(GLdouble u) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble u))glewOrbisResolve("glEvalCoord1d");
  fn(u);
}

void GLAPIENTRY glEvalCoord1dv (const GLdouble *u)
{
  static void (GLAPIENTRY *fn)(const GLdouble *u) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *u))glewOrbisResolve("glEvalCoord1dv");
  fn(u);
}

void GLAPIENTRY glEvalCoord1f (GLfloat u)
{
  static void (GLAPIENTRY *fn)(GLfloat u) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat u))glewOrbisResolve("glEvalCoord1f");
  fn(u);
}

void GLAPIENTRY glEvalCoord1fv (const GLfloat *u)
{
  static void (GLAPIENTRY *fn)(const GLfloat *u) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *u))glewOrbisResolve("glEvalCoord1fv");
  fn(u);
}

void GLAPIENTRY glEvalCoord2d (GLdouble u, GLdouble v)
{
  static void (GLAPIENTRY *fn)(GLdouble u, GLdouble v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble u, GLdouble v))glewOrbisResolve("glEvalCoord2d");
  fn(u, v);
}

void GLAPIENTRY glEvalCoord2dv (const GLdouble *u)
{
  static void (GLAPIENTRY *fn)(const GLdouble *u) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *u))glewOrbisResolve("glEvalCoord2dv");
  fn(u);
}

void GLAPIENTRY glEvalCoord2f (GLfloat u, GLfloat v)
{
  static void (GLAPIENTRY *fn)(GLfloat u, GLfloat v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat u, GLfloat v))glewOrbisResolve("glEvalCoord2f");
  fn(u, v);
}

void GLAPIENTRY glEvalCoord2fv (const GLfloat *u)
{
  static void (GLAPIENTRY *fn)(const GLfloat *u) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *u))glewOrbisResolve("glEvalCoord2fv");
  fn(u);
}

void GLAPIENTRY glEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{
  static void (GLAPIENTRY *fn)(GLenum mode, GLint i1, GLint i2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum mode, GLint i1, GLint i2))glewOrbisResolve("glEvalMesh1");
  fn(mode, i1, i2);
}

void GLAPIENTRY glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
  static void (GLAPIENTRY *fn)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2))glewOrbisResolve("glEvalMesh2");
  fn(mode, i1, i2, j1, j2);
}

void GLAPIENTRY glEvalPoint1 (GLint i)
{
  static void (GLAPIENTRY *fn)(GLint i) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint i))glewOrbisResolve("glEvalPoint1");
  fn(i);
}

void GLAPIENTRY glEvalPoint2 (GLint i, GLint j)
{
  static void (GLAPIENTRY *fn)(GLint i, GLint j) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint i, GLint j))glewOrbisResolve("glEvalPoint2");
  fn(i, j);
}

void GLAPIENTRY glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer)
{
  static void (GLAPIENTRY *fn)(GLsizei size, GLenum type, GLfloat *buffer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei size, GLenum type, GLfloat *buffer))glewOrbisResolve("glFeedbackBuffer");
  fn(size, type, buffer);
}

void GLAPIENTRY glFogf (GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLfloat param))glewOrbisResolve("glFogf");
  fn(pname, param);
}

void GLAPIENTRY glFogfv (GLenum pname, const GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum pname, const GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, const GLfloat *params))glewOrbisResolve("glFogfv");
  fn(pname, params);
}

void GLAPIENTRY glFogi (GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLint param))glewOrbisResolve("glFogi");
  fn(pname, param);
}

void GLAPIENTRY glFogiv (GLenum pname, const GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum pname, const GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, const GLint *params))glewOrbisResolve("glFogiv");
  fn(pname, params);
}

void GLAPIENTRY glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
  static void (GLAPIENTRY *fn)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))glewOrbisResolve("glFrustum");
  fn(left, right, bottom, top, zNear, zFar);
}

GLuint GLAPIENTRY glGenLists (GLsizei range)
{
  static GLuint (GLAPIENTRY *fn)(GLsizei range) = NULL;
  if (fn == NULL) fn = (GLuint (GLAPIENTRY *)(GLsizei range))glewOrbisResolve("glGenLists");
  return fn(range);
}

void GLAPIENTRY glGetClipPlane (GLenum plane, GLdouble *equation)
{
  static void (GLAPIENTRY *fn)(GLenum plane, GLdouble *equation) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum plane, GLdouble *equation))glewOrbisResolve("glGetClipPlane");
  fn(plane, equation);
}

void GLAPIENTRY glGetDoublev (GLenum pname, GLdouble *params)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLdouble *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLdouble *params))glewOrbisResolve("glGetDoublev");
  fn(pname, params);
}

void GLAPIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum light, GLenum pname, GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum light, GLenum pname, GLfloat *params))glewOrbisResolve("glGetLightfv");
  fn(light, pname, params);
}

void GLAPIENTRY glGetLightiv (GLenum light, GLenum pname, GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum light, GLenum pname, GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum light, GLenum pname, GLint *params))glewOrbisResolve("glGetLightiv");
  fn(light, pname, params);
}

void GLAPIENTRY glGetMapdv (GLenum target, GLenum query, GLdouble *v)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum query, GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum query, GLdouble *v))glewOrbisResolve("glGetMapdv");
  fn(target, query, v);
}

void GLAPIENTRY glGetMapfv (GLenum target, GLenum query, GLfloat *v)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum query, GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum query, GLfloat *v))glewOrbisResolve("glGetMapfv");
  fn(target, query, v);
}

void GLAPIENTRY glGetMapiv (GLenum target, GLenum query, GLint *v)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum query, GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum query, GLint *v))glewOrbisResolve("glGetMapiv");
  fn(target, query, v);
}

void GLAPIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum pname, GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum pname, GLfloat *params))glewOrbisResolve("glGetMaterialfv");
  fn(face, pname, params);
}

void GLAPIENTRY glGetMaterialiv (GLenum face, GLenum pname, GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum pname, GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum pname, GLint *params))glewOrbisResolve("glGetMaterialiv");
  fn(face, pname, params);
}

void GLAPIENTRY glGetPixelMapfv (GLenum map, GLfloat *values)
{
  static void (GLAPIENTRY *fn)(GLenum map, GLfloat *values) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum map, GLfloat *values))glewOrbisResolve("glGetPixelMapfv");
  fn(map, values);
}

void GLAPIENTRY glGetPixelMapuiv (GLenum map, GLuint *values)
{
  static void (GLAPIENTRY *fn)(GLenum map, GLuint *values) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum map, GLuint *values))glewOrbisResolve("glGetPixelMapuiv");
  fn(map, values);
}

void GLAPIENTRY glGetPixelMapusv (GLenum map, GLushort *values)
{
  static void (GLAPIENTRY *fn)(GLenum map, GLushort *values) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum map, GLushort *values))glewOrbisResolve("glGetPixelMapusv");
  fn(map, values);
}

void GLAPIENTRY glGetPolygonStipple (GLubyte *mask)
{
  static void (GLAPIENTRY *fn)(GLubyte *mask) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLubyte *mask))glewOrbisResolve("glGetPolygonStipple");
  fn(mask);
}

void GLAPIENTRY glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum pname, GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum pname, GLfloat *params))glewOrbisResolve("glGetTexEnvfv");
  fn(target, pname, params);
}

void GLAPIENTRY glGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum pname, GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum pname, GLint *params))glewOrbisResolve("glGetTexEnviv");
  fn(target, pname, params);
}

void GLAPIENTRY glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, GLdouble *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, GLdouble *params))glewOrbisResolve("glGetTexGendv");
  fn(coord, pname, params);
}

void GLAPIENTRY glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, GLfloat *params))glewOrbisResolve("glGetTexGenfv");
  fn(coord, pname, params);
}

void GLAPIENTRY glGetTexGeniv (GLenum coord, GLenum pname, GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, GLint *params))glewOrbisResolve("glGetTexGeniv");
  fn(coord, pname, params);
}

void GLAPIENTRY glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, void *pixels)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLint level, GLenum format, GLenum type, void *pixels) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLint level, GLenum format, GLenum type, void *pixels))glewOrbisResolve("glGetTexImage");
  fn(target, level, format, type, pixels);
}

void GLAPIENTRY glIndexMask (GLuint mask)
{
  static void (GLAPIENTRY *fn)(GLuint mask) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint mask))glewOrbisResolve("glIndexMask");
  fn(mask);
}

void GLAPIENTRY glIndexPointer (GLenum type, GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLenum type, GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum type, GLsizei stride, const void *pointer))glewOrbisResolve("glIndexPointer");
  fn(type, stride, pointer);
}

void GLAPIENTRY glIndexd (GLdouble c)
{
  static void (GLAPIENTRY *fn)(GLdouble c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble c))glewOrbisResolve("glIndexd");
  fn(c);
}

void GLAPIENTRY glIndexdv (const GLdouble *c)
{
  static void (GLAPIENTRY *fn)(const GLdouble *c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *c))glewOrbisResolve("glIndexdv");
  fn(c);
}

void GLAPIENTRY glIndexf (GLfloat c)
{
  static void (GLAPIENTRY *fn)(GLfloat c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat c))glewOrbisResolve("glIndexf");
  fn(c);
}

void GLAPIENTRY glIndexfv (const GLfloat *c)
{
  static void (GLAPIENTRY *fn)(const GLfloat *c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *c))glewOrbisResolve("glIndexfv");
  fn(c);
}

void GLAPIENTRY glIndexi (GLint c)
{
  static void (GLAPIENTRY *fn)(GLint c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint c))glewOrbisResolve("glIndexi");
  fn(c);
}

void GLAPIENTRY glIndexiv (const GLint *c)
{
  static void (GLAPIENTRY *fn)(const GLint *c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *c))glewOrbisResolve("glIndexiv");
  fn(c);
}

void GLAPIENTRY glIndexs (GLshort c)
{
  static void (GLAPIENTRY *fn)(GLshort c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort c))glewOrbisResolve("glIndexs");
  fn(c);
}

void GLAPIENTRY glIndexsv (const GLshort *c)
{
  static void (GLAPIENTRY *fn)(const GLshort *c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *c))glewOrbisResolve("glIndexsv");
  fn(c);
}

void GLAPIENTRY glIndexub (GLubyte c)
{
  static void (GLAPIENTRY *fn)(GLubyte c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLubyte c))glewOrbisResolve("glIndexub");
  fn(c);
}

void GLAPIENTRY glIndexubv (const GLubyte *c)
{
  static void (GLAPIENTRY *fn)(const GLubyte *c) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLubyte *c))glewOrbisResolve("glIndexubv");
  fn(c);
}

void GLAPIENTRY glInitNames (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glInitNames");
  fn();
}

void GLAPIENTRY glInterleavedArrays (GLenum format, GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLenum format, GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum format, GLsizei stride, const void *pointer))glewOrbisResolve("glInterleavedArrays");
  fn(format, stride, pointer);
}

GLboolean GLAPIENTRY glIsList (GLuint list)
{
  static GLboolean (GLAPIENTRY *fn)(GLuint list) = NULL;
  if (fn == NULL) fn = (GLboolean (GLAPIENTRY *)(GLuint list))glewOrbisResolve("glIsList");
  return fn(list);
}

void GLAPIENTRY glLightModelf (GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLfloat param))glewOrbisResolve("glLightModelf");
  fn(pname, param);
}

void GLAPIENTRY glLightModelfv (GLenum pname, const GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum pname, const GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, const GLfloat *params))glewOrbisResolve("glLightModelfv");
  fn(pname, params);
}

void GLAPIENTRY glLightModeli (GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLint param))glewOrbisResolve("glLightModeli");
  fn(pname, param);
}

void GLAPIENTRY glLightModeliv (GLenum pname, const GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum pname, const GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, const GLint *params))glewOrbisResolve("glLightModeliv");
  fn(pname, params);
}

void GLAPIENTRY glLightf (GLenum light, GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum light, GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum light, GLenum pname, GLfloat param))glewOrbisResolve("glLightf");
  fn(light, pname, param);
}

void GLAPIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum light, GLenum pname, const GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum light, GLenum pname, const GLfloat *params))glewOrbisResolve("glLightfv");
  fn(light, pname, params);
}

void GLAPIENTRY glLighti (GLenum light, GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum light, GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum light, GLenum pname, GLint param))glewOrbisResolve("glLighti");
  fn(light, pname, param);
}

void GLAPIENTRY glLightiv (GLenum light, GLenum pname, const GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum light, GLenum pname, const GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum light, GLenum pname, const GLint *params))glewOrbisResolve("glLightiv");
  fn(light, pname, params);
}

void GLAPIENTRY glLineStipple (GLint factor, GLushort pattern)
{
  static void (GLAPIENTRY *fn)(GLint factor, GLushort pattern) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint factor, GLushort pattern))glewOrbisResolve("glLineStipple");
  fn(factor, pattern);
}

void GLAPIENTRY glListBase (GLuint base)
{
  static void (GLAPIENTRY *fn)(GLuint base) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint base))glewOrbisResolve("glListBase");
  fn(base);
}

void GLAPIENTRY glLoadIdentity (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glLoadIdentity");
  fn();
}

void GLAPIENTRY glLoadMatrixd (const GLdouble *m)
{
  static void (GLAPIENTRY *fn)(const GLdouble *m) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *m))glewOrbisResolve("glLoadMatrixd");
  fn(m);
}

void GLAPIENTRY glLoadMatrixf (const GLfloat *m)
{
  static void (GLAPIENTRY *fn)(const GLfloat *m) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *m))glewOrbisResolve("glLoadMatrixf");
  fn(m);
}

void GLAPIENTRY glLoadName (GLuint name)
{
  static void (GLAPIENTRY *fn)(GLuint name) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint name))glewOrbisResolve("glLoadName");
  fn(name);
}

void GLAPIENTRY glLogicOp (GLenum opcode)
{
  static void (GLAPIENTRY *fn)(GLenum opcode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum opcode))glewOrbisResolve("glLogicOp");
  fn(opcode);
}

void GLAPIENTRY glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points))glewOrbisResolve("glMap1d");
  fn(target, u1, u2, stride, order, points);
}

void GLAPIENTRY glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points))glewOrbisResolve("glMap1f");
  fn(target, u1, u2, stride, order, points);
}

void GLAPIENTRY glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points))glewOrbisResolve("glMap2d");
  fn(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPIENTRY glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points))glewOrbisResolve("glMap2f");
  fn(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPIENTRY glMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{
  static void (GLAPIENTRY *fn)(GLint un, GLdouble u1, GLdouble u2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint un, GLdouble u1, GLdouble u2))glewOrbisResolve("glMapGrid1d");
  fn(un, u1, u2);
}

void GLAPIENTRY glMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{
  static void (GLAPIENTRY *fn)(GLint un, GLfloat u1, GLfloat u2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint un, GLfloat u1, GLfloat u2))glewOrbisResolve("glMapGrid1f");
  fn(un, u1, u2);
}

void GLAPIENTRY glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
  static void (GLAPIENTRY *fn)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2))glewOrbisResolve("glMapGrid2d");
  fn(un, u1, u2, vn, v1, v2);
}

void GLAPIENTRY glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
  static void (GLAPIENTRY *fn)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2))glewOrbisResolve("glMapGrid2f");
  fn(un, u1, u2, vn, v1, v2);
}

void GLAPIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum pname, GLfloat param))glewOrbisResolve("glMaterialf");
  fn(face, pname, param);
}

void GLAPIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum pname, const GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum pname, const GLfloat *params))glewOrbisResolve("glMaterialfv");
  fn(face, pname, params);
}

void GLAPIENTRY glMateriali (GLenum face, GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum pname, GLint param))glewOrbisResolve("glMateriali");
  fn(face, pname, param);
}

void GLAPIENTRY glMaterialiv (GLenum face, GLenum pname, const GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum pname, const GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum pname, const GLint *params))glewOrbisResolve("glMaterialiv");
  fn(face, pname, params);
}

void GLAPIENTRY glMatrixMode (GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum mode))glewOrbisResolve("glMatrixMode");
  fn(mode);
}

void GLAPIENTRY glMultMatrixd (const GLdouble *m)
{
  static void (GLAPIENTRY *fn)(const GLdouble *m) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *m))glewOrbisResolve("glMultMatrixd");
  fn(m);
}

void GLAPIENTRY glMultMatrixf (const GLfloat *m)
{
  static void (GLAPIENTRY *fn)(const GLfloat *m) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *m))glewOrbisResolve("glMultMatrixf");
  fn(m);
}

void GLAPIENTRY glNewList (GLuint list, GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLuint list, GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint list, GLenum mode))glewOrbisResolve("glNewList");
  fn(list, mode);
}

void GLAPIENTRY glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{
  static void (GLAPIENTRY *fn)(GLbyte nx, GLbyte ny, GLbyte nz) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLbyte nx, GLbyte ny, GLbyte nz))glewOrbisResolve("glNormal3b");
  fn(nx, ny, nz);
}

void GLAPIENTRY glNormal3bv (const GLbyte *v)
{
  static void (GLAPIENTRY *fn)(const GLbyte *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLbyte *v))glewOrbisResolve("glNormal3bv");
  fn(v);
}

void GLAPIENTRY glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{
  static void (GLAPIENTRY *fn)(GLdouble nx, GLdouble ny, GLdouble nz) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble nx, GLdouble ny, GLdouble nz))glewOrbisResolve("glNormal3d");
  fn(nx, ny, nz);
}

void GLAPIENTRY glNormal3dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glNormal3dv");
  fn(v);
}

void GLAPIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
  static void (GLAPIENTRY *fn)(GLfloat nx, GLfloat ny, GLfloat nz) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat nx, GLfloat ny, GLfloat nz))glewOrbisResolve("glNormal3f");
  fn(nx, ny, nz);
}

void GLAPIENTRY glNormal3fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glNormal3fv");
  fn(v);
}

void GLAPIENTRY glNormal3i (GLint nx, GLint ny, GLint nz)
{
  static void (GLAPIENTRY *fn)(GLint nx, GLint ny, GLint nz) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint nx, GLint ny, GLint nz))glewOrbisResolve("glNormal3i");
  fn(nx, ny, nz);
}

void GLAPIENTRY glNormal3iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glNormal3iv");
  fn(v);
}

void GLAPIENTRY glNormal3s (GLshort nx, GLshort ny, GLshort nz)
{
  static void (GLAPIENTRY *fn)(GLshort nx, GLshort ny, GLshort nz) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort nx, GLshort ny, GLshort nz))glewOrbisResolve("glNormal3s");
  fn(nx, ny, nz);
}

void GLAPIENTRY glNormal3sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glNormal3sv");
  fn(v);
}

void GLAPIENTRY glNormalPointer (GLenum type, GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLenum type, GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum type, GLsizei stride, const void *pointer))glewOrbisResolve("glNormalPointer");
  fn(type, stride, pointer);
}

void GLAPIENTRY glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
  static void (GLAPIENTRY *fn)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))glewOrbisResolve("glOrtho");
  fn(left, right, bottom, top, zNear, zFar);
}

void GLAPIENTRY glPassThrough (GLfloat token)
{
  static void (GLAPIENTRY *fn)(GLfloat token) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat token))glewOrbisResolve("glPassThrough");
  fn(token);
}

void GLAPIENTRY glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values)
{
  static void (GLAPIENTRY *fn)(GLenum map, GLsizei mapsize, const GLfloat *values) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum map, GLsizei mapsize, const GLfloat *values))glewOrbisResolve("glPixelMapfv");
  fn(map, mapsize, values);
}

void GLAPIENTRY glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values)
{
  static void (GLAPIENTRY *fn)(GLenum map, GLsizei mapsize, const GLuint *values) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum map, GLsizei mapsize, const GLuint *values))glewOrbisResolve("glPixelMapuiv");
  fn(map, mapsize, values);
}

void GLAPIENTRY glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values)
{
  static void (GLAPIENTRY *fn)(GLenum map, GLsizei mapsize, const GLushort *values) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum map, GLsizei mapsize, const GLushort *values))glewOrbisResolve("glPixelMapusv");
  fn(map, mapsize, values);
}

void GLAPIENTRY glPixelStoref (GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLfloat param))glewOrbisResolve("glPixelStoref");
  fn(pname, param);
}

void GLAPIENTRY glPixelTransferf (GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLfloat param))glewOrbisResolve("glPixelTransferf");
  fn(pname, param);
}

void GLAPIENTRY glPixelTransferi (GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum pname, GLint param))glewOrbisResolve("glPixelTransferi");
  fn(pname, param);
}

void GLAPIENTRY glPixelZoom (GLfloat xfactor, GLfloat yfactor)
{
  static void (GLAPIENTRY *fn)(GLfloat xfactor, GLfloat yfactor) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat xfactor, GLfloat yfactor))glewOrbisResolve("glPixelZoom");
  fn(xfactor, yfactor);
}

void GLAPIENTRY glPointSize (GLfloat size)
{
  static void (GLAPIENTRY *fn)(GLfloat size) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat size))glewOrbisResolve("glPointSize");
  fn(size);
}

void GLAPIENTRY glPolygonMode (GLenum face, GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLenum face, GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum face, GLenum mode))glewOrbisResolve("glPolygonMode");
  fn(face, mode);
}

void GLAPIENTRY glPolygonStipple (const GLubyte *mask)
{
  static void (GLAPIENTRY *fn)(const GLubyte *mask) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLubyte *mask))glewOrbisResolve("glPolygonStipple");
  fn(mask);
}

void GLAPIENTRY glPopAttrib (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glPopAttrib");
  fn();
}

void GLAPIENTRY glPopClientAttrib (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glPopClientAttrib");
  fn();
}

void GLAPIENTRY glPopMatrix (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glPopMatrix");
  fn();
}

void GLAPIENTRY glPopName (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glPopName");
  fn();
}

void GLAPIENTRY glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
  static void (GLAPIENTRY *fn)(GLsizei n, const GLuint *textures, const GLclampf *priorities) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei n, const GLuint *textures, const GLclampf *priorities))glewOrbisResolve("glPrioritizeTextures");
  fn(n, textures, priorities);
}

void GLAPIENTRY glPushAttrib (GLbitfield mask)
{
  static void (GLAPIENTRY *fn)(GLbitfield mask) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLbitfield mask))glewOrbisResolve("glPushAttrib");
  fn(mask);
}

void GLAPIENTRY glPushClientAttrib (GLbitfield mask)
{
  static void (GLAPIENTRY *fn)(GLbitfield mask) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLbitfield mask))glewOrbisResolve("glPushClientAttrib");
  fn(mask);
}

void GLAPIENTRY glPushMatrix (void)
{
  static void (GLAPIENTRY *fn)(void) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(void))glewOrbisResolve("glPushMatrix");
  fn();
}

void GLAPIENTRY glPushName (GLuint name)
{
  static void (GLAPIENTRY *fn)(GLuint name) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLuint name))glewOrbisResolve("glPushName");
  fn(name);
}

void GLAPIENTRY glRasterPos2d (GLdouble x, GLdouble y)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y))glewOrbisResolve("glRasterPos2d");
  fn(x, y);
}

void GLAPIENTRY glRasterPos2dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glRasterPos2dv");
  fn(v);
}

void GLAPIENTRY glRasterPos2f (GLfloat x, GLfloat y)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y))glewOrbisResolve("glRasterPos2f");
  fn(x, y);
}

void GLAPIENTRY glRasterPos2fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glRasterPos2fv");
  fn(v);
}

void GLAPIENTRY glRasterPos2i (GLint x, GLint y)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y))glewOrbisResolve("glRasterPos2i");
  fn(x, y);
}

void GLAPIENTRY glRasterPos2iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glRasterPos2iv");
  fn(v);
}

void GLAPIENTRY glRasterPos2s (GLshort x, GLshort y)
{
  static void (GLAPIENTRY *fn)(GLshort x, GLshort y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x, GLshort y))glewOrbisResolve("glRasterPos2s");
  fn(x, y);
}

void GLAPIENTRY glRasterPos2sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glRasterPos2sv");
  fn(v);
}

void GLAPIENTRY glRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y, GLdouble z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y, GLdouble z))glewOrbisResolve("glRasterPos3d");
  fn(x, y, z);
}

void GLAPIENTRY glRasterPos3dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glRasterPos3dv");
  fn(v);
}

void GLAPIENTRY glRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y, GLfloat z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y, GLfloat z))glewOrbisResolve("glRasterPos3f");
  fn(x, y, z);
}

void GLAPIENTRY glRasterPos3fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glRasterPos3fv");
  fn(v);
}

void GLAPIENTRY glRasterPos3i (GLint x, GLint y, GLint z)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y, GLint z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y, GLint z))glewOrbisResolve("glRasterPos3i");
  fn(x, y, z);
}

void GLAPIENTRY glRasterPos3iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glRasterPos3iv");
  fn(v);
}

void GLAPIENTRY glRasterPos3s (GLshort x, GLshort y, GLshort z)
{
  static void (GLAPIENTRY *fn)(GLshort x, GLshort y, GLshort z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x, GLshort y, GLshort z))glewOrbisResolve("glRasterPos3s");
  fn(x, y, z);
}

void GLAPIENTRY glRasterPos3sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glRasterPos3sv");
  fn(v);
}

void GLAPIENTRY glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y, GLdouble z, GLdouble w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y, GLdouble z, GLdouble w))glewOrbisResolve("glRasterPos4d");
  fn(x, y, z, w);
}

void GLAPIENTRY glRasterPos4dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glRasterPos4dv");
  fn(v);
}

void GLAPIENTRY glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y, GLfloat z, GLfloat w))glewOrbisResolve("glRasterPos4f");
  fn(x, y, z, w);
}

void GLAPIENTRY glRasterPos4fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glRasterPos4fv");
  fn(v);
}

void GLAPIENTRY glRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y, GLint z, GLint w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y, GLint z, GLint w))glewOrbisResolve("glRasterPos4i");
  fn(x, y, z, w);
}

void GLAPIENTRY glRasterPos4iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glRasterPos4iv");
  fn(v);
}

void GLAPIENTRY glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
  static void (GLAPIENTRY *fn)(GLshort x, GLshort y, GLshort z, GLshort w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x, GLshort y, GLshort z, GLshort w))glewOrbisResolve("glRasterPos4s");
  fn(x, y, z, w);
}

void GLAPIENTRY glRasterPos4sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glRasterPos4sv");
  fn(v);
}

void GLAPIENTRY glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
  static void (GLAPIENTRY *fn)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2))glewOrbisResolve("glRectd");
  fn(x1, y1, x2, y2);
}

void GLAPIENTRY glRectdv (const GLdouble *v1, const GLdouble *v2)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v1, const GLdouble *v2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v1, const GLdouble *v2))glewOrbisResolve("glRectdv");
  fn(v1, v2);
}

void GLAPIENTRY glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
  static void (GLAPIENTRY *fn)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2))glewOrbisResolve("glRectf");
  fn(x1, y1, x2, y2);
}

void GLAPIENTRY glRectfv (const GLfloat *v1, const GLfloat *v2)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v1, const GLfloat *v2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v1, const GLfloat *v2))glewOrbisResolve("glRectfv");
  fn(v1, v2);
}

void GLAPIENTRY glRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{
  static void (GLAPIENTRY *fn)(GLint x1, GLint y1, GLint x2, GLint y2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x1, GLint y1, GLint x2, GLint y2))glewOrbisResolve("glRecti");
  fn(x1, y1, x2, y2);
}

void GLAPIENTRY glRectiv (const GLint *v1, const GLint *v2)
{
  static void (GLAPIENTRY *fn)(const GLint *v1, const GLint *v2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v1, const GLint *v2))glewOrbisResolve("glRectiv");
  fn(v1, v2);
}

void GLAPIENTRY glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
  static void (GLAPIENTRY *fn)(GLshort x1, GLshort y1, GLshort x2, GLshort y2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x1, GLshort y1, GLshort x2, GLshort y2))glewOrbisResolve("glRects");
  fn(x1, y1, x2, y2);
}

void GLAPIENTRY glRectsv (const GLshort *v1, const GLshort *v2)
{
  static void (GLAPIENTRY *fn)(const GLshort *v1, const GLshort *v2) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v1, const GLshort *v2))glewOrbisResolve("glRectsv");
  fn(v1, v2);
}

GLint GLAPIENTRY glRenderMode (GLenum mode)
{
  static GLint (GLAPIENTRY *fn)(GLenum mode) = NULL;
  if (fn == NULL) fn = (GLint (GLAPIENTRY *)(GLenum mode))glewOrbisResolve("glRenderMode");
  return fn(mode);
}

void GLAPIENTRY glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
  static void (GLAPIENTRY *fn)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z))glewOrbisResolve("glRotated");
  fn(angle, x, y, z);
}

void GLAPIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  static void (GLAPIENTRY *fn)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z))glewOrbisResolve("glRotatef");
  fn(angle, x, y, z);
}

void GLAPIENTRY glScaled (GLdouble x, GLdouble y, GLdouble z)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y, GLdouble z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y, GLdouble z))glewOrbisResolve("glScaled");
  fn(x, y, z);
}

void GLAPIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y, GLfloat z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y, GLfloat z))glewOrbisResolve("glScalef");
  fn(x, y, z);
}

void GLAPIENTRY glSelectBuffer (GLsizei size, GLuint *buffer)
{
  static void (GLAPIENTRY *fn)(GLsizei size, GLuint *buffer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLsizei size, GLuint *buffer))glewOrbisResolve("glSelectBuffer");
  fn(size, buffer);
}

void GLAPIENTRY glShadeModel (GLenum mode)
{
  static void (GLAPIENTRY *fn)(GLenum mode) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum mode))glewOrbisResolve("glShadeModel");
  fn(mode);
}

void GLAPIENTRY glTexCoord1d (GLdouble s)
{
  static void (GLAPIENTRY *fn)(GLdouble s) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble s))glewOrbisResolve("glTexCoord1d");
  fn(s);
}

void GLAPIENTRY glTexCoord1dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glTexCoord1dv");
  fn(v);
}

void GLAPIENTRY glTexCoord1f (GLfloat s)
{
  static void (GLAPIENTRY *fn)(GLfloat s) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat s))glewOrbisResolve("glTexCoord1f");
  fn(s);
}

void GLAPIENTRY glTexCoord1fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glTexCoord1fv");
  fn(v);
}

void GLAPIENTRY glTexCoord1i (GLint s)
{
  static void (GLAPIENTRY *fn)(GLint s) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint s))glewOrbisResolve("glTexCoord1i");
  fn(s);
}

void GLAPIENTRY glTexCoord1iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glTexCoord1iv");
  fn(v);
}

void GLAPIENTRY glTexCoord1s (GLshort s)
{
  static void (GLAPIENTRY *fn)(GLshort s) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort s))glewOrbisResolve("glTexCoord1s");
  fn(s);
}

void GLAPIENTRY glTexCoord1sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glTexCoord1sv");
  fn(v);
}

void GLAPIENTRY glTexCoord2d (GLdouble s, GLdouble t)
{
  static void (GLAPIENTRY *fn)(GLdouble s, GLdouble t) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble s, GLdouble t))glewOrbisResolve("glTexCoord2d");
  fn(s, t);
}

void GLAPIENTRY glTexCoord2dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glTexCoord2dv");
  fn(v);
}

void GLAPIENTRY glTexCoord2f (GLfloat s, GLfloat t)
{
  static void (GLAPIENTRY *fn)(GLfloat s, GLfloat t) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat s, GLfloat t))glewOrbisResolve("glTexCoord2f");
  fn(s, t);
}

void GLAPIENTRY glTexCoord2fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glTexCoord2fv");
  fn(v);
}

void GLAPIENTRY glTexCoord2i (GLint s, GLint t)
{
  static void (GLAPIENTRY *fn)(GLint s, GLint t) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint s, GLint t))glewOrbisResolve("glTexCoord2i");
  fn(s, t);
}

void GLAPIENTRY glTexCoord2iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glTexCoord2iv");
  fn(v);
}

void GLAPIENTRY glTexCoord2s (GLshort s, GLshort t)
{
  static void (GLAPIENTRY *fn)(GLshort s, GLshort t) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort s, GLshort t))glewOrbisResolve("glTexCoord2s");
  fn(s, t);
}

void GLAPIENTRY glTexCoord2sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glTexCoord2sv");
  fn(v);
}

void GLAPIENTRY glTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{
  static void (GLAPIENTRY *fn)(GLdouble s, GLdouble t, GLdouble r) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble s, GLdouble t, GLdouble r))glewOrbisResolve("glTexCoord3d");
  fn(s, t, r);
}

void GLAPIENTRY glTexCoord3dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glTexCoord3dv");
  fn(v);
}

void GLAPIENTRY glTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{
  static void (GLAPIENTRY *fn)(GLfloat s, GLfloat t, GLfloat r) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat s, GLfloat t, GLfloat r))glewOrbisResolve("glTexCoord3f");
  fn(s, t, r);
}

void GLAPIENTRY glTexCoord3fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glTexCoord3fv");
  fn(v);
}

void GLAPIENTRY glTexCoord3i (GLint s, GLint t, GLint r)
{
  static void (GLAPIENTRY *fn)(GLint s, GLint t, GLint r) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint s, GLint t, GLint r))glewOrbisResolve("glTexCoord3i");
  fn(s, t, r);
}

void GLAPIENTRY glTexCoord3iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glTexCoord3iv");
  fn(v);
}

void GLAPIENTRY glTexCoord3s (GLshort s, GLshort t, GLshort r)
{
  static void (GLAPIENTRY *fn)(GLshort s, GLshort t, GLshort r) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort s, GLshort t, GLshort r))glewOrbisResolve("glTexCoord3s");
  fn(s, t, r);
}

void GLAPIENTRY glTexCoord3sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glTexCoord3sv");
  fn(v);
}

void GLAPIENTRY glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
  static void (GLAPIENTRY *fn)(GLdouble s, GLdouble t, GLdouble r, GLdouble q) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble s, GLdouble t, GLdouble r, GLdouble q))glewOrbisResolve("glTexCoord4d");
  fn(s, t, r, q);
}

void GLAPIENTRY glTexCoord4dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glTexCoord4dv");
  fn(v);
}

void GLAPIENTRY glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
  static void (GLAPIENTRY *fn)(GLfloat s, GLfloat t, GLfloat r, GLfloat q) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat s, GLfloat t, GLfloat r, GLfloat q))glewOrbisResolve("glTexCoord4f");
  fn(s, t, r, q);
}

void GLAPIENTRY glTexCoord4fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glTexCoord4fv");
  fn(v);
}

void GLAPIENTRY glTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{
  static void (GLAPIENTRY *fn)(GLint s, GLint t, GLint r, GLint q) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint s, GLint t, GLint r, GLint q))glewOrbisResolve("glTexCoord4i");
  fn(s, t, r, q);
}

void GLAPIENTRY glTexCoord4iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glTexCoord4iv");
  fn(v);
}

void GLAPIENTRY glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{
  static void (GLAPIENTRY *fn)(GLshort s, GLshort t, GLshort r, GLshort q) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort s, GLshort t, GLshort r, GLshort q))glewOrbisResolve("glTexCoord4s");
  fn(s, t, r, q);
}

void GLAPIENTRY glTexCoord4sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glTexCoord4sv");
  fn(v);
}

void GLAPIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLint size, GLenum type, GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint size, GLenum type, GLsizei stride, const void *pointer))glewOrbisResolve("glTexCoordPointer");
  fn(size, type, stride, pointer);
}

void GLAPIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum pname, GLfloat param))glewOrbisResolve("glTexEnvf");
  fn(target, pname, param);
}

void GLAPIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum pname, const GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum pname, const GLfloat *params))glewOrbisResolve("glTexEnvfv");
  fn(target, pname, params);
}

void GLAPIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum pname, GLint param))glewOrbisResolve("glTexEnvi");
  fn(target, pname, param);
}

void GLAPIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLenum pname, const GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLenum pname, const GLint *params))glewOrbisResolve("glTexEnviv");
  fn(target, pname, params);
}

void GLAPIENTRY glTexGend (GLenum coord, GLenum pname, GLdouble param)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, GLdouble param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, GLdouble param))glewOrbisResolve("glTexGend");
  fn(coord, pname, param);
}

void GLAPIENTRY glTexGendv (GLenum coord, GLenum pname, const GLdouble *params)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, const GLdouble *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, const GLdouble *params))glewOrbisResolve("glTexGendv");
  fn(coord, pname, params);
}

void GLAPIENTRY glTexGenf (GLenum coord, GLenum pname, GLfloat param)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, GLfloat param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, GLfloat param))glewOrbisResolve("glTexGenf");
  fn(coord, pname, param);
}

void GLAPIENTRY glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, const GLfloat *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, const GLfloat *params))glewOrbisResolve("glTexGenfv");
  fn(coord, pname, params);
}

void GLAPIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, GLint param) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, GLint param))glewOrbisResolve("glTexGeni");
  fn(coord, pname, param);
}

void GLAPIENTRY glTexGeniv (GLenum coord, GLenum pname, const GLint *params)
{
  static void (GLAPIENTRY *fn)(GLenum coord, GLenum pname, const GLint *params) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum coord, GLenum pname, const GLint *params))glewOrbisResolve("glTexGeniv");
  fn(coord, pname, params);
}

void GLAPIENTRY glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels))glewOrbisResolve("glTexImage1D");
  fn(target, level, internalformat, width, border, format, type, pixels);
}

void GLAPIENTRY glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
  static void (GLAPIENTRY *fn)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels))glewOrbisResolve("glTexSubImage1D");
  fn(target, level, xoffset, width, format, type, pixels);
}

void GLAPIENTRY glTranslated (GLdouble x, GLdouble y, GLdouble z)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y, GLdouble z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y, GLdouble z))glewOrbisResolve("glTranslated");
  fn(x, y, z);
}

void GLAPIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y, GLfloat z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y, GLfloat z))glewOrbisResolve("glTranslatef");
  fn(x, y, z);
}

void GLAPIENTRY glVertex2d (GLdouble x, GLdouble y)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y))glewOrbisResolve("glVertex2d");
  fn(x, y);
}

void GLAPIENTRY glVertex2dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glVertex2dv");
  fn(v);
}

void GLAPIENTRY glVertex2f (GLfloat x, GLfloat y)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y))glewOrbisResolve("glVertex2f");
  fn(x, y);
}

void GLAPIENTRY glVertex2fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glVertex2fv");
  fn(v);
}

void GLAPIENTRY glVertex2i (GLint x, GLint y)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y))glewOrbisResolve("glVertex2i");
  fn(x, y);
}

void GLAPIENTRY glVertex2iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glVertex2iv");
  fn(v);
}

void GLAPIENTRY glVertex2s (GLshort x, GLshort y)
{
  static void (GLAPIENTRY *fn)(GLshort x, GLshort y) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x, GLshort y))glewOrbisResolve("glVertex2s");
  fn(x, y);
}

void GLAPIENTRY glVertex2sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glVertex2sv");
  fn(v);
}

void GLAPIENTRY glVertex3d (GLdouble x, GLdouble y, GLdouble z)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y, GLdouble z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y, GLdouble z))glewOrbisResolve("glVertex3d");
  fn(x, y, z);
}

void GLAPIENTRY glVertex3dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glVertex3dv");
  fn(v);
}

void GLAPIENTRY glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y, GLfloat z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y, GLfloat z))glewOrbisResolve("glVertex3f");
  fn(x, y, z);
}

void GLAPIENTRY glVertex3fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glVertex3fv");
  fn(v);
}

void GLAPIENTRY glVertex3i (GLint x, GLint y, GLint z)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y, GLint z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y, GLint z))glewOrbisResolve("glVertex3i");
  fn(x, y, z);
}

void GLAPIENTRY glVertex3iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glVertex3iv");
  fn(v);
}

void GLAPIENTRY glVertex3s (GLshort x, GLshort y, GLshort z)
{
  static void (GLAPIENTRY *fn)(GLshort x, GLshort y, GLshort z) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x, GLshort y, GLshort z))glewOrbisResolve("glVertex3s");
  fn(x, y, z);
}

void GLAPIENTRY glVertex3sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glVertex3sv");
  fn(v);
}

void GLAPIENTRY glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
  static void (GLAPIENTRY *fn)(GLdouble x, GLdouble y, GLdouble z, GLdouble w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLdouble x, GLdouble y, GLdouble z, GLdouble w))glewOrbisResolve("glVertex4d");
  fn(x, y, z, w);
}

void GLAPIENTRY glVertex4dv (const GLdouble *v)
{
  static void (GLAPIENTRY *fn)(const GLdouble *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLdouble *v))glewOrbisResolve("glVertex4dv");
  fn(v);
}

void GLAPIENTRY glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
  static void (GLAPIENTRY *fn)(GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLfloat x, GLfloat y, GLfloat z, GLfloat w))glewOrbisResolve("glVertex4f");
  fn(x, y, z, w);
}

void GLAPIENTRY glVertex4fv (const GLfloat *v)
{
  static void (GLAPIENTRY *fn)(const GLfloat *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLfloat *v))glewOrbisResolve("glVertex4fv");
  fn(v);
}

void GLAPIENTRY glVertex4i (GLint x, GLint y, GLint z, GLint w)
{
  static void (GLAPIENTRY *fn)(GLint x, GLint y, GLint z, GLint w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint x, GLint y, GLint z, GLint w))glewOrbisResolve("glVertex4i");
  fn(x, y, z, w);
}

void GLAPIENTRY glVertex4iv (const GLint *v)
{
  static void (GLAPIENTRY *fn)(const GLint *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLint *v))glewOrbisResolve("glVertex4iv");
  fn(v);
}

void GLAPIENTRY glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{
  static void (GLAPIENTRY *fn)(GLshort x, GLshort y, GLshort z, GLshort w) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLshort x, GLshort y, GLshort z, GLshort w))glewOrbisResolve("glVertex4s");
  fn(x, y, z, w);
}

void GLAPIENTRY glVertex4sv (const GLshort *v)
{
  static void (GLAPIENTRY *fn)(const GLshort *v) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(const GLshort *v))glewOrbisResolve("glVertex4sv");
  fn(v);
}

void GLAPIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const void *pointer)
{
  static void (GLAPIENTRY *fn)(GLint size, GLenum type, GLsizei stride, const void *pointer) = NULL;
  if (fn == NULL) fn = (void (GLAPIENTRY *)(GLint size, GLenum type, GLsizei stride, const void *pointer))glewOrbisResolve("glVertexPointer");
  fn(size, type, stride, pointer);
}

#else

typedef int glew_orbis_gl11_unused;

#endif /* GLEW_EGL && __ORBIS__ */
