#ifndef Q_SHARED_H
#define Q_SHARED_H

#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#  define vsnprintf _vsnprintf
#endif

static inline char* vaf(char const* format, ...)
{
  va_list     argptr;
  static char str[1024];

  va_start(argptr, format);
  vsnprintf(str, sizeof(str), format, argptr);
  va_end(argptr);

  return str;
}

#ifdef WIN32
#  include <windows.h>
#  ifdef linux
#    undef linux
#  endif
#else
#  include <string.h>
#endif

#include <math.h>
#include <stdint.h>

#include "ExportImport.h"

//=============================================================

typedef uint8_t byte;

typedef enum
{
  qfalse = 0,
  qtrue
} qboolean;

typedef uint32_t qhandle_t;
typedef int32_t  sfxHandle_t;
typedef int32_t  fileHandle_t;
typedef int32_t  clipHandle_t;

#define ARRAY_LEN(x) (sizeof(x) / sizeof(*(x)))
#define STRARRAY_LEN(x) (ARRAY_LEN(x) - 1)

// angle indexes
#define PITCH 0 // up / down
#define YAW 1   // left / right
#define ROLL 2  // fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS 1024  // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS 1024 // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS 1024   // max length of an individual token

#define BIG_INFO_STRING 8192 // used for system info key only

#define MAX_QPATH 64 // max length of a quake game pathname

//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES 32 // bit vector of area visibility

#define PROP_GAP_WIDTH 3
#define PROP_SPACE_WIDTH 8
#define PROP_HEIGHT 27
#define PROP_SMALL_SIZE_SCALE 0.75

#define BLINK_DIVISOR 200
#define PULSE_DIVISOR 75

#define UI_LEFT 0x00000000 // default
#define UI_CENTER 0x00000001
#define UI_RIGHT 0x00000002
#define UI_FORMATMASK 0x00000007
#define UI_SMALLFONT 0x00000010
#define UI_BIGFONT 0x00000020 // default
#define UI_GIANTFONT 0x00000040
#define UI_DROPSHADOW 0x00000800
#define UI_BLINK 0x00001000
#define UI_INVERSE 0x00002000
#define UI_PULSE 0x00004000

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef int32_t fixed4_t;
typedef int32_t fixed8_t;
typedef int32_t fixed16_t;

#ifndef M_PI
#  define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define TINYCHAR_WIDTH (SMALLCHAR_WIDTH)
#define TINYCHAR_HEIGHT (SMALLCHAR_HEIGHT / 2)

#define SMALLCHAR_WIDTH 8
#define SMALLCHAR_HEIGHT 16

#define BIGCHAR_WIDTH 16
#define BIGCHAR_HEIGHT 16

#define GIANTCHAR_WIDTH 32
#define GIANTCHAR_HEIGHT 48

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;

#define DEG2RAD(a) ((a) * ((float)M_PI / 180.f))
#define RAD2DEG(a) ((a) * (180.f / (float)M_PI))
#define RAD2SHORT(a) ((a) * (32768.f / (float)M_PI))
#define SHORT2RAD(a) ((a) * ((float)M_PI / 32768.f))

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

#if 1
#  define DotProduct(x, y) ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
#  define VectorSubtract(a, b, c) ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#  define VectorAdd(a, b, c) ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2])
#  define VectorCopy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#  define VectorScale(v, s, o) ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s))
#  define VectorMA(v, s, b, o)                                                                                         \
    ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s))
#else
#  define DotProduct(x, y) _DotProduct(x, y)
#  define VectorSubtract(a, b, c) _VectorSubtract(a, b, c)
#  define VectorAdd(a, b, c) _VectorAdd(a, b, c)
#  define VectorCopy(a, b) _VectorCopy(a, b)
#  define VectorScale(v, s, o) _VectorScale(v, s, o)
#  define VectorMA(v, s, b, o) _VectorMA(v, s, b, o)
#endif

#define VectorClear(a) ((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b) ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define VectorSet(v, x, y, z) ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))
#define Vector4Copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])

#define Byte4Copy(a, b) ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])

#define SnapVector(v)                                                                                                  \
  {                                                                                                                    \
    v[0] = ((vec_t)(int32_t)(v[0]));                                                                                   \
    v[1] = ((vec_t)(int32_t)(v[1]));                                                                                   \
    v[2] = ((vec_t)(int32_t)(v[2]));                                                                                   \
  }

static inline int32_t VectorCompare(vec3_t const v1, vec3_t const v2)
{
  if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
  {
    return 0;
  }
  return 1;
}

static inline vec_t VectorLength(vec3_t const v)
{
  return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static inline vec_t VectorLength2(vec2_t const v)
{
  return sqrtf(v[0] * v[0] + v[1] * v[1]);
}

static inline vec_t VectorLengthSquared(vec3_t const v)
{
  return (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static inline vec_t VectorLengthSquared2(vec2_t const v)
{
  return (v[0] * v[0] + v[1] * v[1]);
}

static inline vec_t Distance(vec3_t const p1, vec3_t const p2)
{
  vec3_t v;
  VectorSubtract(p2, p1, v);
  return VectorLength(v);
}

static inline vec_t DistanceSquared(vec3_t const p1, vec3_t const p2)
{
  vec3_t v;
  VectorSubtract(p2, p1, v);
  return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

static inline void CrossProduct(vec3_t const v1, vec3_t const v2, vec3_t cross)
{
  cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
  cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
  cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

vec_t VectorNormalize(vec3_t v); // returns vector length
vec_t VectorNormalize2(vec3_t const v, vec3_t out);

void AnglesToAxis(vec3_t const angles, vec3_t axis[3]);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

float   AngleNormalize360(float angle);
float   AngleNormalize180(float angle);
float   AngleNormalize2PI(float angle);
float   AngleNormalizePI(float angle);
int32_t AngleNormalize65536(int32_t angle);
int32_t AngleNormalize32768(int32_t angle);

void RotatePointAroundVector(vec3_t dst, vec3_t const dir, vec3_t const point, float degrees);

void AngleVectors(vec3_t const angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector(vec3_t dst, vec3_t const src);

//=============================================

#define MAX_TOKENLENGTH 1024

typedef struct pc_token_s
{
  int32_t type;
  int32_t subtype;
  int32_t intvalue;
  float   floatvalue;
  char    string[MAX_TOKENLENGTH];
} pc_token_t;

// mode parm for FS_FOpenFile
typedef enum
{
  FS_READ,
  FS_WRITE,
  FS_APPEND,
  FS_APPEND_SYNC
} fsMode_t;

typedef enum
{
  FS_SEEK_CUR,
  FS_SEEK_END,
  FS_SEEK_SET
} fsOrigin_t;

//=============================================

// portable case insensitive compare
int Q_stricmp(char const* s1, char const* s2);
int Q_strncmp(char const* s1, char const* s2, int n);
int Q_stricmpn(char const* s1, char const* s2, int n);

//=============================================

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define CVAR_ARCHIVE                                                                                                   \
  0x0001                       // set to cause it to be saved to vars.rc
                               // used for system variables, not for player
                               // specific configurations
#define CVAR_USERINFO 0x0002   // sent to server on connect or change
#define CVAR_SERVERINFO 0x0004 // sent in response to front end requests
#define CVAR_SYSTEMINFO 0x0008 // these cvars will be duplicated on all clients
#define CVAR_INIT                                                                                                      \
  0x0010 // don't allow change from console at all,
         // but can be set from the command line
#define CVAR_LATCH                                                                                                     \
  0x0020                         // will only change when C code next does
                                 // a Cvar_Get(), so it can't be changed
                                 // without proper initialization.  modified
                                 // will be set, even though the value hasn't
                                 // changed yet
#define CVAR_ROM 0x0040          // display only, cannot be set by user at all
#define CVAR_USER_CREATED 0x0080 // created by a set command
#define CVAR_TEMP 0x0100         // can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT 0x0200        // can not be changed if cheats are disabled
#define CVAR_NORESTART 0x0400    // do not clear when a cvar_restart is issued

#define CVAR_SERVER_CREATED 0x0800 // cvar was created by a server the client connected to.
#define CVAR_VM_CREATED 0x1000     // cvar was created exclusively in one of the VMs.
#define CVAR_PROTECTED 0x2000      // prevent modifying this var from VMs or the server

#define CVAR_NODEFAULT 0x4000 // do not write to config if matching with default value

#define CVAR_PRIVATE 0x8000 // can't be read from VM

#define CVAR_DEVELOPER 0x10000 // can be set only in developer mode

#define CVAR_ARCHIVE_ND (CVAR_ARCHIVE | CVAR_NODEFAULT)

// These flags are only returned by the Cvar_Flags() function
#define CVAR_MODIFIED 0x40000000    // Cvar was modified
#define CVAR_NONEXISTENT 0x80000000 // Cvar doesn't exist.

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s cvar_t;

struct cvar_s
{
  char*    name;
  char*    string;
  char*    resetString;   // cvar_restart will reset to this value
  char*    latchedString; // for CVAR_LATCH vars
  int      flags;
  qboolean modified;          // set each time the cvar is changed
  int      modificationCount; // incremented each time the cvar is changed
  float    value;             // atof( string )
  int      integer;           // atoi( string )
  qboolean validate;
  qboolean integral;
  float    min;
  float    max;
  char*    description;

  cvar_t* next;
  cvar_t* prev;
  cvar_t* hashNext;
  cvar_t* hashPrev;
  int     hashIndex;
};

#define MAX_CVAR_VALUE_STRING 256

typedef uint32_t cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct
{
  cvarHandle_t handle;
  int32_t      modificationCount;
  float        value;
  int32_t      integer;
  char         string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "surfaceflags.h" // shared with the q3map utility

/*
=================
PlaneTypeForNormal
=================
*/

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s
{
  vec3_t normal;
  float  dist;
  byte   type;     // for fast side tests: 0,1,2 = axial, 3 = nonaxial
  byte   signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
  byte   pad[2];
} cplane_t;

// a trace is returned when a box is swept through the world
typedef struct
{
  qboolean allsolid;     // if true, plane is not valid
  qboolean startsolid;   // if true, the initial point was in a solid area
  float    fraction;     // time completed, 1.0 = didn't hit anything
  vec3_t   endpos;       // final position
  cplane_t plane;        // surface normal at impact, transformed to world space
  int32_t  surfaceFlags; // surface hit
  int32_t  contents;     // contents on other side of surface hit
  int32_t  entityNum;    // entity the contacted sirface is a part of
} trace_t;

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

// markfragments are returned by CM_MarkFragments()
typedef struct
{
  int32_t firstPoint;
  int32_t numPoints;
} markFragment_t;

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/

#define ANGLE2SHORT(x) ((int32_t)((x)*65536 / 360) & 65535)
#define SHORT2ANGLE(x) ((x) * (360.0 / 65536))

#define GENTITYNUM_BITS 10 // don't need to send any more
#define MAX_GENTITIES (1 << GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values that are going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE (MAX_GENTITIES - 1)
#define ENTITYNUM_WORLD (MAX_GENTITIES - 2)
#define ENTITYNUM_MAX_NORMAL (MAX_GENTITIES - 2)

//=========================================================

// bit field limits
#define MAX_STATS 16
#define MAX_PERSISTANT 16
#define MAX_POWERUPS 16
#define MAX_WEAPONS 16

#define MAX_PS_EVENTS 2

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
typedef struct playerState_s
{
  int32_t commandTime; // cmd->serverTime of last executed command
  int32_t pm_type;
  int32_t bobCycle; // for view bobbing and footstep generation
  int32_t pm_flags; // ducked, jump_held, etc
  int32_t pm_time;

  vec3_t  origin;
  vec3_t  velocity;
  int32_t weaponTime;
  int32_t gravity;
  int32_t speed;
  int32_t delta_angles[3]; // add to command angles to get view direction
                           // changed by spawns, rotating objects, and teleporters

  int32_t groundEntityNum; // ENTITYNUM_NONE = in air

  int32_t legsTimer; // don't change low priority animations until this runs out
  int32_t legsAnim;  // mask off ANIM_TOGGLEBIT

  int32_t torsoTimer; // don't change low priority animations until this runs out
  int32_t torsoAnim;  // mask off ANIM_TOGGLEBIT

  int32_t movementDir; // a number 0 to 7 that represents the reletive angle
                       // of movement to the view angle (axial and diagonals)
                       // when at rest, the value will remain unchanged
                       // used to twist the legs during strafing

  vec3_t grapplePoint; // location of grapple to pull towards if PMF_GRAPPLE_PULL

  int32_t eFlags; // copied to entityState_t->eFlags

  int32_t eventSequence; // pmove generated events
  int32_t events[MAX_PS_EVENTS];
  int32_t eventParms[MAX_PS_EVENTS];

  int32_t externalEvent; // events set on player from another source
  int32_t externalEventParm;
  int32_t externalEventTime;

  int32_t clientNum; // ranges from 0 to MAX_CLIENTS-1
  int32_t weapon;    // copied to entityState_t->weapon
  int32_t weaponstate;

  vec3_t  viewangles; // for fixed views
  int32_t viewheight;

  // damage feedback
  int32_t damageEvent; // when it changes, latch the other parms
  int32_t damageYaw;
  int32_t damagePitch;
  int32_t damageCount;

  int32_t stats[MAX_STATS];
  int32_t persistant[MAX_PERSISTANT]; // stats that aren't cleared on death
  int32_t powerups[MAX_POWERUPS];     // level.time that the powerup runs out
  int32_t ammo[MAX_WEAPONS];

  int32_t generic1;
  int32_t loopSound;
  int32_t jumppad_ent; // jumppad entity hit this frame

  // not communicated over the net at all
  int32_t ping;             // server to game info for scoreboard
  int32_t pmove_framecount; // FIXME: don't transmit over the network
  int32_t jumppad_frame;
  int32_t entityEventSequence;
} playerState_t;

//====================================================================

//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ATTACK 1
#define BUTTON_TALK 2 // displays talk balloon and disables actions
#define BUTTON_USE_HOLDABLE 4
#define BUTTON_GESTURE 8
#define BUTTON_WALKING                                                                                                 \
  16 // walking can't just be inferred from MOVE_RUN
     // because a key pressed late in the frame will
     // only generate a small move value for that frame
     // walking will use different animations and
     // won't generate footsteps
#define BUTTON_AFFIRMATIVE 32
#define BUTTON_NEGATIVE 64

#define BUTTON_GETFLAG 128
#define BUTTON_GUARDBASE 256
#define BUTTON_PATROL 512
#define BUTTON_FOLLOWME 1024

#define BUTTON_ANY 2048 // any key whatsoever

#define MOVE_RUN                                                                                                       \
  120 // if forwardmove or rightmove are >= MOVE_RUN,
      // then BUTTON_WALKING should be set

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
  int32_t     serverTime;
  int32_t     angles[3];
  int32_t     buttons;
  byte        weapon; // weapon
  signed char forwardmove, rightmove, upmove;
} usercmd_t;

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL 0xffffff

typedef enum
{
  TR_STATIONARY,
  TR_INTERPOLATE, // non-parametric, but interpolate between snapshots
  TR_LINEAR,
  TR_LINEAR_STOP,
  TR_SINE, // value = base + sin( time / duration ) * delta
  TR_GRAVITY
} trType_t;

typedef struct
{
  trType_t trType;
  int32_t  trTime;
  int32_t  trDuration; // if non 0, trTime + trDuration = stop time
  vec3_t   trBase;
  vec3_t   trDelta; // velocity, etc
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large

typedef struct entityState_s
{
  int32_t number; // entity index
  int32_t eType;  // entityType_t
  int32_t eFlags;

  trajectory_t pos;  // for calculating position
  trajectory_t apos; // for calculating angles

  int32_t time;
  int32_t time2;

  vec3_t origin;
  vec3_t origin2;

  vec3_t angles;
  vec3_t angles2;

  int32_t otherEntityNum; // shotgun sources, etc
  int32_t otherEntityNum2;

  int32_t groundEntityNum; // -1 = in air

  int32_t constantLight; // r + (g<<8) + (b<<16) + (intensity<<24)
  int32_t loopSound;     // constantly loop this sound

  int32_t modelindex;
  int32_t modelindex2;
  int32_t clientNum; // 0 to (MAX_CLIENTS - 1), for players and corpses
  int32_t frame;

  int32_t solid; // for client side prediction, trap_linkentity sets this properly

  int32_t event; // impulse events -- muzzle flashes, footsteps, etc
  int32_t eventParm;

  // for players
  int32_t powerups;  // bit flags
  int32_t weapon;    // determines weapon and flash model, etc
  int32_t legsAnim;  // mask off ANIM_TOGGLEBIT
  int32_t torsoAnim; // mask off ANIM_TOGGLEBIT

  int32_t generic1;
} entityState_t;

// font support

#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct
{
  int32_t   height;      // number of scan lines
  int32_t   top;         // top of glyph in buffer
  int32_t   bottom;      // bottom of glyph in buffer
  int32_t   pitch;       // width for copying
  int32_t   xSkip;       // x adjustment
  int32_t   imageWidth;  // width of actual image
  int32_t   imageHeight; // height of actual image
  float     s;           // x offset in image where glyph starts
  float     t;           // y offset in image where glyph starts
  float     s2;
  float     t2;
  qhandle_t glyph; // handle to the shader with the glyph
  char      shaderName[32];
} glyphInfo_t;

typedef struct
{
  glyphInfo_t glyphs[GLYPHS_PER_FONT];
  float       glyphScale;
  char        name[MAX_QPATH];
} fontInfo_t;

// real time
//=============================================

typedef struct qtime_s
{
  int32_t tm_sec;   /* seconds after the minute - [0,59] */
  int32_t tm_min;   /* minutes after the hour - [0,59] */
  int32_t tm_hour;  /* hours since midnight - [0,23] */
  int32_t tm_mday;  /* day of the month - [1,31] */
  int32_t tm_mon;   /* months since January - [0,11] */
  int32_t tm_year;  /* years since 1900 */
  int32_t tm_wday;  /* days since Sunday - [0,6] */
  int32_t tm_yday;  /* days since January 1 - [0,365] */
  int32_t tm_isdst; /* daylight savings time flag */
} qtime_t;

#endif // Q_SHARED_H
