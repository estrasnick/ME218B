/****************************************************************************
 Module
   EnemyCaptureQueue.c

 Description
		This is an array-based implementation of a FIFO queue, used for storing
		   enemy captured stations in the order of their capture. The array is
			 static because we have a maximum of 9 entries. (And because the
			 Tiva is incapable of dyanmic memory management...)
****************************************************************************/

#include "ES_Configure.h"
#include "ES_Framework.h"
#include "DEFINITIONS.h"
#include "EnemyCaptureQueue.h"

#define MAX_QUEUE_CAPACITY 9
#define QUEUE_EMPTY 0xff

typedef struct {
    uint8_t stationIndices[MAX_QUEUE_CAPACITY];
    uint8_t head, tail, max;
} queue;

static queue q = 
{
	.head = 0,
	.tail = 0,
	.max = MAX_QUEUE_CAPACITY
};

// Checks if the given queue is empty
bool IsEmpty()
{
    return q.tail == q.head;
}
 
// Add a new station to the queue
void Enqueue(uint8_t station)
{
		// Cycle around the buffer if need be
    if (q.tail >= q.max) 
		{
			q.tail = 0;
		}
		
		// Store the given station index
    q.stationIndices[q.tail++] = station;
}
 
// Dequeue a station from the queue and returns it
// Returns QUEUE_EMPTY if there is nothing to dequeue
uint8_t Dequeue()
{
		// If there is nothing in the queue, return false
    if (q.head == q.tail) 
		{
			return QUEUE_EMPTY;
		}

		// Otherwise, dequeue the station
		uint8_t returnStation = q.stationIndices[q.head++];
		
		// Cycle around the buffer if need be
		if (q.head >= q.max) { 
			q.head = 0;
		}
		return returnStation;
}

// Return the position of the given station in the queue,
//  or NOT_IN_QUEUE if it is not found
uint8_t PositionInQueue(uint8_t station)
{
	int i = q.head;
	int position = 0;
	
	while(i != q.tail)
	{
		if (q.stationIndices[i] == station)
		{
			return position;
		}
		i++;
		position++;
		if (i == q.max)
		{
			i = 0;
		}
	}
	return NOT_IN_QUEUE;
}