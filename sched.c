#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "sched.h"
#include "pool.h"
#include <stdlib.h>

static int time = 0;

task *all_tasks = NULL;
task *curr = NULL;
task *last_task = NULL;

void sched_new(void (*entrypoint)(void *aspace),
			   void *aspace,
			   int priority,
			   int deadline)
{
	task *new = (task *)malloc(sizeof(task));
	new->next = all_tasks;
	new->prev = last_task;
	new->entrypoint = entrypoint;
	new->ctx = aspace;
	new->prior = priority;
	new->deadline = deadline;
	new->start = -1;

	if (all_tasks == NULL)
	{
		all_tasks = new;
		last_task = all_tasks;

		all_tasks->next = all_tasks;
		all_tasks->prev = all_tasks;

		last_task->next = all_tasks;
		last_task->prev = all_tasks;
	}
	else
	{
		last_task->next = new;
		last_task = new;
		all_tasks->prev = last_task;
	}
}

void sched_cont(void (*entrypoint)(void *aspace),
				void *aspace,
				int timeout)
{
	sched_new(entrypoint, aspace, curr->prior, curr->deadline);
	last_task->start = time + timeout;
}

void sched_time_elapsed(unsigned amount)
{
	time += amount;
}

void delete_task(task *delete)
{
	if (all_tasks == last_task)
	{
		free(all_tasks);
		all_tasks = NULL;
		return;
	}

	delete->prev->next = delete->next;
	delete->next->prev = delete->prev;

	if (all_tasks == delete)
		all_tasks = delete->next;

	if (last_task == delete)
		last_task = delete->prev;

	free(delete);
}

void run_fifo()
{
	curr = all_tasks;

	while (all_tasks != NULL)
	{
		if (curr->start > time)
		{
			curr = curr->next;
			continue;
		}
		
		curr->entrypoint(curr->ctx);
		task *delete = curr;
		curr = curr->next;
		delete_task(delete);
	}
}

task *find_max_prior()
{
	if (all_tasks == NULL)
		return NULL;

	task *start_state = all_tasks;
	task *ans = NULL;
	int max_prior = -1;

	do
	{
		if (all_tasks->prior > max_prior && all_tasks->start <= time)
		{
			max_prior = all_tasks->prior;
			ans = all_tasks;
		}

		all_tasks = all_tasks->next;
	} while (start_state != all_tasks);

	all_tasks = start_state;
	return ans;
}

void run_prio()
{
	for (task *curr = find_max_prior(); curr != NULL; curr = find_max_prior())
	{
		curr->entrypoint(curr->ctx);
		delete_task(curr);
	}
}

task *find_min_deadline()
{
	if (all_tasks == NULL)
		return NULL;

	task *start_state = all_tasks;
	task *ans = NULL;
	int min_deadline = INT_MAX;

	do
	{
		if (min_deadline > all_tasks->deadline && all_tasks->deadline > 0 && all_tasks->start <= time)
		{
			min_deadline = all_tasks->deadline;
			ans = all_tasks;
		}

		all_tasks = all_tasks->next;
	} while (start_state != all_tasks);

	all_tasks = start_state;
	return ans != NULL ? ans : find_max_prior();
}

void run_deadline()
{
	for (task *curr = find_min_deadline(); curr != NULL; curr = find_min_deadline())
	{
		curr->entrypoint(curr->ctx);
		delete_task(curr);
	}
}

void sched_run(enum policy policy)
{
	if (all_tasks == NULL)
		return;

	switch (policy)
	{
	case POLICY_FIFO:
		run_fifo();
		break;
	case POLICY_PRIO:
		run_prio();
		break;
	case POLICY_DEADLINE:
		run_deadline();
		break;
	}
}

