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
#include "cg_draw.h"

#include "cg_local.h"
#include "q_assert.h"

#define RF_NOSHADOW      0x0040 // don't add stencil shadows
#define RDF_NOWORLDMODEL 0x0001 // used for player configuration screen

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640(float* x, float* y, float* w, float* h)
{
  assert(x);
  assert(y);
  assert(w);
  assert(h);
#if 0
  // adjust for wide screens
  if ( cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640 ) {
    *x += 0.5 * ( cgs.glconfig.vidWidth - ( cgs.glconfig.vidHeight * 640 / 480 ) );
  }
#endif
  // scale for screen sizes
  *x *= cgs.screenXScale;
  *y *= cgs.screenXScale; // Note that screenXScale is used to avoid widescreen stretching.
  *w *= cgs.screenXScale;
  *h *= cgs.screenXScale; // Note that screenXScale is used to avoid widescreen stretching.
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect(float x, float y, float w, float h, vec4_t const color)
{
  if (!w || !h) return;
  trap_R_SetColor(color);
  CG_AdjustFrom640(&x, &y, &w, &h);
  trap_R_DrawStretchPic(x, y, w, h, 0, 0, 0, 0, cgs.media.whiteShader);
  trap_R_SetColor(NULL);
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(float x, float y, float w, float h, float size)
{
  CG_AdjustFrom640(&x, &y, &w, &h);
  size *= cgs.screenXScale;
  trap_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
  trap_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader);
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size)
{
  CG_AdjustFrom640(&x, &y, &w, &h);
  size *= cgs.screenXScale;
  trap_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
  trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader);
}

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect(float x, float y, float w, float h, float size, vec4_t const color)
{
  trap_R_SetColor(color);
  CG_DrawTopBottom(x, y, w, h, size);
  CG_DrawSides(x, y + size, w, h - size * 2, size);
  trap_R_SetColor(NULL);
}

/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic(float x, float y, float w, float h, qhandle_t hShader)
{
  CG_AdjustFrom640(&x, &y, &w, &h);
  trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, hShader);
}

/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar(float x, float y, float w, float h, uint8_t ch)
{
  if (ch == ' ') return;

  float const frow = .0625f * (ch >> 4);
  float const fcol = .0625f * (ch & 15);
  float const size = .0625f;

  CG_AdjustFrom640(&x, &y, &w, &h);
  trap_R_DrawStretchPic(x, y, w, h, fcol, frow, fcol + size, frow + size, cgs.media.charsetShader);
}

void CG_DrawText(
  float        x,
  float        y,
  float        sizePx,
  char const*  string,
  vec4_t const color,
  qboolean     alignRight,
  qboolean     shadow)
{
  if (string == NULL) return;

  float         tmpX = x;
  int32_t const len  = (int32_t)strlen(string);

  // draw the drop shadow
  if (shadow)
  {
    trap_R_SetColor(colorBlack);
    if (alignRight)
    {
      for (int32_t i = len - 1; i >= 0; --i)
      {
        tmpX -= sizePx;
        CG_DrawChar(tmpX + 2, y + 2, sizePx, sizePx, string[i]);
      }
    }
    else
    {
      for (int32_t i = 0; i < len; ++i)
      {
        CG_DrawChar(tmpX + 2, y + 2, sizePx, sizePx, string[i]);
        tmpX += sizePx;
      }
    }
  }

  tmpX = x;
  trap_R_SetColor(color);
  if (alignRight)
  {
    for (int32_t i = len - 1; i >= 0; --i)
    {
      tmpX -= sizePx;
      CG_DrawChar(tmpX, y, sizePx, sizePx, string[i]);
    }
  }
  else
  {
    for (int32_t i = 0; i < len; ++i)
    {
      CG_DrawChar(tmpX, y, sizePx, sizePx, string[i]);
      tmpX += sizePx;
    }
  }
  trap_R_SetColor(NULL);
}

void CG_Draw3DModel(
  float        x,
  float        y,
  float        w,
  float        h,
  qhandle_t    model,
  qhandle_t    skin,
  vec3_t const origin,
  vec3_t const angles)
{
  refdef_t    refdef;
  refEntity_t ent;

  // if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
  //  return;
  // }

  CG_AdjustFrom640(&x, &y, &w, &h);

  memset(&refdef, 0, sizeof(refdef));

  memset(&ent, 0, sizeof(ent));
  AnglesToAxis(angles, ent.axis);
  VectorCopy(origin, ent.origin);
  ent.hModel     = model;
  ent.customSkin = skin;
  ent.renderfx   = RF_NOSHADOW; // no stencil shadows

  refdef.rdflags = RDF_NOWORLDMODEL;

  AxisClear(refdef.viewaxis);

  refdef.fov_x = 30;
  refdef.fov_y = 30;

  refdef.x      = (int32_t)x;
  refdef.y      = (int32_t)y;
  refdef.width  = (int32_t)w;
  refdef.height = (int32_t)h;

  refdef.time = 0; // getSnap()->serverTime;
  // refdef.time = cg.time;

  trap_R_ClearScene();
  trap_R_AddRefEntityToScene(&ent);
  trap_R_RenderScene(&refdef);
}

static inline qboolean AngleInFovY(float pitch)
{
  ASSERT_FLOAT_EQ(pitch, AngleNormalizePI(pitch));
  float const half_fov_y = cg.refdef.fov_y / 2;
  return pitch > -half_fov_y && pitch < half_fov_y;
}

static inline qboolean AngleInFovX(float yaw)
{
  ASSERT_FLOAT_EQ(yaw, AngleNormalizePI(yaw));
  float const half_fov_x = cg.refdef.fov_x / 2;
  return yaw > -half_fov_x && yaw < half_fov_x;
}

static inline float ProjectionY(float angle)
{
  ASSERT_FLOAT_EQ(angle, AngleNormalizePI(angle));
  float const half_fov_y = cg.refdef.fov_y / 2;
  if (angle >= half_fov_y) return 0;
  if (angle <= -half_fov_y) return cgs.screenHeight;

  ASSERT_TRUE(AngleInFovY(angle));
  switch (mdd_projection.integer)
  {
  case 0: // Rectilinear projection. Breaks with fov >=180.
    return cgs.screenHeight / 2 * (1 + tanf(angle) / tanf(half_fov_y));
  case 1: // Cylindrical projection. Breaks with fov >360.
    return cgs.screenHeight / 2 * (1 + angle / half_fov_y);
  case 2: // Panini projection. Breaks with fov >=360.
    return cgs.screenHeight / 2 * (1 + tanf(angle / 2) / tanf(half_fov_y / 2));
  default:
    assert(0);
    return 0;
  }
}

static inline float ProjectionX(float angle)
{
  ASSERT_FLOAT_EQ(angle, AngleNormalizePI(angle));
  float const half_fov_x = cg.refdef.fov_x / 2;
  if (angle >= half_fov_x) return 0;
  if (angle <= -half_fov_x) return cgs.screenWidth;

  ASSERT_TRUE(AngleInFovX(angle));
  switch (mdd_projection.integer)
  {
  case 0: // Rectilinear projection. Breaks with fov >=180.
    return cgs.screenWidth / 2 * (1 - tanf(angle) / tanf(half_fov_x));
  case 1: // Cylindrical projection. Breaks with fov >360.
    return cgs.screenWidth / 2 * (1 - angle / half_fov_x);
  case 2: // Panini projection. Breaks with fov >=360.
    return cgs.screenWidth / 2 * (1 - tanf(angle / 2) / tanf(half_fov_x / 2));
  default:
    assert(0);
    return 0;
  }
}

typedef struct
{
  float    x1;
  float    x2;
  qboolean split;
} range_t;

static inline range_t AnglesToRange(float start, float end, float yaw)
{
  if (fabsf(end - start) > 2 * (float)M_PI)
  {
    range_t const ret = { 0, cgs.screenWidth, qfalse };
    return ret;
  }

  qboolean split = end > start;
  start          = AngleNormalizePI(start - yaw);
  end            = AngleNormalizePI(end - yaw);

  if (end > start)
  {
    split           = !split;
    float const tmp = start;
    start           = end;
    end             = tmp;
  }

  range_t const ret = { ProjectionX(start), ProjectionX(end), split };
  return ret;
}

static inline range_t PitchAnglesToRange(float start, float end, float pitch)
{
  if (fabsf(end - start) > 2 * (float)M_PI)
  {
    range_t const ret = { 0, cgs.screenHeight, qfalse };
    return ret;
  }

  qboolean split = end > start;
  start          = AngleNormalizePI(start - pitch);
  end            = AngleNormalizePI(end - pitch);

  if (end > start)
  {
    split           = !split;
    float const tmp = start;
    start           = end;
    end             = tmp;
  }

  range_t const ret = { ProjectionY(start), ProjectionY(end), split };
  return ret;
}

void CG_DrawLinePitch(float angle, float pitch, float x, float w, float h, vec4_t const color)
{
  angle = AngleNormalizePI(angle - pitch);
  if (!AngleInFovY(angle)) return; // TODO: thick lines => if half of line goes out of screen, nothing will be drawn

  float const y = ProjectionY(angle);
  CG_FillRect(x, y - h / 2, w, h, color);
}

void CG_FillAnglePitch(float start, float end, float pitch, float x, float w, vec4_t const color)
{
  range_t const range = PitchAnglesToRange(start, end, pitch);
  if (!range.split)
  {
    CG_FillRect(x, range.x1, w, range.x2 - range.x1, color);
  }
  else
  {
    CG_FillRect(x, 0, w, range.x1, color);
    CG_FillRect(x, range.x2, w, cgs.screenHeight - range.x2, color);
  }
}

void CG_FillAngleYaw(float start, float end, float yaw, float y, float h, vec4_t const color)
{
  range_t const range = AnglesToRange(start, end, yaw);
  if (!range.split)
  {
    CG_FillRect(range.x1, y, range.x2 - range.x1, h, color);
  }
  else
  {
    CG_FillRect(0, y, range.x1, h, color);
    CG_FillRect(range.x2, y, cgs.screenWidth - range.x2, h, color);
  }
}

void CG_DrawLineYaw(float angle, float yaw, float y, float w, float h, vec4_t const color)
{
  angle = AngleNormalizePI(angle - yaw);
  if (!AngleInFovX(angle)) return; // TODO: thick lines => if half of line goes out of screen, nothing will be drawn

  float const x = ProjectionX(angle);
  CG_FillRect(x - w / 2, y, w, h, color);
}

void CG_DrawCharYaw(float angle, float yaw, float y, float w, float h, uint8_t ch, vec4_t const color)
{
  angle = AngleNormalizePI(angle - yaw);
  if (!AngleInFovX(angle)) return; // TODO: wide chars => if half of char goes out of screen, nothing will be drawn

  float const x = ProjectionX(angle);
  trap_R_SetColor(color);
  CG_DrawChar(x - w / 2, y, w, h, ch);
  trap_R_SetColor(NULL);
}
