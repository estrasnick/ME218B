/****************************************************************************
PeriscopeControl_Service header file
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
void LatchPeriscope(void);
void UnlatchPeriscope(void);
void RequireZero(void);
bool IsZeroed(void);
void SetAttemptingToStop(bool val);

#endif 

