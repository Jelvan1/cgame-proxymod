#include "dynamic_pitch.h"

#include "cg_cvar.h"
#include "cg_draw.h"
#include "cg_utils.h"
#include "help.h"

#include <stdlib.h>

static vmCvar_t dynamic_pitch;
static vmCvar_t dynamic_pitch_xw;
static vmCvar_t dynamic_pitch_rgba;

static cvarTable_t dynamic_pitch_cvars[] = {
  { &dynamic_pitch, "mdd_dynamic_pitch", "0b00", CVAR_ARCHIVE_ND },
  { &dynamic_pitch_xw, "mdd_dynamic_pitch_xw", "316 8", CVAR_ARCHIVE_ND },
  { &dynamic_pitch_rgba, "mdd_dynamic_pitch_rgba", ".4 .4 .8 .6", CVAR_ARCHIVE_ND }
};

static help_t dynamic_pitch_help[] = {
  {
    dynamic_pitch_cvars + 0,
    BINARY_LITERAL,
    {
      "mdd_dynamic_pitch 0bXX",
      "                    ||",
      "                    |+-  enable dynamic_pitch",
      "                    +--- halves alpha when in sub optimal origin/angle",
    },
  },
#define DYNAMIC_PITCH_ENABLED     1
#define DYNAMIC_PITCH_SCALE_ALPHA 2
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
      "mdd_dynamic_pitch_rgba X X X X",
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
  vec4_t graph_rgba;

  playerState_t pm_ps;
} dynamic_pitch_t;

static dynamic_pitch_t s;

void draw_dynamic_pitch(void)
{
  if (!dynamic_pitch.integer) return;

  ParseVec(dynamic_pitch_xw.string, s.graph_xw, 2);
  ParseVec(dynamic_pitch_rgba.string, s.graph_rgba, 4);

  s.pm_ps = *getPs();

  float const yaw = s.pm_ps.viewangles[YAW];

  // find the optimal vertical climbing angle, this depends on two things:
  // 1. the player's origin
  // 2. the player's view direction
  // angle indexes
  // 1 -> 70.2 - 72.6                  (1476u, 1487u)
  // 2 -> 71.7 - 72.6                  (1475u, 1487u)
  // 3 -> 73.4 - 74.0                  (1475u, 1487u)
  // 4 -> 73.4 - 74.0                  (1112u, this is the bad one)
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

  float min_optimal_angle;
  float max_optimal_angle;

  //  vec4_t graph_rgba = *s.graph_rgba;
  if (optimal_angle == 1)
  {
    min_optimal_angle = 70.2f;
    max_optimal_angle = 72.6f;
  }
  else if (optimal_angle == 2)
  {
    min_optimal_angle = 71.7f;
    max_optimal_angle = 72.6f;
  }
  else if (optimal_angle == 3)
  {
    min_optimal_angle = 73.4f;
    max_optimal_angle = 74.0f;
  }
  else
  {
    min_optimal_angle = 73.4f;
    max_optimal_angle = 74.0f;
    // graph_rgba        = vec4_t{ s.graph_rgba[0], s.graph_rgba[1], s.graph_rgba[2], s.graph_rgba[3] * 0.5f };
  }

  float const x = s.graph_xw[0];
  float const w = s.graph_xw[1];

  float const pitch = s.pm_ps.viewangles[PITCH];
  CG_FillAnglePitch(DEG2RAD(min_optimal_angle), DEG2RAD(max_optimal_angle), DEG2RAD(pitch), x, w, s.graph_rgba);
}
