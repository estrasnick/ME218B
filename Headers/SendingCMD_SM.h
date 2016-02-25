/****************************************************************************
SendingCMD Header File
 ****************************************************************************/

#ifndef SendingCMD_H
#define SendingCMD_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Waiting_t, Waiting4Interrupt_t, Waiting4Timeout_t } SendingCMDState_t ;


// Public Function Prototypes

ES_Event RunSendingCMDSM( ES_Event CurrentEvent );
void StartSendingCMDSM ( ES_Event CurrentEvent );
SendingCMDState_t QuerySendingCMDSM ( void );

		
uint8_t * getResponseArray(void);
#endif 

