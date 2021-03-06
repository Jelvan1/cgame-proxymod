#ifndef G_PUBLIC_H
#define G_PUBLIC_H

#include "q_shared.h"

// g_public.h -- game module information visible to server

//===============================================================

typedef struct
{
  // incomplete
  qboolean linked; // qfalse if not in any good cluster
  int32_t  linkcount;

  int32_t svFlags; // SVF_NOCLIENT, SVF_BROADCAST, etc

  // only send to this client when SVF_SINGLECLIENT is set
  // if SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce
  // this)
  int32_t singleClient;

  qboolean bmodel; // if false, assume an explicit mins / maxs bounding box
                   // only set by trap_SetBrushModel
  vec3_t  mins, maxs;
  int32_t contents; // CONTENTS_TRIGGER, CONTENTS_SOLID, CONTENTS_BODY, etc
                    // a non-solid entity should set to 0

  vec3_t absmin, absmax; // derived from mins/maxs and origin + rotation

  // currentOrigin will be used for all collision detection and world linking.
  // it will not necessarily be the same as the trajectory evaluation for the current
  // time, because each entity must be moved one at a time after time is advanced
  // to avoid simultaneous collision issues
  vec3_t currentOrigin;
  vec3_t currentAngles;

  // when a trace call is made and passEntityNum != ENTITYNUM_NONE,
  // an ent will be excluded from testing if:
  // ent->s.number == passEntityNum (don't interact with self)
  // ent->r.ownerNum == passEntityNum (don't interact with your own missiles)
  // entity[ent->r.ownerNum].r.ownerNum == passEntityNum  (don't interact with other missiles from owner)
  int32_t ownerNum;
} entityShared_t;

#endif // G_PUBLIC_H
