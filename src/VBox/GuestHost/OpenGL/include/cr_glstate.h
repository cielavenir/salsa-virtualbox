/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_GLSTATE_H
#define CR_GLSTATE_H

/* Forward declaration since some of the state/cr_*.h files need the CRContext type */
struct CRContext;
typedef struct CRContext CRContext;

#include "cr_version.h"

#include "state/cr_buffer.h"
#include "state/cr_bufferobject.h"
#include "state/cr_client.h"
#include "state/cr_current.h"
#include "state/cr_evaluators.h"
#include "state/cr_feedback.h"
#include "state/cr_fog.h"
#include "state/cr_hint.h"
#include "state/cr_lighting.h"
#include "state/cr_limits.h"
#include "state/cr_line.h"
#include "state/cr_lists.h"
#include "state/cr_multisample.h"
#include "state/cr_occlude.h"
#include "state/cr_pixel.h"
#include "state/cr_point.h"
#include "state/cr_polygon.h"
#include "state/cr_program.h"
#include "state/cr_regcombiner.h"
#include "state/cr_stencil.h"
#include "state/cr_texture.h"
#include "state/cr_transform.h"
#include "state/cr_viewport.h"
#include "state/cr_attrib.h"
#include "state/cr_framebuffer.h"
#include "state/cr_glsl.h"

#include "state/cr_statefuncs.h"
#include "state/cr_stateerror.h"

#include "spu_dispatch_table.h"

#include <iprt/cdefs.h>

#ifndef IN_GUEST
#include <VBox/ssm.h>
#endif

#define CR_MAX_EXTENTS 256

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bit vectors describing GL state
 */
typedef struct {
    CRAttribBits      attrib;
    CRBufferBits      buffer;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObjectBits bufferobject;
#endif
    CRClientBits      client;
    CRCurrentBits     current;
    CREvaluatorBits   eval;
    CRFeedbackBits    feedback;
    CRFogBits         fog;
    CRHintBits        hint;
    CRLightingBits    lighting;
    CRLineBits        line;
    CRListsBits       lists;
    CRMultisampleBits multisample;
#if CR_ARB_occlusion_query
    CROcclusionBits   occlusion;
#endif
    CRPixelBits       pixel;
    CRPointBits       point;
    CRPolygonBits     polygon;
    CRProgramBits     program;
    CRRegCombinerBits regcombiner;
    CRSelectionBits   selection;
    CRStencilBits     stencil;
    CRTextureBits     texture;
    CRTransformBits   transform;
    CRViewportBits    viewport;
} CRStateBits;

typedef void (*CRStateFlushFunc)( void *arg );


typedef struct _CRSharedState {
    CRHashTable *textureTable;  /* all texture objects */
    CRHashTable *dlistTable;    /* all display lists */
    GLint refCount;
} CRSharedState;


/**
 * Chromium version of the state variables in OpenGL
 */
struct CRContext {
    int id;
    CRbitvalue bitid[CR_MAX_BITARRAY];
    CRbitvalue neg_bitid[CR_MAX_BITARRAY];

    CRSharedState *shared;

    GLenum     renderMode;

    GLenum     error;

    CRStateFlushFunc flush_func;
    void            *flush_arg;

    CRAttribState      attrib;
    CRBufferState      buffer;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObjectState bufferobject;
#endif
    CRClientState      client;
    CRCurrentState     current;
    CREvaluatorState   eval;
    CRExtensionState   extensions;
    CRFeedbackState    feedback;
    CRFogState         fog;
    CRHintState        hint;
    CRLightingState    lighting;
    CRLimitsState      limits;
    CRLineState        line;
    CRListsState       lists;
    CRMultisampleState multisample;
#if CR_ARB_occlusion_query
    CROcclusionState   occlusion;
#endif
    CRPixelState       pixel;
    CRPointState       point;
    CRPolygonState     polygon;
    CRProgramState     program;
    CRRegCombinerState regcombiner;
    CRSelectionState   selection;
    CRStencilState     stencil;
    CRTextureState     texture;
    CRTransformState   transform;
    CRViewportState    viewport;

#ifdef CR_EXT_framebuffer_object
    CRFramebufferObjectState    framebufferobject;
#endif

#ifdef CR_OPENGL_VERSION_2_0
    CRGLSLState        glsl;
#endif

    /*@todo add back buffer, depth and fbos and move out of here*/
    GLvoid *pImage; /*stored front buffer image*/

    /** For buffering vertices for selection/feedback */
    /*@{*/
    GLuint    vCount;
    CRVertex  vBuffer[4];
    GLboolean lineReset;
    GLboolean lineLoop;
    /*@}*/
};


DECLEXPORT(void) crStateInit(void);
DECLEXPORT(CRContext *) crStateCreateContext(const CRLimitsState *limits, GLint visBits, CRContext *share);
DECLEXPORT(CRContext *) crStateCreateContextEx(const CRLimitsState *limits, GLint visBits, CRContext *share, GLint presetID);
DECLEXPORT(void) crStateMakeCurrent(CRContext *ctx);
DECLEXPORT(void) crStateSetCurrent(CRContext *ctx);
DECLEXPORT(CRContext *) crStateGetCurrent(void);
DECLEXPORT(void) crStateDestroyContext(CRContext *ctx);

DECLEXPORT(void) crStateFlushFunc( CRStateFlushFunc ff );
DECLEXPORT(void) crStateFlushArg( void *arg );
DECLEXPORT(void) crStateDiffAPI( SPUDispatchTable *api );
DECLEXPORT(void) crStateUpdateColorBits( void );

DECLEXPORT(void) crStateSetCurrentPointers( CRContext *ctx, CRCurrentStatePointers *current );
DECLEXPORT(void) crStateResetCurrentPointers( CRCurrentStatePointers *current );

DECLEXPORT(void) crStateSetExtensionString( CRContext *ctx, const GLubyte *extensions );

DECLEXPORT(void) crStateDiffContext( CRContext *from, CRContext *to );
DECLEXPORT(void) crStateSwitchContext( CRContext *from, CRContext *to );
DECLEXPORT(void) crStateApplyFBImage(CRContext *to);

#ifndef IN_GUEST
DECLEXPORT(int32_t) crStateSaveContext(CRContext *pContext, PSSMHANDLE pSSM);
DECLEXPORT(int32_t) crStateLoadContext(CRContext *pContext, PSSMHANDLE pSSM);
#endif


   /* XXX move these! */

DECLEXPORT(void) STATE_APIENTRY
crStateChromiumParameteriCR( GLenum target, GLint value );

DECLEXPORT(void) STATE_APIENTRY
crStateChromiumParameterfCR( GLenum target, GLfloat value );

DECLEXPORT(void) STATE_APIENTRY
crStateChromiumParametervCR( GLenum target, GLenum type, GLsizei count, const GLvoid *values );

DECLEXPORT(void) STATE_APIENTRY
crStateGetChromiumParametervCR( GLenum target, GLuint index, GLenum type,
                                GLsizei count, GLvoid *values );

DECLEXPORT(void) STATE_APIENTRY
crStateReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type, GLvoid *pixels );

#ifdef __cplusplus
}
#endif

#endif /* CR_GLSTATE_H */