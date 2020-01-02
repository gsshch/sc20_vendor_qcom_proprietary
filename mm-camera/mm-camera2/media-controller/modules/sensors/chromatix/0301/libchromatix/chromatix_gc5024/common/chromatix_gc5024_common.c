/*============================================================================

  Copyright (c) 2012-2013 Qualcomm Technologies, Inc. All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.

============================================================================*/

/*============================================================================
 *                      INCLUDE FILES
 *===========================================================================*/
#include "chromatix_common.h"
//#include "sensor_dbg.h"

static chromatix_VFE_common_type chromatix_gc5024_parms = {
#include "chromatix_gc5024_common.h"
};

/*============================================================================
 * FUNCTION    - load_chromatix -
 *
 * DESCRIPTION:
 *==========================================================================*/
void *load_chromatix(void)
{
  //ALOGE("chromatix ptr %p", &chromatix_gc5024_parms);
  return &chromatix_gc5024_parms;
}
