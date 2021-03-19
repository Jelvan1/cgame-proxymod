#include "cg_snap.h"

#include "bg_pmove.h"
#include "cg_cvar.h"
#include "cg_draw.h"
#include "cg_local.h"
#include "cg_utils.h"
#include "help.h"
#include "q_assert.h"

static vmCvar_t snap;
static vmCvar_t snap_trueness;
static vmCvar_t snap_min_speed;
static vmCvar_t snap_scale;
static vmCvar_t snap_yh;
static vmCvar_t snap_def_rgba;
static vmCvar_t snap_alt_rgba;
static vmCvar_t snap_hl_def_rgba;
static vmCvar_t snap_hl_alt_rgba;
static vmCvar_t snap_45_def_rgba;
static vmCvar_t snap_45_alt_rgba;

static cvarTable_t snap_cvars[] = {
  { &snap, "mdd_snap", "0b00011", CVAR_ARCHIVE_ND },
  { &snap_trueness, "mdd_snap_trueness", "0b000", CVAR_ARCHIVE_ND },
  { &snap_min_speed, "mdd_snap_min_speed", "0", CVAR_ARCHIVE_ND },
  { &snap_scale, "mdd_snap_scale", "1", CVAR_ARCHIVE_ND },
  { &snap_yh, "mdd_snap_yh", "176 4", CVAR_ARCHIVE_ND },
  { &snap_def_rgba, "mdd_snap_def_rgba", ".9 .5 .7 .7", CVAR_ARCHIVE_ND },
  { &snap_alt_rgba, "mdd_snap_alt_rgba", ".05 .05 .05 .15", CVAR_ARCHIVE_ND },
  { &snap_hl_def_rgba, "mdd_snap_hl_def_rgba", ".5 .7 .9 .7", CVAR_ARCHIVE_ND },
  { &snap_hl_alt_rgba, "mdd_snap_hl_alt_rgba", ".5 .7 .9 .15", CVAR_ARCHIVE_ND },
  { &snap_45_def_rgba, "mdd_snap_45_def_rgba", ".5 .7 .9 .7", CVAR_ARCHIVE_ND },
  { &snap_45_alt_rgba, "mdd_snap_45_alt_rgba", ".05 .05 .05 .15", CVAR_ARCHIVE_ND },
};

static help_t snap_help[] = {
  {
    snap_cvars + 0,
    BINARY_LITERAL,
    {
      "mdd_snap 0bXXXXX",
      "           |||||",
      "           ||||+- draw basic hud",
      "           |||+-- highlight active zone",
      "           ||+--- draw 45deg shifted zones",
      "           |+---- accel-based color  (min/max => blue/red)",
      "           +----- accel-based height (min/max => low/high)",
    },
  },
#define SNAP_NORMAL    1
#define SNAP_HL_ACTIVE 2
#define SNAP_45        4
#define SNAP_BLUERED   8
#define SNAP_HEIGHT    16
  {
    snap_cvars + 1,
    BINARY_LITERAL,
    {
      "mdd_snap_trueness 0bXXX",
      "                    |||",
      "                    ||+- show true jump/crouch zones",
      "                    |+-- show true CPM air control zones",
      "                    +--- show true ground zones (deprecated)",
    },
  },
#define SNAP_JUMPCROUCH 1
#define SNAP_CPM        2
#define SNAP_GROUND     4 // TODO: Remove
  {
    snap_cvars + 4,
    Y | H,
    {
      "mdd_snap_yh X X",
    },
  },
  {
    snap_cvars + 5,
    RGBA,
    {
      "mdd_snap_def_rgba X X X X",
    },
  },
  {
    snap_cvars + 6,
    RGBA,
    {
      "mdd_snap_alt_rgba X X X X",
    },
  },
  {
    snap_cvars + 7,
    RGBA,
    {
      "mdd_snap_hl_def_rgba X X X X",
    },
  },
  {
    snap_cvars + 8,
    RGBA,
    {
      "mdd_snap_hl_alt_rgba X X X X",
    },
  },
  {
    snap_cvars + 9,
    RGBA,
    {
      "mdd_snap_45_def_rgba X X X X",
    },
  },
  {
    snap_cvars + 10,
    RGBA,
    {
      "mdd_snap_45_alt_rgba X X X X",
    },
  },
};

void init_snap(void)
{
  init_cvars(snap_cvars, ARRAY_LEN(snap_cvars));
  init_help(snap_help, ARRAY_LEN(snap_help));
}

void update_snap(void)
{
  update_cvars(snap_cvars, ARRAY_LEN(snap_cvars));
  snap.integer          = cvar_getInteger("mdd_snap");
  snap_trueness.integer = cvar_getInteger("mdd_snap_trueness");
}

#define MAX_SNAPHUD_ZONES_Q1                                                                                           \
  101 // Max nb of snapzones in 1 quadrant
      // => round(2.56 * 15 * 1.3) * 2 + 1 = 101
      //                 ^^   ^^^
      //                CPM   HASTE

typedef struct
{
  float         a;
  unsigned char maxAccel; // Max accel defined as
                          // => maxAccel = round(sAT)
  unsigned short zones[MAX_SNAPHUD_ZONES_Q1];
  unsigned char  xAccel[MAX_SNAPHUD_ZONES_Q1];
  unsigned char  yAccel[MAX_SNAPHUD_ZONES_Q1];
  float          absAccel[MAX_SNAPHUD_ZONES_Q1];
  float          minAbsAccel;
  float          maxAbsAccel;

  uint32_t mode;

  vec3_t m;

  vec2_t graph_yh;

  vec4_t graph_rgba[6];

  vec2_t wishvel;

  pmove_t       pm;
  playerState_t pm_ps;
  pml_t         pml;
} snap_t;

static snap_t s;

static void PmoveSingle(void);
static void PM_AirMove(void);
static void PM_WalkMove(void);

static void update_snap_state(void);
static void one_snap_draw(int yaw);

void draw_snap(void)
{
  if (!snap.integer) return;

  s.pm_ps = *getPs();

  s.pm.tracemask = s.pm_ps.pm_type == PM_DEAD ? MASK_PLAYERSOLID & ~CONTENTS_BODY : MASK_PLAYERSOLID;

  if (VectorLengthSquared2(s.pm_ps.velocity) >= snap_min_speed.value * snap_min_speed.value) PmoveSingle();
}

static void PmoveSingle(void)
{
  int8_t const scale = s.pm_ps.stats[13] & PSF_USERINPUT_WALK ? 64 : 127;
  if (!cg.demoPlayback && !(s.pm_ps.pm_flags & PMF_FOLLOW))
  {
    int32_t const cmdNum = trap_GetCurrentCmdNumber();
    trap_GetUserCmd(cmdNum, &s.pm.cmd);
  }
  else
  {
    s.pm.cmd.forwardmove = scale * ((s.pm_ps.stats[13] & PSF_USERINPUT_FORWARD) / PSF_USERINPUT_FORWARD -
                                    (s.pm_ps.stats[13] & PSF_USERINPUT_BACKWARD) / PSF_USERINPUT_BACKWARD);
    s.pm.cmd.rightmove   = scale * ((s.pm_ps.stats[13] & PSF_USERINPUT_RIGHT) / PSF_USERINPUT_RIGHT -
                                  (s.pm_ps.stats[13] & PSF_USERINPUT_LEFT) / PSF_USERINPUT_LEFT);
    s.pm.cmd.upmove      = scale * ((s.pm_ps.stats[13] & PSF_USERINPUT_JUMP) / PSF_USERINPUT_JUMP -
                               (s.pm_ps.stats[13] & PSF_USERINPUT_CROUCH) / PSF_USERINPUT_CROUCH);
  }

  // clear all pmove local vars
  memset(&s.pml, 0, sizeof(s.pml));

  // save old velocity for crashlanding
  VectorCopy(s.pm_ps.velocity, s.pml.previous_velocity);

  AngleVectors(s.pm_ps.viewangles, s.pml.forward, s.pml.right, s.pml.up);

  if (s.pm.cmd.upmove < 10)
  {
    // not holding jump
    s.pm_ps.pm_flags &= ~PMF_JUMP_HELD;
  }

  if (s.pm_ps.pm_type >= PM_DEAD)
  {
    s.pm.cmd.forwardmove = 0;
    s.pm.cmd.rightmove   = 0;
    s.pm.cmd.upmove      = 0;
  }

  // Use default key combination when no user input
  if (!s.pm.cmd.forwardmove && !s.pm.cmd.rightmove)
  {
    s.pm.cmd.forwardmove = scale;
    s.pm.cmd.rightmove   = scale;
  }

  // set mins, maxs, and viewheight
  PM_CheckDuck(&s.pm, &s.pm_ps);

  // set watertype, and waterlevel
  PM_SetWaterLevel(&s.pm, &s.pm_ps);

  // set groundentity
  PM_GroundTrace(&s.pm, &s.pm_ps, &s.pml);

  // if ( s.pm_ps.pm_type == PM_DEAD ) {
  //   PM_DeadMove ();
  // }

  if (s.pm_ps.powerups[PW_FLIGHT])
  {
    // // flight powerup doesn't allow jump and has different friction
    // PM_FlyMove();
    return;
  }
  else if (s.pm_ps.pm_flags & PMF_GRAPPLE_PULL)
  {
    // PM_GrappleMove();
    // // We can wiggle a bit
    // PM_AirMove();
    return;
  }
  else if (s.pm_ps.pm_flags & PMF_TIME_WATERJUMP)
  {
    // PM_WaterJumpMove();
    return;
  }
  else if (s.pm.waterlevel > 1)
  {
    // // swimming
    // PM_WaterMove();
    return;
  }
  else if (s.pml.walking)
  {
    // walking on ground
    PM_WalkMove();
  }
  else
  {
    // airborne
    PM_AirMove();
  }
  
  // TODO: we should not go to short angles just yet and keep using floating point numbers until we start drawing. 
  // Otherwise the positions of the snapzone borders will be slightly off since we round/snap the yaw angle here.
  float const yaw = RAD2SHORT(atan2f(s.wishvel[1], s.wishvel[0]));

  one_snap_draw(AngleNormalize65536(lroundf(yaw)));
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate(float const wishspeed, float const accel)
{
  float a = accel * (snap_scale.value * wishspeed) * pm_frametime;
  if (a > 50)
    a = 50; // 2.56 * 15 * 1.3 ~ 50
            //        ^^   ^^^
            //       CPM   HASTE

  if (a != s.a)
  {
    s.a = a;
    update_snap_state();
  }
}

static void PM_SlickAccelerate(float const wishspeed, float const accel)
{
  // TODO
  // float const g          = s.pm_ps.gravity * pm_frametime;
  // float const g_squared  = g * g;
  float a = accel * (snap_scale.value * wishspeed) * pm_frametime;
  if (a > 50)
    a = 50; // 2.56 * 15 * 1.3 ~ 50
            //        ^^   ^^^
            //       CPM   HASTE

  if (a != s.a)
  {
    s.a = a;
    update_snap_state();
  }
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove(void)
{
  float const scale = snap_trueness.integer & SNAP_JUMPCROUCH ? PM_CmdScale(&s.pm_ps, &s.pm.cmd)
                                                              : PM_AltCmdScale(&s.pm_ps, &s.pm.cmd);

  // project moves down to flat plane
  s.pml.forward[2] = 0;
  s.pml.right[2]   = 0;
  VectorNormalize(s.pml.forward);
  VectorNormalize(s.pml.right);

  for (uint8_t i = 0; i < 2; ++i)
  {
    s.wishvel[i] = s.pm.cmd.forwardmove * s.pml.forward[i] + s.pm.cmd.rightmove * s.pml.right[i];
  }

  float const wishspeed = scale * VectorLength2(s.wishvel);

  if (s.pm_ps.pm_flags & PMF_PROMODE)
  {
    if (snap_trueness.integer & SNAP_CPM)
    {
      if (!s.pm.cmd.forwardmove && s.pm.cmd.rightmove)
      {
        PM_Accelerate(wishspeed > cpm_airwishspeed ? cpm_airwishspeed : wishspeed, cpm_airstrafeaccelerate);
      }
      else if (s.pm.cmd.forwardmove && !s.pm.cmd.rightmove)
      {
        // TODO
        PM_Accelerate(wishspeed, pm_airaccelerate);
        // if (s.d_max > (float)M_PI / 2)
        // AIRCONTROl
      }
      else
      {
        // TODO: forward only
        // Air control when s.pm.cmd.forwardmove != 0 && s.pm.cmd.rightmove == 0 only changes direction
        PM_Accelerate(wishspeed, pm_airaccelerate);
        // if (s.d_max > (float)M_PI / 2)
        // {
        //   float       v_squared  = VectorLengthSquared2(s.pml.previous_velocity);
        //   float const vf_squared = VectorLengthSquared2(s.pm_ps.velocity);
        //   float const a          = cpm_airstopaccelerate * wishspeed * pm_frametime;
        //   if (v_squared - vf_squared >= 2 * a * wishspeed - a * a) v_squared = vf_squared;
        //   float const vf = sqrtf(vf_squared);
        //   {
        //     float const num = v_squared - vf_squared - a * a;
        //     float const den = 2 * a * vf;
        //     s.d_max         = num >= den ? 0 : acosf(num / den);
        //   }
        // }
      }
    }
    else
    {
      // Always show the zones for holding 2 keys
      if (s.pm.cmd.forwardmove)
        s.pm.cmd.rightmove = s.pm.cmd.forwardmove;
      else
        s.pm.cmd.forwardmove = s.pm.cmd.rightmove;
      PM_Accelerate(wishspeed, pm_airaccelerate);
    }
  }
  else
  {
    PM_Accelerate(wishspeed, pm_airaccelerate);
  }

  // // we may have a ground plane that is very steep, even
  // // though we don't have a groundentity
  // // slide along the steep plane
  // if (s.pml->groundPlane)
  // {
  //   PM_ClipVelocity(vf, s.pml->groundTrace.plane.normal, vf, OVERCLIP);
  // }

#if 0
  // ZOID:  If we are on the grapple, try stair-stepping
  // this allows a player to use the grapple to pull himself
  // over a ledge
  if (s.pm_ps.pm_flags & PMF_GRAPPLE_PULL)
    PM_StepSlideMove(qtrue);
  else
    PM_SlideMove(qtrue);
#endif

  // PM_StepSlideMove(qtrue);
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove(void)
{
  if (s.pm.waterlevel > 2 && DotProduct(s.pml.forward, s.pml.groundTrace.plane.normal) > 0)
  {
    // // begin swimming
    // PM_WaterMove();
    return;
  }

  if (PM_CheckJump(&s.pm, &s.pm_ps, &s.pml))
  {
    // jumped away
    if (s.pm.waterlevel > 1)
    {
      // PM_WaterMove();
    }
    else
    {
      PM_AirMove();
    }
    return;
  }

  float const scale = snap_trueness.integer & SNAP_JUMPCROUCH ? PM_CmdScale(&s.pm_ps, &s.pm.cmd)
                                                              : PM_AltCmdScale(&s.pm_ps, &s.pm.cmd);

  // project moves down to flat plane
  s.pml.forward[2] = 0;
  s.pml.right[2]   = 0;

  // TODO: only flat ground correct now
  // // project the forward and right directions onto the ground plane
  // PM_ClipVelocity(pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP);
  // PM_ClipVelocity(pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP);
  //
  VectorNormalize(s.pml.forward);
  VectorNormalize(s.pml.right);

  for (uint8_t i = 0; i < 2; ++i)
  {
    s.wishvel[i] = s.pm.cmd.forwardmove * s.pml.forward[i] + s.pm.cmd.rightmove * s.pml.right[i];
  }

  float wishspeed = scale * VectorLength2(s.wishvel);

  // clamp the speed lower if ducking
  if (s.pm_ps.pm_flags & PMF_DUCKED && wishspeed > s.pm_ps.speed * pm_duckScale)
  {
    wishspeed = s.pm_ps.speed * pm_duckScale;
  }

  // clamp the speed lower if wading or walking on the bottom
  if (s.pm.waterlevel)
  {
    float const waterScale = 1.f - (1.f - pm_swimScale) * s.pm.waterlevel / 3.f;
    if (wishspeed > s.pm_ps.speed * waterScale)
    {
      wishspeed = s.pm_ps.speed * waterScale;
    }
  }

  // when a player gets hit, they temporarily lose
  // full control, which allows them to be moved a bit
  if (snap_trueness.integer & SNAP_GROUND)
  {
    if (s.pml.groundTrace.surfaceFlags & SURF_SLICK || s.pm_ps.pm_flags & PMF_TIME_KNOCKBACK)
    {
      PM_SlickAccelerate(wishspeed, s.pm_ps.pm_flags & PMF_PROMODE ? cpm_slickaccelerate : pm_slickaccelerate);
    }
    else
    {
      // don't reset the z velocity for slopes
      // s.pm_ps.velocity[2] = 0;
      PM_Accelerate(wishspeed, s.pm_ps.pm_flags & PMF_PROMODE ? cpm_accelerate : pm_accelerate);
    }
  }
  else
  {
    PM_Accelerate(wishspeed, pm_airaccelerate);
  }

  // // don't do anything if standing still
  // if (!s.pm_ps.velocity[0] && !s.pm_ps.velocity[1])
  // {
  //   return;
  // }

  // PM_StepSlideMove(qfalse);
}

static void update_snap_state(void)
{
  // double startTime = get_time();
  // double endTime = (double) clock() / CLOCKS_PER_SEC;
  // double timeElapsed = endTime - startTime;
  // g_syscall( CG_PRINT, vaf("Elapsed time: %.6f\n", timeElapsed));

  ASSERT_GT(s.a, 0);
  s.maxAccel             = (unsigned char)(s.a + .5f);
  unsigned char xnyAccel = (unsigned char)(s.a / sqrtf(2.f) + .5f); // xAccel and yAccel at 45deg
                                                                    // ^       ^  ^
  // Find the last shortangle in each snapzone which is smaller than 45deg (= 8192) using
  //  /asin -> increasing angles
  //  \acos -> decreasing angles
  // and concatenate those 2 sorted arrays in the upperhalf of 'zones' so we can merge them
  // in the lower half afterwards                 ^^^^^^^^^ => s.maxAccel +
  for (unsigned char i = 0; i <= xnyAccel - 1; ++i)
    s.zones[s.maxAccel + i] = 16383 - (unsigned short)(RAD2SHORT(acosf((i + .5f) / s.a)));
  for (unsigned char i = xnyAccel; i <= s.maxAccel - 1; ++i)
    s.zones[s.maxAccel + (s.maxAccel - 1) - (i - xnyAccel)] = (unsigned short)(RAD2SHORT(acosf((i + .5f) / s.a)));

  // Merge 2 sorted arrays in the lowerhalf
  unsigned char bi      = s.maxAccel + 0;          // begin i
  unsigned char ei      = s.maxAccel + xnyAccel;   // end   i
  unsigned char bj      = s.maxAccel + xnyAccel;   // begin j
  unsigned char ej      = s.maxAccel + s.maxAccel; // end   j
  unsigned char i       = bi;
  unsigned char j       = bj;
  unsigned char k       = 0;
  unsigned char xAccel_ = s.maxAccel - (j - bj);
  unsigned char yAccel_ = i - bi;
  float         absAccel_;
  s.minAbsAccel = (float)(2 * s.maxAccel); // upperbound > sqrt(2)*s.maxAccel
  s.maxAbsAccel = 0;                       // lowerbound
  while (i < ei && j < ej)
  {
    absAccel_ = sqrtf((float)(xAccel_ * xAccel_ + yAccel_ * yAccel_));
    if (absAccel_ < s.minAbsAccel) s.minAbsAccel = absAccel_;
    if (absAccel_ > s.maxAbsAccel) s.maxAbsAccel = absAccel_;
    s.xAccel[k]                    = xAccel_;
    s.yAccel[k]                    = yAccel_;
    s.absAccel[k]                  = absAccel_;
    s.xAccel[2 * s.maxAccel - k]   = yAccel_;
    s.yAccel[2 * s.maxAccel - k]   = xAccel_;
    s.absAccel[2 * s.maxAccel - k] = absAccel_;
    if (s.zones[i] < s.zones[j])
    {
      s.zones[k++] = s.zones[i++];
      yAccel_      = i - bi;
    }
    else
    {
      s.zones[k++] = s.zones[j++];
      xAccel_      = s.maxAccel - (j - bj);
    }
  }
  while (i < ei) // Store remaining elements
  {
    absAccel_ = sqrtf((float)(xAccel_ * xAccel_ + yAccel_ * yAccel_));
    if (absAccel_ < s.minAbsAccel) s.minAbsAccel = absAccel_;
    if (absAccel_ > s.maxAbsAccel) s.maxAbsAccel = absAccel_;
    s.xAccel[k]                    = xAccel_;
    s.yAccel[k]                    = yAccel_;
    s.absAccel[k]                  = absAccel_;
    s.xAccel[2 * s.maxAccel - k]   = yAccel_;
    s.yAccel[2 * s.maxAccel - k]   = xAccel_;
    s.absAccel[2 * s.maxAccel - k] = absAccel_;
    s.zones[k++]                   = s.zones[i++];
    yAccel_                        = i - bi;
  }
  while (j < ej) // Store remaining elements
  {
    absAccel_ = sqrtf((float)(xAccel_ * xAccel_ + yAccel_ * yAccel_));
    if (absAccel_ < s.minAbsAccel) s.minAbsAccel = absAccel_;
    if (absAccel_ > s.maxAbsAccel) s.maxAbsAccel = absAccel_;
    s.xAccel[k]                    = xAccel_;
    s.yAccel[k]                    = yAccel_;
    s.absAccel[k]                  = absAccel_;
    s.xAccel[2 * s.maxAccel - k]   = yAccel_;
    s.yAccel[2 * s.maxAccel - k]   = xAccel_;
    s.absAccel[2 * s.maxAccel - k] = absAccel_;
    s.zones[k++]                   = s.zones[j++];
    xAccel_                        = s.maxAccel - (j - bj);
  }
  // Fill in the acceleration of the snapzone at 45deg since we only searched for shortangles
  // smaller than 45deg (= 8192)
  absAccel_ = sqrtf(2) * xnyAccel;
  if (absAccel_ < s.minAbsAccel) s.minAbsAccel = absAccel_;
  if (absAccel_ > s.maxAbsAccel) s.maxAbsAccel = absAccel_;
  s.xAccel[k]   = xnyAccel;
  s.yAccel[k]   = xnyAccel;
  s.absAccel[k] = absAccel_;

  for (i = 0; i < s.maxAccel; ++i) s.zones[s.maxAccel + i] = 16383 - s.zones[s.maxAccel - 1 - i];
  s.zones[2 * s.maxAccel] = s.zones[0] + 16384;

  // g_syscall( CG_PRINT, vaf("%.3f %.3f\n", s.minAbsAccel, s.maxAbsAccel));

  // for (int i = 0; i < 2*s.maxAccel; i++)
  //     g_syscall( CG_PRINT, vaf("%u ", s.zones[i]));
  // g_syscall( CG_PRINT, "\n");

  // for (int i = 0; i < 2*s.maxAccel; i++)
  //     g_syscall( CG_PRINT, vaf("%.3f ", s.absAccel[i]));
  // g_syscall( CG_PRINT, "\n");
}

static void one_zone_draw(
  int const      start,
  int const      end,
  int const      yaw,
  float const    y,
  float const    h,
  vec4_t* const  def_color,
  uint8_t const  alt_color,
  qboolean const hl_color)
{
  ASSERT_LE(start, end);
  ASSERT_LE(alt_color, 1); // 0 or 1
  if (hl_color && AngleNormalize65536(yaw - start) <= AngleNormalize65536(end - start))
  {
    CG_FillAngleYaw(SHORT2RAD(start), SHORT2RAD(end), SHORT2RAD(yaw), y, h, s.graph_rgba[2 + alt_color]);
  }
  else
  {
    CG_FillAngleYaw(SHORT2RAD(start), SHORT2RAD(end), SHORT2RAD(yaw), y, h, def_color[alt_color]);
  }
}

static void one_snap_draw(int yaw)
{
  ParseVec(snap_yh.string, s.graph_yh, 2);
  for (uint8_t i = 0; i < 6; ++i) ParseVec(snap_cvars[5 + i].vmCvar->string, s.graph_rgba[i], 4);

  if (snap.integer & SNAP_BLUERED) // blue/red (min/max accel)
  {
    vec4_t colorr;
    float  diffAbsAccel = s.maxAbsAccel - s.minAbsAccel;
    for (int i = 0; i < 2 * s.maxAccel; ++i)
    {
      colorr[0] = (s.absAccel[i + 1] - s.minAbsAccel) / diffAbsAccel;
      colorr[1] = 0.f;
      colorr[2] = (s.maxAbsAccel - s.absAccel[i + 1]) / diffAbsAccel;
      colorr[3] = s.graph_rgba[0][3];
      for (int j = 0; j < 65536; j += 16384)
      {
        int const bSnap = s.zones[i] + 1 + j;
        int const eSnap = s.zones[i + 1] + 0 + j;
        one_zone_draw(bSnap, eSnap, yaw, s.graph_yh[0], s.graph_yh[1], &colorr, 0, snap.integer & SNAP_HL_ACTIVE);
      }
    }
  }
  if (snap.integer & SNAP_45) // shifted 45deg
  {
    int8_t alt_color = 0;
    for (int i = 0; i < 2 * s.maxAccel; ++i)
    {
      for (int j = 0; j < 65536; j += 16384)
      {
        int const bSnap = s.zones[i] + 1 + j;
        int const eSnap = s.zones[i + 1] + 0 + j;
        one_zone_draw(bSnap, eSnap, yaw + 8192, s.graph_yh[0], s.graph_yh[1], &s.graph_rgba[4], alt_color, qfalse);
      }
      alt_color ^= 1;
    }
  }
  if (snap.integer & SNAP_NORMAL) // normal
  {
    int8_t alt_color = 0;
    for (int i = 0; i < 2 * s.maxAccel; ++i)
    {
      for (int j = 0; j < 65536; j += 16384)
      {
        int const bSnap = s.zones[i] + 1 + j;
        int const eSnap = s.zones[i + 1] + 0 + j;
        one_zone_draw(
          bSnap, eSnap, yaw, s.graph_yh[0], s.graph_yh[1], &s.graph_rgba[0], alt_color, snap.integer & SNAP_HL_ACTIVE);
      }
      alt_color ^= 1;
    }
  }
  if (snap.integer & SNAP_HEIGHT) // heavily inspired by breadsticks' version
  {
    float       gain;
    float const diffAbsAccel = s.maxAbsAccel - s.minAbsAccel;
    for (int i = 0; i < 2 * s.maxAccel; ++i)
    {
      gain           = (s.absAccel[i + 1] - s.minAbsAccel) / diffAbsAccel;
      gain           = gain * .8f + .2f;
      float const h_ = s.graph_yh[1] * gain;
      float const y_ = s.graph_yh[0] + s.graph_yh[1] * (1.f - gain);
      for (int j = 0; j < 65536; j += 16384)
      {
        int const bSnap = s.zones[i] + 1 + j;
        int const eSnap = s.zones[i + 1] + 0 + j;
        one_zone_draw(bSnap, eSnap, yaw, y_, h_, &s.graph_rgba[0], 0, snap.integer & SNAP_HL_ACTIVE);
      }
    }
  }
}
