/****************************************************************************
 MapKeys header file

 ****************************************************************************/

#ifndef MapKeys_H
#define MapKeys_H

// Public Function Prototypes

bool InitMapKeys ( uint8_t Priority );
bool PostMapKeys( ES_Event ThisEvent );
ES_Event RunMapKeys( ES_Event ThisEvent );

#endif /*MapKeys_H */

