/****************************************************************************
PeriscopeControl_Service
 ****************************************************************************/

#ifndef PeriscopeControl_H
#define PeriscopeControl_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitPeriscopeControlService ( uint8_t Priority );
bool PostPeriscopeControlService( ES_Event ThisEvent );
ES_Event RunPeriscopeControlService( ES_Event ThisEvent );

float GetPeriscopeAngle(void);
void PeriscopeEncoder_InterruptResponse(void);
void ResetPeriscopeEncoderTicks(void);

#endif 

