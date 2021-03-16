#include "defrag.h"

#include "cg_local.h"
#include "q_assert.h"

#include <stdlib.h>

static defrag_t const* defrag_version;

defrag_t defrag_versions[] = {
  {
    "1.91.24",  // name
    0xF9C2764A, // crc32sum
    0x000E9D70, // pps_offset
    0x00002924, // cg_draw2d_defrag
    0x0001CB74, // cg_draw2d_vanilla
  },
  {
    "1.91.25",  // name
    0x04150518, // crc32sum
    0x000E9D98, // pps_offset
    0x00002924, // cg_draw2d_defrag
    0x0001CB95, // cg_draw2d_vanilla
  },
  {
    "1.91.26",  // name
    0xC2840107, // crc32sum
    0x000E9DA0, // pps_offset
    0x00002956, // cg_draw2d_defrag
    0x0001CBC9, // cg_draw2d_vanilla
  },
  {
    "1.91.27",  // name
    0x8D53765B, // crc32sum
    0x000EA09C, // pps_offset
    0x000029E5, // cg_draw2d_defrag
    0x0001CCF1, // cg_draw2d_vanilla
  },
};

qboolean init_defrag(uint32_t crc32sum)
{
  for (size_t i = 0, n = ARRAY_LEN(defrag_versions); i < n; ++i)
  {
    defrag_version = &defrag_versions[i];
    if (defrag_version->crc32sum == crc32sum) return qtrue;
  }
  defrag_version = NULL;

  // Report error about unsupported defrag version
  static_assert(ARRAY_LEN(defrag_versions) > 0, "");
  size_t len = strlen(defrag_versions[0].name);
  for (size_t i = 1, n = ARRAY_LEN(defrag_versions); i < n; ++i)
  {
    len += 2; // ", "
    len += strlen(defrag_versions[i].name);
  }
  char* const versions = malloc(len + 1);
  strcpy(versions, defrag_versions[0].name);
  for (size_t i = 1, n = ARRAY_LEN(defrag_versions); i < n; ++i)
  {
    strcat(versions, ", ");
    strcat(versions, defrag_versions[i].name);
  }
  trap_Error(vaf(
    "The proxymod does not support your current defrag version, it only supports %s. "
    "Download the latest version from https://q3defrag.org/files/defrag.\n",
    versions));
  return qfalse;
}

defrag_t const* defrag(void)
{
  assert(defrag_version);
  return defrag_version;
}
