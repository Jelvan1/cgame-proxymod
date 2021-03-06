/*
  ==============================
  Written by id software, nightmare and hk of mdd
  This file is part of mdd client proxymod.

  mdd client proxymod is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  mdd client proxymod is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with mdd client proxymod.  If not, see <http://www.gnu.org/licenses/>.
  ==============================
  Note: mdd client proxymod contains large quantities from the quake III arena source code
*/
#ifndef CG_LOCAL_H
#define CG_LOCAL_H

#include "bg_public.h"
#include "cg_public.h"
#include "q_shared.h"
#include "tr_types.h"

/* cg_vm.c */
intptr_t callVM(
  int32_t cmd,
  int32_t arg0,
  int32_t arg1,
  int32_t arg2,
  int32_t arg3,
  int32_t arg4,
  int32_t arg5,
  int32_t arg6,
  int32_t arg7,
  int32_t arg8,
  int32_t arg9,
  int32_t arg10,
  int32_t arg11);
intptr_t callVM_Destroy(void);
int32_t  setVMPtr(int32_t arg0);
int32_t  initVM(void);

#define MAX_VERTS_ON_POLY 10
#define MAX_MARK_POLYS 256

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct
{
  int32_t oldFrame;
  int32_t oldFrameTime; // time when ->oldFrame was exactly on

  int32_t frame;
  int32_t frameTime; // time when ->frame will be exactly on

  float backlerp;

  float    yawAngle;
  qboolean yawing;
  float    pitchAngle;
  qboolean pitching;

  int32_t      animationNumber; // may include ANIM_TOGGLEBIT
  animation_t* animation;
  int32_t      animationTime; // time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct
{
  lerpFrame_t legs, torso, flag;
  int32_t     painTime;
  int32_t     painDirection; // flip from 0 to 1
  int32_t     lightningFiring;

  int32_t railFireTime;

  // machinegun spinning
  float    barrelAngle;
  int32_t  barrelTime;
  qboolean barrelSpinning;
} playerEntity_t;

//=================================================

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s
{
  entityState_t currentState; // from cg.frame
  entityState_t nextState;    // from cg.nextFrame, if available
  qboolean      interpolate;  // true if next is valid to interpolate to
  qboolean      currentValid; // true if cg.frame holds this entity

  int32_t muzzleFlashTime; // move to playerEntity?
  int32_t previousEvent;
  int32_t teleportFlag;

  int32_t trailTime; // so missile trails can handle dropped initial packets
  int32_t dustTrailTime;
  int32_t miscTime;

  int32_t snapShotTime; // last time this entity was found in a snapshot

  playerEntity_t pe;

  int32_t errorTime; // decay the error from this time
  vec3_t  errorOrigin;
  vec3_t  errorAngles;

  qboolean extrapolated; // false if origin / angles is an interpolation
  vec3_t   rawOrigin;
  vec3_t   rawAngles;

  vec3_t beamEnd;

  // exact interpolated position of entity on this frame
  vec3_t lerpOrigin;
  vec3_t lerpAngles;
} centity_t;

//======================================================================

#define MAX_CONFIGSTRINGS 1024
#define MAX_GAMESTATE_CHARS 16000

typedef struct
{
  int32_t stringOffsets[MAX_CONFIGSTRINGS];
  char    stringData[MAX_GAMESTATE_CHARS];
  int32_t dataCount;
} gameState_t;

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS 16

typedef struct
{
  // incomplete
  int32_t clientNum;

  qboolean demoPlayback;

  int32_t time; // this is the time value that the client
                // is rendering at.

  // view rendering
  refdef_t refdef;
} cg_t;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct
{
  // incomplete
  qhandle_t charsetShader;
  qhandle_t charsetProp;
  qhandle_t charsetPropGlow;
  qhandle_t charsetPropB;
  qhandle_t whiteShader;

  qhandle_t deferShader;
} cgMedia_t;

typedef struct
{
  // incomplete
  gameState_t gameState;    // gamestate from server
  glconfig_t  glconfig;     // rendering configuration
  float       screenXScale; // derived from glconfig
  float       screenYScale;
  float       screenXBias;

  int32_t levelStartTime;

  cgMedia_t media;
} cgs_t;

//==============================================================================

extern cgs_t cgs;
extern cg_t  cg;

extern vmCvar_t mdd_fov;
extern vmCvar_t mdd_projection;

//
// cg_main.c
//
char const* CG_ConfigString(int32_t index);

void CG_UpdateCvars(void);

//
// cg_view.c
//
void CG_DrawActiveFrame(int32_t serverTime, stereoFrame_t stereoView, qboolean demoPlayback);

//
// cg_marks.c
//
void CG_ImpactMark(
  qhandle_t    markShader,
  vec3_t const origin,
  vec3_t const dir,
  float        orientation,
  float        r,
  float        g,
  float        b,
  float        a,
  qboolean     alphaFade,
  float        radius,
  qboolean     temporary);

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand(void);
void     CG_InitConsoleCommands(void);

//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void trap_Print(char const* fmt);

// abort the game
void trap_Error(char const* fmt);

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int32_t trap_Milliseconds(void);

// console variable interaction
void trap_Cvar_Register(vmCvar_t* vmCvar, char const* varName, char const* defaultValue, int32_t flags);
void trap_Cvar_Update(vmCvar_t* vmCvar);
void trap_Cvar_Set(char const* var_name, char const* value);
void trap_Cvar_VariableStringBuffer(char const* var_name, char* buffer, int32_t bufsize);

// ServerCommand and ConsoleCommand parameter access
int32_t trap_Argc(void);
void    trap_Argv(int32_t n, char* buffer, int32_t bufferLength);
void    trap_Args(char* buffer, int32_t bufferLength);

// filesystem access
// returns length of file
int32_t trap_FS_FOpenFile(char const* qpath, fileHandle_t* f, fsMode_t mode);
void    trap_FS_Read(void* buffer, int32_t len, fileHandle_t f);
void    trap_FS_Write(void const* buffer, int32_t len, fileHandle_t f);
void    trap_FS_FCloseFile(fileHandle_t f);
int32_t trap_FS_Seek(fileHandle_t f, long offset, int32_t origin); // fsOrigin_t

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void trap_AddCommand(char const* cmdName);
void trap_RemoveCommand(char const* cmdName);

// model collision
int32_t trap_CM_NumInlineModels(void);
int32_t trap_CM_PointContents(vec3_t const p, clipHandle_t model);
void    trap_CM_BoxTrace(
     trace_t*     results,
     vec3_t const start,
     vec3_t const end,
     vec3_t const mins,
     vec3_t const maxs,
     clipHandle_t model,
     int32_t      brushmask);

// Returns the projection of a polygon onto the solid brushes in the world
int32_t trap_CM_MarkFragments(
  int32_t         numPoints,
  vec3_t const*   points,
  vec3_t const    projection,
  int32_t         maxPoints,
  vec3_t          pointBuffer,
  int32_t         maxFragments,
  markFragment_t* fragmentBuffer);

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t trap_R_RegisterModel(char const* name);       // returns rgb axis if not found
qhandle_t trap_R_RegisterSkin(char const* name);        // returns all white if not found
qhandle_t trap_R_RegisterShader(char const* name);      // returns all white if not found
qhandle_t trap_R_RegisterShaderNoMip(char const* name); // returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void trap_R_ClearScene(void);
void trap_R_AddRefEntityToScene(refEntity_t const* re);

// polys are intended for simple wall marks, not really for doing
// significant construction
void    trap_R_AddPolyToScene(qhandle_t hShader, int32_t numVerts, polyVert_t const* verts);
void    trap_R_AddPolysToScene(qhandle_t hShader, int32_t numVerts, polyVert_t const* verts, int32_t numPolys);
void    trap_R_AddLightToScene(vec3_t const org, float intensity, float r, float g, float b);
void    trap_R_AddAdditiveLightToScene(vec3_t const org, float intensity, float r, float g, float b);
int32_t trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
void    trap_R_RenderScene(refdef_t const* fd);
void    trap_R_SetColor(float const* rgba); // NULL = 1,1,1,1
void    trap_R_DrawStretchPic(
     float     x,
     float     y,
     float     w,
     float     h,
     float     s1,
     float     t1,
     float     s2,
     float     t2,
     qhandle_t hShader);
void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void trap_GetGlconfig(glconfig_t* glconfig);

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void trap_GetGameState(gameState_t* gamestate);

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned separately so that
// snapshot latency can be calculated.
void trap_GetCurrentSnapshotNumber(int32_t* snapshotNumber, int32_t* serverTime);

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean trap_GetSnapshot(int32_t snapshotNumber, snapshot_t* snapshot);

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int32_t trap_GetCurrentCmdNumber(void);

qboolean trap_GetUserCmd(int32_t cmdNumber, usercmd_t* ucmd);

qboolean trap_GetEntityToken(char* buffer, int32_t bufferSize);

#endif // CG_LOCAL_H
