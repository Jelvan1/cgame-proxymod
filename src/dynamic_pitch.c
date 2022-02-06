#include "dynamic_pitch.h"

#include "cg_cvar.h"
#include "cg_draw.h"
#include "cg_utils.h"
#include "help.h"

#include <stdlib.h>

static vmCvar_t dynamic_pitch;
static vmCvar_t dynamic_pitch_xw;
static vmCvar_t dynamic_pitch_max_rgba;
static vmCvar_t dynamic_pitch_submax_rgba;
static vmCvar_t dynamic_pitch_badmax_rgba;

static cvarTable_t dynamic_pitch_cvars[] = {
  { &dynamic_pitch, "mdd_dynamic_pitch", "0", CVAR_ARCHIVE_ND },
  { &dynamic_pitch_xw, "mdd_dynamic_pitch_xw", "316 8", CVAR_ARCHIVE_ND },
  { &dynamic_pitch_max_rgba, "mdd_dynamic_pitch_max_rgba", ".2 .8 .2 .9", CVAR_ARCHIVE_ND },
  { &dynamic_pitch_submax_rgba, "mdd_dynamic_pitch_submax_rgba", ".8 .6 .2 .7", CVAR_ARCHIVE_ND },
  { &dynamic_pitch_badmax_rgba, "mdd_dynamic_pitch_badmax_rgba", ".9 .2 .2 .6", CVAR_ARCHIVE_ND },
};

static help_t dynamic_pitch_help[] = {
  {
    dynamic_pitch_cvars + 1,
    X | W,
    {
      "mdd_dynamic_pitch_xw X X ",
    },
  },
  {
    dynamic_pitch_cvars + 2,
    RGBA,
    {
      "mdd_dynamic_pitch_max_rgba X X X X",
    },
  },
  {
    dynamic_pitch_cvars + 3,
    RGBA,
    {
      "mdd_dynamic_pitch_submax_rgba X X X X",
    },
  },
  {
    dynamic_pitch_cvars + 4,
    RGBA,
    {
      "mdd_dynamic_pitch_badmax_rgba X X X X",
    },
  },
};

void init_dynamic_pitch(void)
{
  init_cvars(dynamic_pitch_cvars, ARRAY_LEN(dynamic_pitch_cvars));
  init_help(dynamic_pitch_help, ARRAY_LEN(dynamic_pitch_help));
}

void update_dynamic_pitch(void)
{
  update_cvars(dynamic_pitch_cvars, ARRAY_LEN(dynamic_pitch_cvars));
}

typedef struct
{
  vec3_t graph_xw;
  vec4_t max_graph_rgba;
  vec4_t submax_graph_rgba;
  vec4_t badmax_graph_rgba;

  playerState_t pm_ps;
} dynamic_pitch_t;

static dynamic_pitch_t s;

void draw_dynamic_pitch(void)
{
  if (!dynamic_pitch.integer) return;

  ParseVec(dynamic_pitch_xw.string, s.graph_xw, 2);
  ParseVec(dynamic_pitch_max_rgba.string, s.max_graph_rgba, 4);
  ParseVec(dynamic_pitch_submax_rgba.string, s.submax_graph_rgba, 4);
  ParseVec(dynamic_pitch_badmax_rgba.string, s.badmax_graph_rgba, 4);

  s.pm_ps = *getPs();

  float const yaw = s.pm_ps.viewangles[YAW];

  // find the optimal vertical climbing angle, this depends on two things:
  // 1. the player's origin
  // 2. the player's view direction
  // angle indexes
  // 1 -> 70.2 - 70.6     (1476u, submax)
  //      70.7 - 72.6     (1487u, max)
  // 2 -> 71.7 - 72.1     (1475u, submax)
  //      72.2 - 72.6     (1487u, max)
  // 3 -> 73.4 - 73.5     (1475u, submax)
  //      73.6 - 74.0     (1487u, max)
  // 4 -> 73.4 - 74.0     (1112u, badmax this is the bad one)
  int optimal_angle;
  if (s.pm_ps.origin[0] >= 0 && s.pm_ps.origin[1] >= 0)
  {
    // NE
    if (yaw >= -45.0f && yaw < 45.0f)
    {
      optimal_angle = 2;
    }
    else if (yaw >= 45.0f && yaw < 135.0f)
    {
      optimal_angle = 2;
    }
    else if (yaw >= 135.0f || yaw < -135.0f)
    {
      optimal_angle = 3;
    }
    else
    {
      optimal_angle = 3;
    }
  }
  else if (s.pm_ps.origin[0] < 0 && s.pm_ps.origin[1] >= 0)
  {
    // NW
    if (yaw >= -45.0f && yaw < 45.0f)
    {
      optimal_angle = 4;
    }
    else if (yaw >= 45.0f && yaw < 135.0f)
    {
      optimal_angle = 2;
    }
    else if (yaw >= 135.0f || yaw < -135.0f)
    {
      optimal_angle = 1;
    }
    else
    {
      optimal_angle = 3;
    }
  }
  else if (s.pm_ps.origin[0] < 0 && s.pm_ps.origin[1] < 0)
  {
    // SW
    if (yaw >= -45.0f && yaw < 45.0f)
    {
      optimal_angle = 4;
    }
    else if (yaw >= 45.0f && yaw < 135.0f)
    {
      optimal_angle = 4;
    }
    else if (yaw >= 135.0f || yaw < -135.0f)
    {
      optimal_angle = 1;
    }
    else
    {
      optimal_angle = 1;
    }
  }
  else
  {
    // SE
    if (yaw >= -45.0f && yaw < 45.0f)
    {
      optimal_angle = 2;
    }
    else if (yaw >= 45.0f && yaw < 135.0f)
    {
      optimal_angle = 4;
    }
    else if (yaw >= 135.0f || yaw < -135.0f)
    {
      optimal_angle = 3;
    }
    else
    {
      optimal_angle = 1;
    }
  }

  float    submax_optimal_angle_min;
  float    submax_optimal_angle_max;
  qboolean submax_set = qfalse;

  float    max_optimal_angle_min;
  float    max_optimal_angle_max;
  qboolean max_set = qfalse;

  float    badmax_optimal_angle_min;
  float    badmax_optimal_angle_max;
  qboolean badmax_set = qfalse;

  if (optimal_angle == 1)
  {
    submax_optimal_angle_min = 70.2f;
    submax_optimal_angle_max = 70.6f;

    max_optimal_angle_min = 70.7f;
    max_optimal_angle_max = 72.6f;

    submax_set = qtrue;
    max_set    = qtrue;
  }
  else if (optimal_angle == 2)
  {
    submax_optimal_angle_min = 71.7f;
    submax_optimal_angle_max = 72.1f;

    max_optimal_angle_min = 72.2f;
    max_optimal_angle_max = 72.6f;

    submax_set = qtrue;
    max_set    = qtrue;
  }
  else if (optimal_angle == 3)
  {
    submax_optimal_angle_min = 73.4f;
    submax_optimal_angle_max = 73.5f;

    max_optimal_angle_min = 73.6f;
    max_optimal_angle_max = 74.0f;

    submax_set = qtrue;
    max_set    = qtrue;
  }
  else
  {
    badmax_optimal_angle_min = 73.4f;
    badmax_optimal_angle_max = 74.0f;

    badmax_set = qtrue;
  }

  float const x = s.graph_xw[0];
  float const w = s.graph_xw[1];

  float const pitch = s.pm_ps.viewangles[PITCH];

  if (submax_set)
  {
    CG_FillAnglePitch(
      DEG2RAD(submax_optimal_angle_min), DEG2RAD(submax_optimal_angle_max), DEG2RAD(pitch), x, w, s.submax_graph_rgba);
  }
  if (max_set)
  {
    CG_FillAnglePitch(
      DEG2RAD(max_optimal_angle_min), DEG2RAD(max_optimal_angle_max), DEG2RAD(pitch), x, w, s.max_graph_rgba);
  }
  if (badmax_set)
  {
    CG_FillAnglePitch(
      DEG2RAD(badmax_optimal_angle_min), DEG2RAD(badmax_optimal_angle_max), DEG2RAD(pitch), x, w, s.badmax_graph_rgba);
  }
}
