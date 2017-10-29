#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
// matches ship
// tobacco money
// paper crew

// An agent semaphore represents items on the table
sem_t agent_ready;

// Each sailor semaphore represents when a sailor has the items they need
sem_t sailor_semaphors[3];

// This is an array of strings describing what each sailor type needs
char* sailor_types[3] = { "ship & money", "ship & crew", "money & crew" };

// This list represents item types that are on the table. This should correspond
// with the sailor_types, such that each item is the one the sailor has. So the
// first item would be paper, then tobacco, then matches.
bool items_on_table[3] = { false, false, false };

// Each pusher pushes a certian type item, manage these with this semaphore
sem_t pusher_semaphores[3];

/**
 * sailor function, handles waiting for the item's that they need, and then
 * smoking. Repeat this three times
 */
void* sailor(void* arg)
{
	int sailor_id = *(int*) arg;
	int type_id   = sailor_id % 3;
	int random;
	// Smoke 3 times
	for (int i = 0; i < 3; ++i)
	{
		printf("\033[0;37msailor %d \033[0;31m>>\033[0m Waiting for %s\n",
			sailor_id, sailor_types[type_id]);

		// Wait for the proper combination of items to be on the table
		sem_wait(&sailor_semaphors[type_id]);

		// Make the ship before releasing the agent
		printf("\033[0;37msailor %d \033[0;32m<<\033[0m Now getting ready to set sail\n", sailor_id);
		// random = rand() % 10000000;
		
		// printf("%d\n", random);
		
		usleep(200000);
		
        // We're sailing now
		printf("\033[0;37msailor %d \033[0;37m--\033[0m Now sailing\n", sailor_id);
        
        // random = rand() % 10000000;
        // printf("%d\n",random);
        
        usleep(200000);
        sem_post(&agent_ready);

	}

	return NULL;
}

// This semaphore gives the pusher exclusive access to the items on the table
sem_t pusher_lock;

/**
 * The pusher is responsible for releasing the proper sailor semaphore when the
 * right item's are on the table.
 */
void* pusher(void* arg)
{
	int pusher_id = *(int*) arg;

	for (int i = 0; i < 12; ++i)
	{
		// Wait for this pusher to be needed
		sem_wait(&pusher_semaphores[pusher_id]);
		sem_wait(&pusher_lock);

		// Check if the other item we need is on the table
		if (items_on_table[(pusher_id + 1) % 3])
		{
			items_on_table[(pusher_id + 1) % 3] = false;
			sem_post(&sailor_semaphors[(pusher_id + 2) % 3]);
		}
		else if (items_on_table[(pusher_id + 2) % 3])
		{
			items_on_table[(pusher_id + 2) % 3] = false;
			sem_post(&sailor_semaphors[(pusher_id + 1) % 3]);
		}
		else
		{
			// The other item's aren't on the table yet
			items_on_table[pusher_id] = true;
		}

		sem_post(&pusher_lock);
	}

	return NULL;
}

/**
 * The agent puts items on the table
 */
void* agent(void* arg)
{
	int agent_id = *(int*) arg;

	for (int i = 0; i < 6; ++i)
	{
		usleep(200000);

		// Wait for a lock on the agent
		sem_wait(&agent_ready);

		// Release the items this agent gives out
		

		// Say what type of items we just put on the table
		printf("\033[0;35m==> \033[0;33mAgent %d giving out %s\033[0;0m\n",
			agent_id, sailor_types[(agent_id + 2) % 3]);
		
		sem_post(&pusher_semaphores[agent_id]);
		sem_post(&pusher_semaphores[(agent_id + 1) % 3]);
	}

	return NULL;
}

/**
 * The main thread handles the agent's arbitration of items.
 */
int main(int argc, char* arvg[])
{
	// Seed our random number since we will be using random numbers
	srand(time(NULL));

	// There is only one agent semaphore since only one set of items may be on
	// the table at any given time. A values of 1 = nothing on the table
	sem_init(&agent_ready, 0, 1);

	// Initalize the pusher lock semaphore
	sem_init(&pusher_lock, 0, 1);

	// Initialize the semaphores for the sailors and pusher
	for (int i = 0; i < 3; ++i)
	{
		sem_init(&sailor_semaphors[i], 0, 0);
		sem_init(&pusher_semaphores[i], 0, 0);
	}



	// sailor ID's will be passed to the threads. Allocate the ID's on the stack
	int sailor_ids[6];

	pthread_t sailor_threads[6];

	// Create the 6 sailor threads with IDs
	for (int i = 0; i < 6; ++i)
	{
		sailor_ids[i] = i;

		if (pthread_create(&sailor_threads[i], NULL, sailor, &sailor_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	// Pusher ID's will be passed to the threads. Allocate the ID's on the stack
	int pusher_ids[6];

	pthread_t pusher_threads[6];

	for (int i = 0; i < 3; ++i)
	{
		pusher_ids[i] = i;

		if (pthread_create(&pusher_threads[i], NULL, pusher, &pusher_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	// Agent ID's will be passed to the threads. Allocate the ID's on the stack
	int agent_ids[3];

	pthread_t agent_threads[3];

	for (int i = 0; i < 3; ++i)
	{
		agent_ids[i] =i;

		if (pthread_create(&agent_threads[i], NULL, agent, &agent_ids[i]) == EAGAIN)
		{
			perror("Insufficient resources to create thread");
			return 0;
		}
	}

	// Make sure all the sailors are done smoking
	for (int i = 0; i < 6; ++i)
	{
		pthread_join(sailor_threads[i], NULL);
	}

	return 0;
}
