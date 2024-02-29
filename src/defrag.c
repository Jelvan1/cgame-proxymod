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
    0x00002948, // cg_draw2d_defrag (0x00002924)
    0x0001CBAF, // cg_draw2d_vanilla (0x0001CB74)
  },
  {
    "1.91.25",  // name
    0x04150518, // crc32sum
    0x000E9D98, // pps_offset
    0x00002948, // cg_draw2d_defrag (0x00002924)
    0x0001CBD0, // cg_draw2d_vanilla (0x0001CB95)
  },
  {
    "1.91.26",  // name
    0xC2840107, // crc32sum
    0x000E9DA0, // pps_offset
    0x0000297A, // cg_draw2d_defrag (0x00002956)
    0x0001CC04, // cg_draw2d_vanilla (0x0001CBC9)
  },
  {
    "1.91.27",  // name
    0x8D53765B, // crc32sum
    0x000EA09C, // pps_offset
    0x00002A09, // cg_draw2d_defrag (0x000029E5)
    0x0001CD2C, // cg_draw2d_vanilla (0x0001CCF1)
  },
  // 1.91.28 requires VM_MAGIC_VER2 support
  {
    "1.91.29",  // name
    0xE5F9882A, // crc32sum
    0x000EA11C, // pps_offset
    0x00002A20, // cg_draw2d_defrag (0x000029FC)
    0x0001CD87, // cg_draw2d_vanilla (0x0001CD4C)
  },
  {
    "1.91.30",  // name
    0x03BDC08B, // crc32sum
    0x000EA11C, // pps_offset
    0x00002A2C, // cg_draw2d_defrag (0x00002A08)
    0x0001CD93, // cg_draw2d_vanilla (0x0001CD58)
  },
  {
    "1.91.31",  // name
    0x50AD1672, // crc32sum
    0x000EA13C, // pps_offset
    0x00002A36, // cg_draw2d_defrag (0x00002A12)
    0x0001CDC1, // cg_draw2d_vanilla (0x0001CD86)
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
