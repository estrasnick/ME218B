/****************************************************************************
EnemyCaptureQueue
 ****************************************************************************/

#ifndef EnemyCaptureQueue_H
#define EnemyCaptureQueue_H

#include <stdlib.h>
#include <string.h>
#include "ES_Types.h"



// Public Function Prototypes
bool IsEmpty(void);
void Enqueue(uint8_t station);
uint8_t Dequeue(void);
uint8_t PositionInQueue(uint8_t station);

#endif 

