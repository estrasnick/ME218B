/***************************************************************

Definitions header file for any definitiosn taht should be included throughout entire project

****************************************************************/

#ifndef Definitions_H
#define Definitions_H

//*******************************************************************************************
//--------------------------------- INCLUDES --------------------------------------
//*******************************************************************************************
//General Header Includes
#include "ES_Configure.h"
#include "ES_Types.h"

// the headers to access the GPIO subsystem
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_sysctl.h"

// the headers to access the TivaWare Library
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#define ALL_BITS (0xff<<2)
#define TICKS_PER_MS 40000ul
#define MICROSECONDS_DIVISOR 1000

//*******************************************************************************************
//--------------------------------- TESTING MODE --------------------------------------
//*******************************************************************************************
#define TESTING_MODE true


//*******************************************************************************************
//--------------------------------- PIN DEFINITIONS --------------------------------------
//*******************************************************************************************

// Port A
// SSI/PAC Module
#define PAC_SYSCTL 			SYSCTL_RCGCGPIO_R0
#define PAC_BASE 				GPIO_PORTA_BASE

#define unavailable1 		GPIO_PIN_0
#define unavailable2		GPIO_PIN_1
#define PAC_CLOCK_PIN 	GPIO_PIN_2
#define PAC_FSS_PIN 		GPIO_PIN_3
#define PAC_RX_PIN 			GPIO_PIN_4
#define PAC_TX_PIN 			GPIO_PIN_5
#define unused1					GPIO_PIN_6
#define unused2 				GPIO_PIN_7

// Port B
// Periscope, Reload, Cannon Angle, Hopper 
#define PERISCOPE_SYSCTL	SYSCTL_RCGCGPIO_R1
#define PERISCOPE_BASE 		GPIO_PORTB_BASE
#define RELOAD_SYSCTL			SYSCTL_RCGCGPIO_R1
#define RELOAD_BASE 			GPIO_PORTB_BASE
#define CANNONANGLE_SYSCTL	SYSCTL_RCGCGPIO_R1
#define CANNONANGLE_BASE 		GPIO_PORTB_BASE
#define HOPPER_SYSCTL			SYSCTL_RCGCGPIO_R1
#define HOPPER_BASE 			GPIO_PORTB_BASE

#define unused0									GPIO_PIN_0
#define unused3 								GPIO_PIN_1
#define unused4 								GPIO_PIN_2
#define unused5									GPIO_PIN_3
#define CANNON_PWM_PIN  				GPIO_PIN_4
#define PERISCOPE_PWM_PIN 			GPIO_PIN_5
#define HOPPER_PWM_PIN 							GPIO_PIN_6
#define PERISCOPE_LATCH_PWM_PIN	GPIO_PIN_7

// Port C
// Phototransistor
#define PHOTOTRANSISTOR_SYSCTL SYSCTL_RCGCGPIO_R2
#define PHOTOTRANSISTOR_BASE GPIO_PORTC_BASE

#define unavailable3 GPIO_PIN_0
#define unavailable4 GPIO_PIN_1
#define unavailable5 GPIO_PIN_2
#define unavailable6 GPIO_PIN_3
#define PHOTOTRANSISTOR_CAPTURE_PIN 	GPIO_PIN_4
#define CANNON_ENCODER_PIN 				GPIO_PIN_5
#define PERISCOPE_ENCODER_PIN_1 			GPIO_PIN_6
#define PERISCOPE_ENCODER_PIN_2 				GPIO_PIN_7

// Port D
// Hall Effect Sensors, Drive Encoders
#define HALL_SENSOR_SYSCTL 					SYSCTL_RCGCGPIO_R3
#define HALL_SENSOR_BASE 						GPIO_PORTD_BASE
#define DRIVE_ENCODER_TIMER_SYSCTL 	SYSCTL_RCGCGPIO_R3
#define DRIVE_ENCODER_TIMER_BASE 		GPIO_PORTD_BASE

#define HALL_SENSOR_OUTER_LEFT_PIN	GPIO_PIN_0
#define HALL_SENSOR_INNER_LEFT_PIN 	GPIO_PIN_1
#define HALL_SENSOR_INNER_RIGHT_PIN GPIO_PIN_2
#define HALL_SENSOR_OUTER_RIGHT_PIN GPIO_PIN_3
#define unavailable7 								GPIO_PIN_4
#define unavailable8 								GPIO_PIN_5
#define DRIVE_ENCODER_LEFT_PIN 			GPIO_PIN_6
#define DRIVE_ENCODER_RIGHT_PIN 		GPIO_PIN_7

// Port E
// Hall Effect Sensors, Drive Encoders
#define DRIVE_SYSCTL 									SYSCTL_RCGCGPIO_R4
#define DRIVE_BASE 										GPIO_PORTE_BASE
#define DRIVE_ENCODER_POLLING_SYSCTL 	SYSCTL_RCGCGPIO_R4
#define DRIVE_ENCODER_POLLING_BASE 		GPIO_PORTE_BASE

#define DRIVE_DIRECTION_LEFT_PIN		GPIO_PIN_0
#define DRIVE_DIRECTION_RIGHT_PIN 	GPIO_PIN_1
#define DRIVE_ENCODER_LEFT_POLLING_PIN	 	GPIO_PIN_2
#define DRIVE_ENCODER_RIGHT_POLLING_PIN 	GPIO_PIN_3
#define DRIVE_PWM_LEFT_PIN 					GPIO_PIN_4
#define DRIVE_PWM_RIGHT_PIN 				GPIO_PIN_5
#define unavailable9 								GPIO_PIN_6
#define unavailable10 							GPIO_PIN_7

// Port F
// Game Swiches
#define GAME_SYSCTL 	SYSCTL_RCGCGPIO_R5
#define GAME_BASE 		GPIO_PORTF_BASE

#define COLOR_PIN							GPIO_PIN_0
#define GAME_STATUS_PIN				GPIO_PIN_1
#define unused6 							GPIO_PIN_2
#define unused7 							GPIO_PIN_3
#define unused8 							GPIO_PIN_4
#define unavailable11 				GPIO_PIN_5
#define unavailable12 				GPIO_PIN_6
#define unavailable13			 		GPIO_PIN_7


//*******************************************************************************************
//--------------------------------- INTERRUPTS --------------------------------------
//*******************************************************************************************
//!!!!!! NOTE TO CHANGE THESE REMEMBER TO ALTER THE START UP FILE AS WELL!!!!!!
//Encoder Interrupts
#define DRIVE_LEFT_ENCODER_INTERRUPT		WT5CCP0
#define DRIVE_RIGHT_ENCODER_INTERRUPT		WT5CCP1
#define PERISCOPE_ENCODER_INTERRUPT_1			WT1CCP0
#define PERISCOPE_ENCODER_INTERRUPT_2			WT1CCP1
#define CANNON_ENCODER_INTERRUPT				WT0CCP1

//PhotoTransistor Interrupts
#define PHOTOTRANSISTOR_INTERRUPT		WT0CCP0
//#define PHOTOTRANSISTOR_RIGHT_INTERRUPT		WT0CCP1

//Hall Effect Interrupts
#define HALLSENSOR_OUTER_LEFT_INTERRUPT		WT2CCP0
#define HALLSENSOR_INNER_LEFT_INTERRUPT		WT2CCP1
#define HALLSENSOR_INNER_RIGHT_INTERRUPT	WT3CCP0
#define HALLSENSOR_OUTER_RIGHT_INTERRUPT	WT3CCP1

//Tape Sensor Interrupt
//#define PERISCOPE_TAPE_SENSOR_INTERRUPT   

//Control Law Interrupts
#define DRIVE_CONTROL_INTERRUPT		WT4CCP0
#define CANNON_CONTROL_INTERRUPT	WT4CCP1

//PRIORITIES
#define DRIVE_ENCODER_INTERRUPT_PRIORITY 0
#define PERISCOPE_ENCODER_INTERRUPT_PRIORITY 0
#define CANNON_ENCODER_INTERRUPT_PRIORITY 0

#define PHOTOTRANSISTOR_INTERRUPT_PRIORITY	1

#define HALLSENSOR_INTERRUPT_PRIORITY	3

#define DRIVE_CONTROL_INTERRUPT_PRIORITY 2
#define CANNON_CONTROL_INTERRUPT_PRIORITY 2

//PERIODIC INTERRUPT TIMES (microseconds)
#define DRIVE_CONTROL_INTERRUPT_PERIOD 2000
#define CANNON_CONTROL_INTERRUPT_PERIOD 2000
#define NULL_INTERRUPT_PERIOD 0

//Timer Definitions
#define WT0CCP0	0, 0
#define WT0CCP1	0, 1
#define WT1CCP0	1, 0
#define WT1CCP1	1, 1
#define WT2CCP0	2, 0
#define WT2CCP1	2, 1
#define WT3CCP0	3, 0
#define WT3CCP1	3, 1
#define WT4CCP0	4, 0
#define WT4CCP1	4, 1
#define WT5CCP0	5, 0
#define WT5CCP1	5, 1

//Paramaters to Pass our Initialization Functions
#define DRIVE_LEFT_ENCODER_INTERRUPT_PARAMATERS			DRIVE_LEFT_ENCODER_INTERRUPT, DRIVE_ENCODER_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define DRIVE_RIGHT_ENCODER_INTERRUPT_PARAMATERS		DRIVE_RIGHT_ENCODER_INTERRUPT, DRIVE_ENCODER_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_1			PERISCOPE_ENCODER_INTERRUPT_1, PERISCOPE_ENCODER_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define PERISCOPE_ENCODER_INTERRUPT_PARAMATERS_2			PERISCOPE_ENCODER_INTERRUPT_2, PERISCOPE_ENCODER_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define CANNON_ENCODER_INTERRUPT_PARAMATERS					CANNON_ENCODER_INTERRUPT, CANNON_ENCODER_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD

//PhotoTransistor Interrupts
#define PHOTOTRANSISTOR_INTERRUPT_PARAMATERS		PHOTOTRANSISTOR_INTERRUPT, PHOTOTRANSISTOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
//#define PHOTOTRANSISTOR_RIGHT_INTERRUPT_PARAMATERS	PHOTOTRANSISTOR_RIGHT_INTERRUPT, PHOTOTRANSISTOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD

//Hall Effect Interrupts
#define HALLSENSOR_OUTER_LEFT_INTERRUPT_PARAMATERS		HALLSENSOR_OUTER_LEFT_INTERRUPT, HALLSENSOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define HALLSENSOR_INNER_LEFT_INTERRUPT_PARAMATERS		HALLSENSOR_INNER_LEFT_INTERRUPT, HALLSENSOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define HALLSENSOR_INNER_RIGHT_INTERRUPT_PARAMATERS		HALLSENSOR_INNER_RIGHT_INTERRUPT, HALLSENSOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD
#define HALLSENSOR_OUTER_RIGHT_INTERRUPT_PARAMATERS		HALLSENSOR_OUTER_RIGHT_INTERRUPT, HALLSENSOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD

//Tape Sensor Interrupts
#define PERISCOPE_TAPE_SENSOR_INTERRUPT_PARAMATERS  PERISCOPE_TAPE_SENSOR_INTERRUPT, PERISCOPE_TAPE_SENSOR_INTERRUPT_PRIORITY, NULL_INTERRUPT_PERIOD

//Control Law Interrupts
#define DRIVE_CONTROL_INTERRUPT_PARAMATERS		DRIVE_CONTROL_INTERRUPT, DRIVE_CONTROL_INTERRUPT_PRIORITY, DRIVE_CONTROL_INTERRUPT_PERIOD
#define CANNON_CONTROL_INTERRUPT_PARAMATERS		CANNON_CONTROL_INTERRUPT, CANNON_CONTROL_INTERRUPT_PRIORITY, CANNON_CONTROL_INTERRUPT_PERIOD

//*******************************************************************************************
//--------------------------------- PWM --------------------------------------
//*******************************************************************************************
//Module 1
#define M0PWM0	0, 0, 0 //B6
#define M0PWM1	0, 0, 1 //B7
#define M0PWM2	0, 1, 0 //B4
#define M0PWM3	0, 1, 1 //B5

//Module 2
#define M1PWM2	1, 1, 0 //E4	
#define M1PWM3	1, 1, 1 //E5

//ASSIGN THE PWM PINS!
#define	LEFT_DRIVE_PWM				M1PWM2
#define	RIGHT_DRIVE_PWM				M1PWM3
#define	CANNON_PWM						M0PWM2
#define	PERISCOPE_PWM 				M0PWM3
#define	HOPPER_PWM 						M0PWM0
#define PERISCOPE_LATCH_PWM   M0PWM1

//PERIODS ARE IN MICROSECONDS
//ASSIGN THE PERIODS
#define LEFT_DRIVE_PERIOD			50
#define RIGHT_DRIVE_PERIOD		50
#define PERISCOPE_PERIOD			50
#define PERISCOPE_LATCH_PERIOD 50
#define HOPPER_PERIOD					50
#define CANNON_PERIOD					50

//PIN DIRECTIONS (note hopper is a servo so it doesn't need this)
#define LEFT_DRIVE_FORWARD_PIN_DIRECTION 		0
#define LEFT_DRIVE_BACKWARD_PIN_DIRECTION 	1
#define RIGHT_DRIVE_FORWARD_PIN_DIRECTION 	0
#define RIGHT_DRIVE_BACKWARD_PIN_DIRECTION 	1


//Init Definitions
#define LEFT_DRIVE_PWM_PARAMATERS		LEFT_DRIVE_PWM, LEFT_DRIVE_PERIOD
#define RIGHT_DRIVE_PWM_PARAMATERS	RIGHT_DRIVE_PWM, RIGHT_DRIVE_PERIOD
#define PERISCOPE_PWM_PARAMATERS		PERISCOPE_PWM, PERISCOPE_PERIOD
#define HOPPER_PWM_PARAMATERS				HOPPER_PWM, HOPPER_PERIOD
#define CANNON_PWM_PARAMATERS				CANNON_PWM, CANNON_PERIOD
#define PERISCOPE_LATCH_PWM_PARAMATERS    PERISCOPE_LATCH_PWM, PERISCOPE_LATCH_PERIOD

#define PERISCOPE_PWM_DUTY 30 //45
#define PERISCOPE_LATCH_DUTY 75
#define PERISCOPE_UNLATCH_DUTY 25
#define HOPPER_LOAD_DUTY 35
#define HOPPER_DEFAULT_DUTY 50

//*******************************************************************************************
//--------------------------------- SPI COMMANDS --------------------------------------
//*******************************************************************************************
#define CAMPAIGN_STATUS_COMMAND 0xC0
#define QUERY_COMMAND 0x70
#define RECEIVING_COMMAND 0x00

#define RED 0
#define BLUE 1

#define SSI_TIMER_LENGTH 2

//Possible Responses
#define RESPONSE_READY 0xAA

//Acknowledged
typedef enum { NACK_b, ACK_b, Blocked_b, Busy_b } Acknowledge_b ;
//Red Blue None
typedef enum { RED_b, BLUE_b, Unclaimed_b, Undefined_b} Claimed_b ;

//Acknowledged Structure
typedef struct { 
	Acknowledge_b state;
	uint8_t	bits;
} Acknowledge_st;


//--------------------------------- Locations --------------------------------------
#define SACREMENTO_CODE		0x1
#define SEATTLE_CODE			0x2
#define BILLINGS_CODE			0x3
#define DENVER_CODE				0x4
#define DALLAS_CODE				0x5
#define CHICAGO_CODE			0x6
#define MIAMI_CODE				0x7
#define WASHINGTON_CODE		0x8
#define CONCORD_CODE			0x9

#define NUM_STATIONS 9


//*******************************************************************************************
//--------------------------------- Gear Ratios --------------------------------------
//*******************************************************************************************
#define PERISCOPE_GEAR_RATIO 100
#define DRIVE_GEAR_RATIO 50
#define FLYWHEEL_GEAR_RATIO 25 // NOTE: This is not final!!!



//*******************************************************************************************
//--------------------------------- Hall Effect Frequencies --------------------------------------
//*******************************************************************************************
//Define the possible periods of the hall effect sensors
//note that the periods are actually periods
#define HE_f_1000 1333
#define HE_f_947 1277
#define HE_f_893 1222
#define HE_f_840 1166
#define HE_f_787 1111
#define HE_f_733 1055
#define HE_f_680 1000
#define HE_f_627 944
#define HE_f_573 889
#define HE_f_520 833
#define HE_f_467 778
#define HE_f_413 722
#define HE_f_360 667
#define HE_f_307 611
#define HE_f_253 556
#define HE_f_200 500

//Bit Codes for Hall Effect Sensors
#define HE_b_1000 0x0
#define HE_b_947 	0x1
#define HE_b_893 	0x2
#define HE_b_840  0x3
#define HE_b_787  0x4
#define HE_b_733 	0x5
#define HE_b_680 	0x6
#define HE_b_627 	0x7
#define HE_b_573 	0x8
#define HE_b_520 	0x9
#define HE_b_467 	0xA
#define HE_b_413 	0xB
#define HE_b_360 	0xC
#define HE_b_307 	0xC
#define HE_b_253 	0xD
#define HE_b_200 	0xE

static uint8_t PS_Frequency_Codes[] = {
	HE_b_1000, 
	HE_b_947, 
	HE_b_893,
	HE_b_840, 
	HE_b_787, 
	HE_b_733,
	HE_b_680, 
	HE_b_627, 
	HE_b_573,
	HE_b_520, 
	HE_b_467, 
	HE_b_413,
	HE_b_360,
	HE_b_307, 
	HE_b_253, 
	HE_b_200
};

//*******************************************************************************************
//--------------------------------- Other --------------------------------------
//*******************************************************************************************
//Pin Directions
#define INPUT 0
#define OUTPUT 1

#define RELATIVE_POSITION_T 10
#define DISTANCE_BETWEEN_WHEELS 11.0486f
#define WHEEL_CIRCUMFERENCE 9.42477796f

#define ENCODER_PULSES_PER_REV 5
#define SECS_PER_MIN 60
#define MS_PER_SEC 1000
	
#define NUM_STATIONS 9

#define COLOR_BLUE 1
#define COLOR_RED 0
	
#define NULL_STATION 10
	
#define DEFAULT_DRIVE_RPM 60.0f
#define NOT_IN_QUEUE 0x30

#define PRI_DISTANCE_MULTIPLIER 1
#define PRI_CAPTURE_HISTORY_MULTIPLIER 50
#define RETRY_MULTIPLIER 30

#define NULL_BEACON 5

#define HALL_SENSOR_OFFSET_IN_TICKS 30

#define CANNON_DISTANCE_MULTIPLIER 5

#define PROXIMITY_TO_OUR_STATION_THRESHOLD 5.0
	
#endif
