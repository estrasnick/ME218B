/****************************************************************************
SendingByte Header File
 ****************************************************************************/

#ifndef SendingByte_H
#define SendingByte_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Waiting2Send_t,  } SendingByteState_t ;


// Public Function Prototypes

ES_Event RunSendingByteSM( ES_Event CurrentEvent );
void StartSendingByteSM ( ES_Event CurrentEvent );
SendingByteState_t QuerySendingByteSM ( void );

#endif /*SHMTemplate_H */

