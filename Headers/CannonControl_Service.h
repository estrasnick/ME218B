/****************************************************************************
CannonControl_Service
 ****************************************************************************/

#ifndef CannonControl_H
#define CannonControl_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitCannonControlService ( uint8_t Priority );
bool PostCannonControlService( ES_Event ThisEvent );
ES_Event RunCannonControlService( ES_Event ThisEvent );

void setTargetCannonSpeed(uint32_t newCannonRPM);
void CannonControl_PeriodicInterruptResponse(void);
void CannonEncoder_InterruptResponse(void);

#endif 

