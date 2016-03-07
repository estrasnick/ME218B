/****************************************************************************
DriveTrainControl_Service header file
 ****************************************************************************/

#ifndef DriveTrainControl_H
#define DriveTrainControl_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitDriveTrainControlService ( uint8_t Priority );
bool PostDriveTrainControlService( ES_Event ThisEvent );
ES_Event RunDriveTrainControlService( ES_Event ThisEvent );

void setTargetDriveSpeed(float newRPMTarget_left, float newRPMTarget_right);
void setDriveToAlignToBucket(void);
void clearDriveAligningToBucket(void);
void setTargetEncoderTicks(uint32_t leftTicks, uint32_t rightTicks, bool negativeLeft, bool negativeRight);

void DriveEncoder_Left_InterruptResponse(void);
void DriveEncoder_Right_InterruptResponse(void);
void DriveControl_PeriodicInterruptResponse(void);

uint32_t GetLeftEncoderTicks(void);
uint32_t GetRightEncoderTicks(void);
void ResetEncoderTicks(void);
bool IsMoving(void);
void SetBackingUp(bool val);

#endif 

