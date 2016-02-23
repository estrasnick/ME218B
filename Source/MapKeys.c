/****************************************************************************
 Module
   MapKeys.c

 Revision
   1.0.1

 Description
   This service maps keystrokes to events 

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
            case 'A' : ThisEvent.EventType = ES_DRIVE_FULL_SPEED; 
											printf("Commanding: ES_DRIVE_FULL_SPEED \n\r");
                       break;
						case 'S' : ThisEvent.EventType = ES_DRIVE_HALF_SPEED; 
											printf("Commanding: ES_DRIVE_HALF_SPEED \n\r");
                       break;
            case 'D' : ThisEvent.EventType = ES_STOP_DRIVE;
											 printf("Commanding: ES_STOP_DRIVE \n\r");
                       break;
						case 'F' : ThisEvent.EventType = ES_ROTATE_45; 
 											 printf("Commanding: ES_ROTATE_45 \n\r");
												break;
						case 'G' : ThisEvent.EventType = ES_ROTATE_90; 
											 printf("Commanding: ES_ROTATE_90 \n\r");                     
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
											 ThisEvent.EventParam = 0;
											 printf("Commanding: ES_PS_DETECTED, Event Param = 0 \n\r");                     
											 break;
						case 'K' : ThisEvent.EventType = ES_CALCULATE_POSITION; 
											 ThisEvent.EventParam = 0;
											 printf("Commanding: ES_CALCULATE_POSITION, Event Param = 0 \n\r");                     
											 break;
						case 'H' : ThisEvent.EventType = ES_MANUAL_SHOOT; 
											 ThisEvent.EventParam = 0;
											 printf("Commanding: ES_MANUAL_SHOOT, Event Param = 0 \n\r");                     
											 break;
									
        }
				
				PostMasterSM(ThisEvent);
				PostDriveTrainControlService(ThisEvent);
				PostPeriscopeControlService(ThisEvent);
				PostPositionLogicService(ThisEvent);
    }
    
  return ReturnEvent;
}


