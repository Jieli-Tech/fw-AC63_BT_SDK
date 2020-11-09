#include "power_api.h"
#include "asm/power_interface.h"
#include "asm/power/p33.h"

enum {
    P3_WKUP_SRC_PCNT_OVF = 0,
    P3_WKUP_SRC_EDGE,
    P3_WKUP_SRC_SUB = 3,
    P3_WKUP_SRC_VDD50_LVD,
    P3_WKUP_SRC_CHG = 6,
    P3_WKUP_SRC_WDT_INT,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief power_wakeup_reason
 *
 * @return 唤醒原因
 */
/* ----------------------------------------------------------------------------*/
uint32_t power_wakeup_reason(void)
{
    u8 wkup_src = P3_WKUP_SRC;
    u32 reason = 0;

    if (wkup_src & BIT(P3_WKUP_SRC_PCNT_OVF)) {
        reason |= BIT(PWR_WK_REASON_PLUSE_CNT_OVERFLOW);
    }

    if (wkup_src & BIT(P3_WKUP_SRC_EDGE)) {
        u8 wkup_pnd = P3_WKUP_PND;

        for (int i = 0; i < MAX_WAKEUP_PORT; i++) {
            if (wkup_pnd & BIT(i)) {
                reason |= BIT(PWR_WK_REASON_EDGE_INDEX0 + i);
            }
        }
    }

    /* if (wkup_src & BIT(P3_WKUP_SRC_VDD50_LVD)) { */
    /* return ; */
    /* } */
    /* if (wkup_src & BIT(P3_WKUP_SRC_VDD50_LVD)) { */
    /* return  */
    /* } */
    /* if (wkup_src & BIT(P3_WKUP_SRC_WDT_INT)) { */
    /* return; */
    /* } */

    /* u8 r3_wkup_src = P2M_R3_WKUP_SRC; */

    return reason;
}
