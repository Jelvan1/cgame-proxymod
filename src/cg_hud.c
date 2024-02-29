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
#include "cg_hud.h"

#include "bbox.h"
#include "cg_ammo.h"
#include "cg_cgaz.h"
#include "cg_cvar.h"
#include "cg_entity.h"
#include "cg_gl.h"
#include "cg_jump.h"
#include "cg_local.h"
#include "cg_rl.h"
#include "cg_snap.h"
#include "cg_timer.h"
#include "compass.h"
#include "dynamic_pitch.h"
#include "help.h"
#include "pitch.h"
#include "version.h"

static vmCvar_t hud;
static vmCvar_t version;

vmCvar_t mdd_fov;
vmCvar_t mdd_projection;

static cvarTable_t hud_cvars[] = {
  { &hud, "mdd_hud", "1", CVAR_ARCHIVE_ND },
  { &version, "mdd_version", VERSION, CVAR_USERINFO | CVAR_INIT },
  { &mdd_fov, "mdd_fov", "0", CVAR_ARCHIVE_ND },
  { &mdd_projection, "mdd_projection", "0", CVAR_ARCHIVE_ND },
};

void init_hud(void)
{
  init_cvars(hud_cvars, ARRAY_LEN(hud_cvars));

  init_ammo();
  init_bbox();
  init_cgaz();
  init_compass();
  init_entityStates();
  init_gl();
  init_jump();
  init_pitch();
  init_dynamic_pitch();
  init_rl();
  init_snap();
  init_timer();
}

void del_hud(void)
{
  del_help();
}

void update_hud(void)
{
  update_cvars(hud_cvars, ARRAY_LEN(hud_cvars));

  if (!hud.integer) return;

  update_ammo();
  update_bbox();
  update_cgaz();
  update_compass();
  update_entityStates();
  update_gl();
  update_jump();
  update_pitch();
  update_dynamic_pitch();
  update_rl();
  update_snap();
  update_timer();
}

void draw_hud(void)
{
  // First check if we have models, otherwise CM_ClipHandleToModel will fail
  if (!trap_CM_NumInlineModels()) return;

  if (!hud.integer) return;

  draw_compass();
  draw_cgaz();
  draw_snap();
  draw_pitch();
  draw_dynamic_pitch();

  draw_ammo();
  draw_jump();
  draw_timer();
}
