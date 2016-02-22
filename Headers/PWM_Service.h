/****************************************************************************
 
  Header file for template service 
  based on the Gen 2 Events and Services Framework

 ****************************************************************************/

#ifndef PWM_Service_H
#define PWM_Service_H

#include "ES_Types.h"

// Public Function Prototypes
bool InitPWMService ( uint8_t Priority );
bool PostPWMService( ES_Event ThisEvent );
ES_Event RunPWMService( ES_Event ThisEvent );

//PWM Setters
void SetPWM_DriveLeft(float duty, uint32_t direction);
void SetPWM_DriveRight(float duty, uint32_t direction);
void SetPWM_Periscope(float duty);
void SetPWM_Hopper(float duty);
void SetPWM_Cannon(float duty);

#endif 

