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
#ifndef CG_MAIN_H
#define CG_MAIN_H

#include "ExportImport.h"

#include <stdint.h>

typedef enum
{
  CG_INIT,
  // void CG_Init( int32_t serverMessageNum, int32_t serverCommandSequence, int32_t clientNum )
  // called when the level loads or when the renderer is restarted
  // all media should be registered at this time
  // cgame will display loading status by calling SCR_Update, which
  // will call CG_DrawInformation during the loading process
  // reliableCommandSequence will be 0 on fresh loads, but higher for
  // demos, tourney restarts, or vid_restarts

  CG_SHUTDOWN,
  // void (*CG_Shutdown)( void );
  // opportunity to flush and close any open files

  CG_CONSOLE_COMMAND,
  // qboolean (*CG_ConsoleCommand)( void );
  // a console command has been issued locally that is not recognized by the
  // main game system.
  // use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
  // command is not known to the game

  CG_DRAW_ACTIVE_FRAME,
  // void (*CG_DrawActiveFrame)( int32_t serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
  // Generates and draws a game scene and status information at the given time.
  // If demoPlayback is set, local movement prediction will not be enabled

  CG_CROSSHAIR_PLAYER,
  // int32_t (*CG_CrosshairPlayer)( void );

  CG_LAST_ATTACKER,
  // int32_t (*CG_LastAttacker)( void );

  CG_KEY_EVENT,
  // void (*CG_KeyEvent)( int32_t key, qboolean down );

  CG_MOUSE_EVENT,
  // void (*CG_MouseEvent)( int32_t dx, int32_t dy );

  CG_EVENT_HANDLING
  // void (*CG_EventHandling)(int32_t type);
} cgameExport_t;

EXPORTIMPORT intptr_t vmMain(
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

#endif // CG_MAIN_H
