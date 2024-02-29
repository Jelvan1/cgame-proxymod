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
#ifndef CG_DRAW_H
#define CG_DRAW_H

#include "q_shared.h"

void CG_AdjustFrom640(float* x, float* y, float* w, float* h);
void CG_FillRect(float x, float y, float w, float h, vec4_t const color);
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);
void CG_DrawRect(float x, float y, float w, float h, float size, vec4_t const color);
void CG_DrawPic(float x, float y, float w, float h, qhandle_t hShader);

void CG_DrawChar(float x, float y, float w, float h, uint8_t ch);
void CG_DrawText(
  float        x,
  float        y,
  float        sizePx,
  char const*  string,
  vec4_t const color,
  qboolean     alignRight,
  qboolean     shadow);

void CG_Draw3DModel(
  float        x,
  float        y,
  float        w,
  float        h,
  qhandle_t    model,
  qhandle_t    skin,
  vec3_t const origin,
  vec3_t const angles);

void CG_FillAnglePitch(float start, float end, float pitch, float x, float w, vec4_t const color);
void CG_DrawLinePitch(float angle, float pitch, float x, float w, float h, vec4_t const color);

void CG_FillAnglePitch(float start, float end, float pitch, float x, float w, vec4_t const color);
void CG_FillAngleYaw(float start, float end, float yaw, float y, float h, vec4_t const color);
void CG_DrawLineYaw(float angle, float yaw, float y, float w, float h, vec4_t const color);
void CG_DrawCharYaw(float angle, float yaw, float y, float w, float h, uint8_t ch, vec4_t const color);

#endif // CG_DRAW_H
