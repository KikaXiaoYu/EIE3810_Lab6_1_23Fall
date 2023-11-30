#include "stm32f10x.h"
#include "EIE3810_OTHERS.h"


void EIE3810_NVIC_SetPriorityGroup(u8 prigroup)
{
    u32 temp1, temp2;
    temp2 = prigroup & 0x00000007; // get the 3-bit priority group number
    temp2 <<= 8;                   // the priority group number should be the 3 bits [10:8] in AIRCR
    temp1 = SCB->AIRCR;            // get the value of AIRCR
    temp1 &= 0x0000F8FF;           // set [15:0] to 0xF8FF, and reset [31:16] to 0x0000
                                   // to be specific, the value in [10:8] has been set to 0b1000
                                   // i.e. reset the priority group
    temp1 |= 0x05FA0000;           // set [31:16] to 0x05FA = 0b0000_0101_1111_1010
                                   // this set the VECTKEY/VECTKEYSTAT
    temp1 |= temp2;                // set the priority group to the value of prigroup
    SCB->AIRCR = temp1;            // update the AIRCR value
}


void Delay(u32 count)
{
    u32 i;
    for (i = 0; i < count; i++)
        ;
}