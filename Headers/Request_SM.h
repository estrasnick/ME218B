/****************************************************************************
Request Header File
 ****************************************************************************/

#ifndef Request_H
#define Request_H


// typedefs for the states
// State definitions for use with the query function
typedef enum { Request_t, Query_t } RequestState_t ;


// Public Function Prototypes

ES_Event RunRequestSM( ES_Event CurrentEvent );
void StartRequestSM ( ES_Event CurrentEvent );
RequestState_t QueryRequestSM ( void );

#endif /*SHMTemplate_H */

