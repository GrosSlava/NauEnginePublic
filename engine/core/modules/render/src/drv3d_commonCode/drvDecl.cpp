// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/3d/dag_drv3d.h"
//#include <generic/dag_carray.h>
#include <EASTL/utility.h>
#include <EASTL/algorithm.h>
#include "nau/osApiWrappers/dag_localConv.h"
#include "nau/util/common.h"


namespace drv3d_generic
{
bool disableResEntryRealloc = false;
};

// From https://github.com/WebKit/webkit/blob/main/Source/ThirdParty/ANGLE/src/libANGLE/renderer/driver_utils.h#L18 and
// https://gamedev.stackexchange.com/a/31626
static const eastl::pair<uint32_t, int> vendor_id_table[] = //
  {
    {0x10DE, D3D_VENDOR_NVIDIA},

    {0x1002, D3D_VENDOR_AMD},
    {0x1022, D3D_VENDOR_AMD},

    {0x8086, D3D_VENDOR_INTEL},
    {0x8087, D3D_VENDOR_INTEL},
    {0x163C, D3D_VENDOR_INTEL},

    {0x5143, D3D_VENDOR_QUALCOMM},

    {0x1010, D3D_VENDOR_IMGTEC},

    {0x13B5, D3D_VENDOR_ARM},

    {0x05AC, D3D_VENDOR_APPLE},

    {0x10005, D3D_VENDOR_MESA},

    {0x0003, D3D_VENDOR_SHIM_DRIVER},

    {0x144D, D3D_VENDOR_SAMSUNG},

    {0x0000, D3D_VENDOR_NONE},
};

static const eastl::array<const char *, D3D_VENDOR_COUNT> vendor_names = {
  "Unknown",     // D3D_VENDOR_NONE
  "Mesa",        // D3D_VENDOR_MESA
  "ImgTec",      // D3D_VENDOR_IMGTEC
  "AMD",         // D3D_VENDOR_AMD / D3D_VENDOR_ATI
  "NVIDIA",      // D3D_VENDOR_NVIDIA
  "Intel",       // D3D_VENDOR_INTEL
  "Apple",       // D3D_VENDOR_APPLE
  "Shim driver", // D3D_VENDOR_SHIM_DRIVER
  "ARM",         // D3D_VENDOR_ARM
  "Qualcomm",    // D3D_VENDOR_QUALCOMM
  "Samsung",     // D3D_VENDOR_SAMSUNG
};

void d3d_get_render_target(Driver3dRenderTarget &rt) { d3d::get_render_target(rt); }
void d3d_set_render_target(Driver3dRenderTarget &rt) { d3d::set_render_target(rt); }

void d3d_get_view_proj(ViewProjMatrixContainer &vp)
{
  d3d::gettm(TM_VIEW, vp.savedView);
  vp.p_ok = d3d::getpersp(vp.p);
  // Get proj tm also even though persp is ok. Overally there is no good having
  // uninitialized proj whereas everyone can use it
  d3d::gettm(TM_PROJ, &vp.savedProj);
}

void d3d_set_view_proj(const ViewProjMatrixContainer &vp)
{
  d3d::settm(TM_VIEW, vp.savedView);
  if (!vp.p_ok)
    d3d::settm(TM_PROJ, &vp.savedProj);
  else
    d3d::setpersp(vp.p);
}

void d3d_get_view(int &viewX, int &viewY, int &viewW, int &viewH, float &viewN, float &viewF)
{
  d3d::getview(viewX, viewY, viewW, viewH, viewN, viewF);
}
void d3d_set_view(int viewX, int viewY, int viewW, int viewH, float viewN, float viewF)
{
  d3d::setview(viewX, viewY, viewW, viewH, viewN, viewF);
  d3d::setscissor(viewX, viewY, viewW, viewH);
}

const char *d3d_get_vendor_name(int vendor)
{
  NAU_ASSERT(vendor >= 0 && vendor < vendor_names.size());
  return vendor_names[vendor];
}

int d3d_get_vendor(uint32_t vendor_id, const char *description)
{
  int vendor = D3D_VENDOR_NONE;
  auto ref =
    eastl::find_if(eastl::begin(vendor_id_table), eastl::end(vendor_id_table), [=](auto &ent) { return ent.first == vendor_id; });
  if (ref != eastl::end(vendor_id_table))
    vendor = ref->second;
  else if (description)
  {
    if (strstr(description, "ATI") || strstr(description, "AMD"))
      vendor = D3D_VENDOR_AMD;
    else
    {
      if (char *lower = str_dup(description, tmpmem))
      {
        lower = nau::hal::dd_strlwr(lower);
        if (strstr(lower, "radeon"))
          vendor = D3D_VENDOR_AMD;
        else if (strstr(lower, "geforce") || strstr(lower, "nvidia"))
          vendor = D3D_VENDOR_NVIDIA;
        else if (strstr(lower, "intel") || strstr(lower, "rdpdd")) // Assume the worst for the Microsoft Mirroring Driver - take it as
                                                                   // it mirrors to Intel driver with broken voltex compression.
          vendor = D3D_VENDOR_INTEL;
        tmpmem->deallocate(lower,0);//memfree(lower, tmpmem);
      }
    }
  }
  return vendor;
}
