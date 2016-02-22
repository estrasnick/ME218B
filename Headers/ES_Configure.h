/****************************************************************************
 Module
     ES_Configure.h
 Description
     This file contains macro definitions that are edited by the user to
     adapt the Events and Services framework to a particular application.
 Notes
     
 History
 When           Who     What/Why
 -------------- ---     --------
 10/21/13 20:54 jec      lots of added entries to bring the number of timers
                         and services up to 16 each
 08/06/13 14:10 jec      removed PostKeyFunc stuff since we are moving that
                         functionality out of the framework and putting it
                         explicitly into the event checking functions
 01/15/12 10:03 jec      started coding
*****************************************************************************/

#ifndef CONFIGURE_H
#define CONFIGURE_H

/****************************************************************************/
// The maximum number of services sets an upper bound on the number of 
// services that the framework will handle. Reasonable values are 8 and 16
// corresponding to an 8-bit(uint8_t) and 16-bit(uint16_t) Ready variable size
#define MAX_NUM_SERVICES 16

/****************************************************************************/
// This macro determines that nuber of services that are *actually* used in
// a particular application. It will vary in value from 1 to MAX_NUM_SERVICES
#define NUM_SERVICES 8

/****************************************************************************/
// These are the definitions for Service 0, the lowest priority service.
// Every Events and Services application must have a Service 0. Further 
// services are added in numeric sequence (1,2,3,...) with increasing 
// priorities
// the header file with the public function prototypes
#define SERV_0_HEADER "MapKeys.h"
// the name of the Init function
#define SERV_0_INIT InitMapKeys
// the name of the run function
#define SERV_0_RUN RunMapKeys
// How big should this services Queue be?
#define SERV_0_QUEUE_SIZE 2

/****************************************************************************/
// The following sections are used to define the parameters for each of the
// services. You only need to fill out as many as the number of services 
// defined by NUM_SERVICES
/****************************************************************************/
// These are the definitions for Service 1
#if NUM_SERVICES > 1
#define MASTER_PRIORITY 1 //defining this for our deferral events

// the header file with the public function prototypes
#define SERV_1_HEADER "Master_SM.h"
// the name of the Init function
#define SERV_1_INIT InitMasterSM
// the name of the run function
#define SERV_1_RUN RunMasterSM
// How big should this services Queue be?
#define SERV_1_QUEUE_SIZE 10
#endif

/****************************************************************************/
// These are the definitions for Service 2
#if NUM_SERVICES > 2
// the header file with the public function prototypes
#define SERV_2_HEADER "PWM_Service.h"
// the name of the Init function
#define SERV_2_INIT InitPWMService
// the name of the run function
#define SERV_2_RUN RunPWMService
// How big should this services Queue be?
#define SERV_2_QUEUE_SIZE 5
#endif

/****************************************************************************/
// These are the definitions for Service 3
#if NUM_SERVICES > 3
// the header file with the public function prototypes
#define SERV_3_HEADER "DriveTrainControl_Service.h"
// the name of the Init function
#define SERV_3_INIT InitDriveTrainControlService
// the name of the run function
#define SERV_3_RUN RunDriveTrainControlService
// How big should this services Queue be?
#define SERV_3_QUEUE_SIZE 7
#endif

/****************************************************************************/
// These are the definitions for Service 4
#if NUM_SERVICES > 4
// the header file with the public function prototypes
#define SERV_4_HEADER "PositionLogic_Service.h"
// the name of the Init function
#define SERV_4_INIT InitPositionLogicService
// the name of the run function
#define SERV_4_RUN RunPositionLogicService
// How big should this services Queue be?
#define SERV_4_QUEUE_SIZE 5
#endif

/****************************************************************************/
// These are the definitions for Service 5
#if NUM_SERVICES > 5
// the header file with the public function prototypes
#define SERV_5_HEADER "PhotoTransistor_Service.h"
// the name of the Init function
#define SERV_5_INIT InitPhotoTransistorService
// the name of the run function
#define SERV_5_RUN RunPhotoTransistorService
// How big should this services Queue be?
#define SERV_5_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 6
#if NUM_SERVICES > 6
// the header file with the public function prototypes
#define SERV_6_HEADER "PeriscopeControl_Service.h"
// the name of the Init function
#define SERV_6_INIT InitPeriscopeControlService
// the name of the run function
#define SERV_6_RUN RunPeriscopeControlService
// How big should this services Queue be?
#define SERV_6_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 7
#if NUM_SERVICES > 7
// the header file with the public function prototypes
#define SERV_7_HEADER "CannonControl_Service.h"
// the name of the Init function
#define SERV_7_INIT InitCannonControlService
// the name of the run function
#define SERV_7_RUN RunCannonControlService
// How big should this services Queue be?
#define SERV_7_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 8
#if NUM_SERVICES > 8
// the header file with the public function prototypes
#define SERV_8_HEADER "TestHarnessService8.h"
// the name of the Init function
#define SERV_8_INIT InitTestHarnessService8
// the name of the run function
#define SERV_8_RUN RunTestHarnessService8
// How big should this services Queue be?
#define SERV_8_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 9
#if NUM_SERVICES > 9
// the header file with the public function prototypes
#define SERV_9_HEADER "TestHarnessService9.h"
// the name of the Init function
#define SERV_9_INIT InitTestHarnessService9
// the name of the run function
#define SERV_9_RUN RunTestHarnessService9
// How big should this services Queue be?
#define SERV_9_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 10
#if NUM_SERVICES > 10
// the header file with the public function prototypes
#define SERV_10_HEADER "TestHarnessService10.h"
// the name of the Init function
#define SERV_10_INIT InitTestHarnessService10
// the name of the run function
#define SERV_10_RUN RunTestHarnessService10
// How big should this services Queue be?
#define SERV_10_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 11
#if NUM_SERVICES > 11
// the header file with the public function prototypes
#define SERV_11_HEADER "TestHarnessService11.h"
// the name of the Init function
#define SERV_11_INIT InitTestHarnessService11
// the name of the run function
#define SERV_11_RUN RunTestHarnessService11
// How big should this services Queue be?
#define SERV_11_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 12
#if NUM_SERVICES > 12
// the header file with the public function prototypes
#define SERV_12_HEADER "TestHarnessService12.h"
// the name of the Init function
#define SERV_12_INIT InitTestHarnessService12
// the name of the run function
#define SERV_12_RUN RunTestHarnessService12
// How big should this services Queue be?
#define SERV_12_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 13
#if NUM_SERVICES > 13
// the header file with the public function prototypes
#define SERV_13_HEADER "TestHarnessService13.h"
// the name of the Init function
#define SERV_13_INIT InitTestHarnessService13
// the name of the run function
#define SERV_13_RUN RunTestHarnessService13
// How big should this services Queue be?
#define SERV_13_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 14
#if NUM_SERVICES > 14
// the header file with the public function prototypes
#define SERV_14_HEADER "TestHarnessService14.h"
// the name of the Init function
#define SERV_14_INIT InitTestHarnessService14
// the name of the run function
#define SERV_14_RUN RunTestHarnessService14
// How big should this services Queue be?
#define SERV_14_QUEUE_SIZE 3
#endif

/****************************************************************************/
// These are the definitions for Service 15
#if NUM_SERVICES > 15
// the header file with the public function prototypes
#define SERV_15_HEADER "TestHarnessService15.h"
// the name of the Init function
#define SERV_15_INIT InitTestHarnessService15
// the name of the run function
#define SERV_15_RUN RunTestHarnessService15
// How big should this services Queue be?
#define SERV_15_QUEUE_SIZE 3
#endif


/****************************************************************************/
// Name/define the events of interest
// Universal events occupy the lowest entries, followed by user-defined events
typedef enum {  ES_NO_EVENT = 0,
                ES_ERROR,  /* used to indicate an error from the service */
                ES_INIT,   /* used to transition from initial pseudo-state */
                ES_NEW_KEY, /* signals a new key received from terminal */
                ES_TIMEOUT, /* signals that the timer has expired */
                ES_ENTRY,
                ES_ENTRY_HISTORY,
                ES_EXIT,
                /* User-defined events start here */
								
								ES_NEW_DESTINATION, //Posted whenever we have chosen a new destination
								
								ES_PS_DETECTED,			//posted when we think we detected a a polling station
								ES_PS_MEASURING,		//tells hall effect sensor to re-enter the measuring state and continue looking for polling statinos
								ES_PS_CAPTURED, 		//tells master when a polling station has been captured
	
								ES_OPEN_HOPPER,			//commands the hopper to release a ball to the shooter
								ES_RELOAD_HOPPER,		//commands the hopper to load another ball in
								
								ES_CANNON_READY,		//when the cannon has reached our desired speed
	
								ES_TRANSACTION_COMPLETE,	//posted when a transaction is completed with the super pac
								ES_SEND_CMD,					//tells the PAC to send a command
								ES_SEND_BYTE,					//tells the PAC to send a byte
								ES_EOT,								//receive this when we the byte has been sent
								
								//The Following Are Events solely for testing purposes
								ES_DRIVE_FULL_SPEED,
								ES_DRIVE_HALF_SPEED,
								ES_REVERSE_FULL_SPEED,
								ES_REVERSE_HALF_SPEED,
								ES_STOP_DRIVE,
								ES_ROTATE_45,
								ES_ROTATE_90,
								
								ES_START_CANNON,
								ES_STOP_CANNON,
								
								ES_FACE_TARGET,
								ES_DRIVE_TO_TARGET,
								
								ES_CALCULATE_POSITION,
								ES_ARRIVED,
								
								ES_START_PERISCOPE,
								ES_STOP_PERISCOPE,
								
								ES_MANUAL_START
								
								} ES_EventTyp_t ;

/****************************************************************************/
// These are the definitions for the Distribution lists. Each definition
// should be a comma separated list of post functions to indicate which
// services are on that distribution list.
#define NUM_DIST_LISTS 1
#if NUM_DIST_LISTS > 0 
#define DIST_LIST0 PostMapKeys, PostMasterSM
#endif
#if NUM_DIST_LISTS > 1 
#define DIST_LIST1 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 2 
#define DIST_LIST2 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 3 
#define DIST_LIST3 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 4 
#define DIST_LIST4 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 5 
#define DIST_LIST5 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 6 
#define DIST_LIST6 PostTemplateFSM
#endif
#if NUM_DIST_LISTS > 7 
#define DIST_LIST7 PostTemplateFSM
#endif

/****************************************************************************/
// This are the name of the Event checking funcion header file. 
#define EVENT_CHECK_HEADER "EventCheckers.h"

/****************************************************************************/
// This is the list of event checking functions 
#define EVENT_CHECK_LIST Check4Keystroke

/****************************************************************************/
// These are the definitions for the post functions to be executed when the
// corresponding timer expires. All 16 must be defined. If you are not using
// a timer, then you should use TIMER_UNUSED
// Unlike services, any combination of timers may be used and there is no
// priority in servicing them
#define TIMER_UNUSED ((pPostFunc)0)
#define TIMER0_RESP_FUNC PostDriveTrainControlService
		#define MOTOR_STOPPED_L	0
#define TIMER1_RESP_FUNC PostMasterSM
		#define SEARCH_4_PS	1
#define TIMER2_RESP_FUNC PostMasterSM
		#define POSITION_CHECK	2
		#define POSITION_CHECK_T 100
#define TIMER3_RESP_FUNC PostMasterSM
		#define CAMPAIGN_STATUS_CHECK	3
#define TIMER4_RESP_FUNC PostMasterSM
		#define SSI_TIMER	4
#define TIMER5_RESP_FUNC PostMasterSM
		#define GAME_TIMER	5
		#define GAME_TIMER_T 138000
#define TIMER6_RESP_FUNC PostPositionLogicService
		#define RELATIVE_POSITION_TIMER 6
#define TIMER7_RESP_FUNC PostDriveTrainControlService
		#define MOTOR_STOPPED_R 7
#define TIMER8_RESP_FUNC PostMasterSM
#define TIMER9_RESP_FUNC PostDriveTrainControlService
		#define ROTATE_TESTING_TIMER	9
#define TIMER10_RESP_FUNC PostDriveTrainControlService
		#define TEST_TIMER 10
#define TIMER11_RESP_FUNC PostPeriscopeControlService
		#define START_PERISCOPE_TIMER 11
		#define START_PERISCOPE_T 100
#define TIMER12_RESP_FUNC TIMER_UNUSED
#define TIMER13_RESP_FUNC TIMER_UNUSED
#define TIMER14_RESP_FUNC TIMER_UNUSED
#define TIMER15_RESP_FUNC TIMER_UNUSED


/****************************************************************************/
// Give the timer numbers symbolc names to make it easier to move them
// to different timers if the need arises. Keep these definitions close to the
// definitions for the response functions to make it easier to check that
// the timer number matches where the timer event will be routed
// These symbolic names should be changed to be relevant to your application 

#define SERVICE0_TIMER 15

#endif /* CONFIGURE_H */
