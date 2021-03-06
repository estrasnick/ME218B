/****************************************************************************
PositionLogic_Service Header file
 ****************************************************************************/

#ifndef PositionLogic_H
#define PositionLogic_H

#include "ES_Types.h"

// Public Function Prototypes

bool InitPositionLogicService ( uint8_t Priority );
bool PostPositionLogicService( ES_Event ThisEvent );
ES_Event RunPositionLogicService( ES_Event ThisEvent );

float getX(void);
float getY(void);
float getTheta(void);

void SetTargetLocation(float x, float y);
void SetMyLocation(float x, float y, float theta);
bool IsAbsolutePosition(void);
void ResetAbsolutePosition(void);
float DistanceToPoint(float TargetX, float TargetY);
float DetermineDistanceToBucket(void);
float ToAppropriateRange(float angle);
void ExecuteBackup(void);

#endif 

