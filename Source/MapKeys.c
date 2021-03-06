/****************************************************************************
 Module
   MapKeys.c

 Revision
   1.0.1

 Description
   This service maps keystrokes to events. Used for testing.

 Notes

 History
 When           Who     What/Why
 -------------- ---     --------
 02/06/14 14:44 jec      tweaked to be a more generic key-mapper
 02/07/12 00:00 jec      converted to service for use with E&S Gen2
 02/20/07 21:37 jec      converted to use enumerated type for events
 02/21/05 15:38 jec      Began coding
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include <stdio.h>
#include <ctype.h>
#include "ES_Configure.h"
#include "ES_Framework.h"
#include "MapKeys.h"
#include "Master_SM.h"
#include "DEFINITIONS.h"
#include "DriveTrainControl_Service.h"
#include "PeriscopeControl_Service.h"
#include "PositionLogic_Service.h"
#include "PWM_Service.h"
#include "AttackStrategy_SM.h"
#include "PhotoTransistor_Service.h"

/*----------------------------- Module Defines ----------------------------*/

/*---------------------------- Module Functions ---------------------------*/
/* prototypes for private functions for this service.They should be functions
   relevant to the behavior of this service
*/


/*---------------------------- Module Variables ---------------------------*/
// with the introduction of Gen2, we need a module level Priority variable
static uint8_t MyPriority;


/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMapKeys

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any 
     other required initialization for this service
 Notes

 Author
     J. Edward Carryer, 02/07/12, 00:04
****************************************************************************/
bool InitMapKeys ( uint8_t Priority )
{
  MyPriority = Priority;
	
  return true;
}

/****************************************************************************
 Function
     PostMapKeys

 Parameters
     EF_Event ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes

 Author
     J. Edward Carryer, 10/23/11, 19:25
****************************************************************************/
bool PostMapKeys( ES_Event ThisEvent )
{
  return ES_PostToService( MyPriority, ThisEvent);
}


/****************************************************************************
 Function
    RunMapKeys

 Parameters
   ES_Event : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   maps keys to Events for HierMuWave Example
 Notes
   
 Author
   J. Edward Carryer, 02/07/12, 00:08
****************************************************************************/
ES_Event RunMapKeys( ES_Event ThisEvent )
{
  ES_Event ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

    if ( ThisEvent.EventType == ES_NEW_KEY) // there was a key pressed
    {
        switch ( toupper(ThisEvent.EventParam))
        {
						case 'S' : ThisEvent.EventType = ES_DRIVE_HALF_SPEED; 
											printf("Commanding: ES_DRIVE_HALF_SPEED \n\r");
                       break;
            case 'D' : ThisEvent.EventType = ES_STOP_DRIVE;
											 printf("Commanding: ES_STOP_DRIVE \n\r");
                       break;
						case 'Q' : ThisEvent.EventType = ES_START_CANNON; 
											 printf("Commanding: ES_START_CANNON \n\r");                     
											 break;
						case 'W' : ThisEvent.EventType = ES_STOP_CANNON; 
											 printf("Commanding: ES_STOP_CANNON \n\r");                     
											 break;
						case 'Z' : ThisEvent.EventType = ES_START_PERISCOPE; 
											 printf("Commanding: ES_START_PERISCOPE \n\r");                     
											 break;
						case 'X' : ThisEvent.EventType = ES_STOP_PERISCOPE; 
											 printf("Commanding: ES_STOP_PERISCOPE \n\r");                     
											 break;
						case 'L' : ThisEvent.EventType = ES_MANUAL_START; 
											 printf("Commanding: ES_MANUAL_START \n\r");                     
											 break;
						case 'P' : ThisEvent.EventType = ES_PS_DETECTED; 
											 printf("Commanding: ES_PS_DETECTED, Event Param = 0 \n\r");                     
											 break;
						case 'K' : ThisEvent.EventType = ES_CALCULATE_POSITION; 
											 SetBeaconAngles(270, 180, 90);
											 printf("Commanding: ES_CALCULATE_POSITION, Event Param = 0 \n\r");                     
											 break;
						case '3' : ThisEvent.EventType = ES_NO_EVENT; 
											SetPWM_Cannon(15);
											 printf("Commanding: Start Cannon \n\r");                     
											 break;
						case '4' : ThisEvent.EventType = ES_NO_EVENT; 
											SetPWM_Cannon(0);
											 printf("Commanding: Stop Cannon \n\r");                     
											 break;
						case '1' : ThisEvent.EventType = ES_NO_EVENT; 
											printf("Load Hopper");
											SetPWM_Hopper(HOPPER_LOAD_DUTY);             
											 break;
						case '2' : ThisEvent.EventType = ES_NO_EVENT; 
											printf("Shoot Hopper");
											SetPWM_Hopper(HOPPER_DEFAULT_DUTY);             
											 break;
						case 'J' : ThisEvent.EventType = ES_NO_EVENT; 
											 static uint8_t PWM_Val_Latch = 15;
											 SetPWM_PeriscopeLatch (PWM_Val_Latch);
											 PWM_Val_Latch--;
											 ThisEvent.EventParam = 0;
											 printf("Commanding: ES_MANUAL_SHOOT, Event Param = 0 \n\r");                     
											 break;
							
						case 'E' : ThisEvent.EventType = ES_ALIGNED_TO_BUCKET; 	
											 clearDriveAligningToBucket();
											 break;
						case 'C' : ThisEvent.EventType = ES_NO_EVENT; 
											 printf("Attacking disabled\r\n");
											 disableAttacking();
											 break;
						case 'H' : ThisEvent.EventType = ES_MANUAL_SHOOT; 
												//SetPWM_Cannon(20);
											ThisEvent.EventParam = 0;
											printf("Commanding: ES_MANUAL_SHOOT, Event Param = 0 \n\r");
											break;
						case 'R' : ThisEvent.EventType = ES_CANNON_READY;
											printf("Releasing hopper\r\n");
											break;

        }
				
				PostMasterSM(ThisEvent);
				PostDriveTrainControlService(ThisEvent);
				PostPeriscopeControlService(ThisEvent);
				PostPositionLogicService(ThisEvent);
    }
    
  return ReturnEvent;
}


