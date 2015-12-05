// Author: Phillip Stewart
// CPSC 351, Operating Systems
// Project 2

// All necessary functions are contained in this main.c,
// and the project can be compiled using:
//  ~$ gcc main.c


#include "stdio.h"
#include "stdlib.h"

// Holds relevant process information
typedef struct process_info {
	int p_num;
	int arrive_time;
	int life_time;
	int start_time;
	int mem_req;
	struct process_info* next;
} node;


// Generator for process_info structs
node* make_node(int num, int arrive, int life, int mem)
{
	node* p = (node*) malloc(sizeof(node));
	p->p_num = num;
	p->arrive_time = arrive;
	p->life_time = life;
	p->mem_req = mem;
	p->start_time = 1000000;
	p->next = 0;
	return p;
}


// Writes the 'Input Queue: [1, 2, 3]' line to file
void write_input_q(FILE* fp, int* q)
{
	int n, i = 0;
	char buffer[80];
	fwrite("\tInput Queue:[", sizeof(char), 14, fp);
	if (q[i]) {
		n = sprintf(buffer, "%d", q[i]);
		fwrite(buffer, sizeof(char), n, fp);
		i++;
	}
	while (q[i]) {
		n = sprintf(buffer, ", %d", q[i]);
		fwrite(buffer, sizeof(char), n, fp);
		i++;
	}
	fwrite("]\n", sizeof(char), 2, fp);
}


/* Writes the memory map to file, ex:
	Memory map:
		0-199:   Process 1, Page 1
		200-399: Process 1, Page 2
		400-999: Free frame(s)
*/
void write_mem_map(FILE* fp, int* map, int num_pages, int page_size)
{
	int from, to, n;
	char buffer[80];
	int p_counters[300];
	for (int i=0; i < 300; i++) {
		p_counters[i] = 1;
	}
	fwrite("\tMemory map:\n", sizeof(char), 13, fp);
	int i = 0;
	while (i < num_pages) {
		if (map[i]) {
			from = i * page_size;
			to = (i+1) * page_size - 1;
			n = sprintf(buffer, "\t\t%d-%d: Process %d, Page %d\n", from, to, map[i], p_counters[map[i]-1]);
			fwrite(buffer, sizeof(char), n, fp);
			++p_counters[map[i]-1];
			i++;
		} else {
			from = i * page_size;
			while(map[i] == 0 && i < num_pages) i++;
			to = i * page_size - 1;
			n = sprintf(buffer, "\t\t%d-%d: Free frame(s)\n", from, to);
			fwrite(buffer, sizeof(char), n, fp);
		}
	}
}


// Reads mem_size and page_size from stdin
void get_info(int* m, int* p)
{
	char buffer[80];
	printf("Memory size>");
	fgets(buffer, 80, stdin);
	sscanf(buffer, "%d", m);
	printf("Page Size (1:100, 2:200, 3:400)>");
	fgets(buffer, 80, stdin);
	sscanf(buffer, "%d", p);
}


// Reads process information from input file
void scan_file(FILE* fp, int* n, node** head_ptr)
{
	char buffer[80];
	char* b2;
	int p_num;
	int arrival;
	int length;
	int num_mems;
	int mem_scan;
	int mem;

	fgets(buffer, 80, fp);
	sscanf(buffer, "%d", n);

	fgets(buffer, 80, fp);
	sscanf(buffer, "%d", &p_num);

	fgets(buffer, 80, fp);
	sscanf(buffer, "%d %d", &arrival, &length);

	fgets(buffer, 80, fp);
	sscanf(buffer, "%d", &num_mems);

	b2 = buffer + 2;
	mem = 0;
	for (int i=0; i < num_mems; i++) {
		sscanf(b2, "%d", &mem_scan);
		while (*b2 != ' ') b2++;
		b2++;
		mem += mem_scan;
	}
	fgets(buffer, 80, fp);

	*head_ptr = make_node(p_num, arrival, length, mem);
	node* t = *head_ptr;
	for (int i=1; i < *n; i++) {
		fgets(buffer, 80, fp);
		sscanf(buffer, "%d", &p_num);

		fgets(buffer, 80, fp);
		sscanf(buffer, "%d %d", &arrival, &length);

		fgets(buffer, 80, fp);
		sscanf(buffer, "%d", &num_mems);

		b2 = buffer + 2;
		mem = 0;
		for (int i=0; i < num_mems; i++) {
			sscanf(b2, "%d", &mem_scan);
			while (*b2 != ' ') b2++;
			b2++;
			mem += mem_scan;
		}
		fgets(buffer, 80, fp);
		t->next = make_node(p_num, arrival, length, mem);
		t = t->next;
	}
}


// Returns the size of free memory availabe
int get_mem_size(int* pages, int num_pages, int page_size) {
	int total = 0;
	for (int i=0; i < num_pages; i++)
		if (!pages[i])
			total += page_size;

	return total;
}


// Push an int to the back of an array
// -note: does not check bounds.
void push_back(int* q, int p)
{
	int i = 0;
	while (q[i]) i++;
	q[i] = p;
}

// Pops an int from an array at index i
// -note: assumes sentinel 0 at end of array
int pop(int* q, int i)
{
	int p = q[i];
	while (q[i]) {
		q[i] = q[i+1];
		i++;
	}
	return p;
}


/* Runs the memory simulation, the main loop is:
while (t <= 100000):
	Remove any completed processes from memory
	Add new processes to the input queue
	If a process completed or arrived:
		Add any waiting processes that fit in memory.
*/
void run_mem_sim(FILE* fp, node* head, int num_processes, int mem_size, int page_size)
{
	int n, i;
	int pages[300];
	int input_q[21];
	int turn_times[20];
	for (i=0; i < 20; i++) {
		turn_times[i] = 0;
	}
	for (i=0; i < 21; i++) {
		input_q[i] = 0;
	}
	for (i=0; i < 300; i++) {
		pages[i] = 0;
	}
	int num_pages = mem_size / page_size;
	char buffer[80];
	node* trav;
	int arrived = 0;
	int completed = 0;
	int num_completed = 0;
	int t = 0;
	while (t <= 100000) {
		//check for completions
		trav = head;
		while (trav) {
			if (trav->start_time + trav->life_time == t) {
				turn_times[trav->p_num-1] = t - trav->arrive_time;
				if (!completed) {
					completed = 1;
					n = sprintf(buffer, "t = %d: ", t);
					fwrite(buffer, sizeof(char), n, fp);
				} else {
					fwrite("\t", sizeof(char), 1, fp);
				}
				n = sprintf(buffer, "Process %d completes\n", trav->p_num);
				fwrite(buffer, sizeof(char), n, fp);
				for (i = 0; i < num_pages; i++) {
					if (pages[i] == trav->p_num) {
						pages[i] = 0;
					}
				}
				write_mem_map(fp, pages, num_pages, page_size);
				num_completed++;
			}
			trav = trav->next;
		}

		//check for arrivals
		trav = head;
		while (trav) {
			if (trav->arrive_time == t) {
				i = 0;
				while (input_q[i]) i++;
				input_q[i] = trav->p_num;
				if (!(arrived || completed)) {
					arrived = 1;
					n = sprintf(buffer, "t = %d: ", t);
					fwrite(buffer, sizeof(char), n, fp);
				} else {
					fwrite("\t", sizeof(char), 1, fp);
				}
				n = sprintf(buffer, "Process %d arrives\n", trav->p_num);
				fwrite(buffer, sizeof(char), n, fp);
				write_input_q(fp, input_q);
				//write_mem_map(fp, pages, num_pages, page_size);
			}
			trav = trav->next;
		}

		if (num_processes == num_completed) break;
		t++;
		if (!(arrived || completed)) continue;

		//move processes
		i = 0;
		while (input_q[i]) {
			trav = head;
			while (1) {
				if (input_q[i] == trav->p_num) break;
				trav = trav->next;
				if (!trav) {
					//print error and exit
					printf("Error moving process\n");
					exit(1);
				}
			}
			if (trav->mem_req <= get_mem_size(pages, num_pages, page_size)) {
				//add into pages
				trav->start_time = t-1;
				pop(input_q, i);
				i--;
				int pages_needed = trav->mem_req / page_size;
				if (trav->mem_req % page_size) pages_needed++;
				int j = 0;
				for (int k = 0; k < pages_needed; k++) {
					while (pages[j]) j++;
					if (j >= num_pages) {
						printf("Error mapping pages\n");
						exit(1);
					}
					pages[j] = trav->p_num;
				}
				n = sprintf(buffer, "\tMM moves process %d to memory\n", trav->p_num);
				fwrite(buffer, sizeof(char), n, fp);
				write_input_q(fp, input_q);
				write_mem_map(fp, pages, num_pages, page_size);
			}
			i++;
		}

		arrived = 0;
		completed = 0;
	}

	// Processes that did not fit in memory may not have run
	// Also, very long runnng processes may have run over the time limit
	while (input_q[0]) {
			n = sprintf(buffer, "Process %d not completed\n", input_q[0]);
			fwrite(buffer, sizeof(char), n, fp);
			pop(input_q, 0);
	}

	//write turnaround times
	int total = 0;
	for (i=0; i < num_processes; i++) {
		total += turn_times[i];
	}
	double avg_time = (double) total / num_processes;
	n = sprintf(buffer, "Average turnaround: %f, (%d/%d)\n", avg_time, total, num_processes);
	fwrite(buffer, sizeof(char), n, fp);
}



int main()
{
	int i;
	int num_processes;
	int mem_size;
	int page_size;
	node* head;
	char buffer[80];

	//get mem_size and page size
	get_info(&mem_size, &page_size);

	//get input file
	printf("Input file: ");
	fgets(buffer, 80, stdin);
	i = 0;
	while (buffer[i] != '\n') i++;
	buffer[i] = 0;
	FILE* fp = fopen(buffer, "r");
	if (!fp) {
		printf("Error opening %s\n", buffer);
		exit(1);
	}
	
	//scan file for info
	scan_file(fp, &num_processes, &head);
	fclose(fp);
	switch (page_size) {
		case 1:
			page_size = 100;
			fp = fopen("out1.txt", "w");
			break;
		case 2:
			page_size = 200;
			fp = fopen("out2.txt", "w");
			break;
		case 3:
			page_size = 400;
			fp = fopen("out3.txt", "w");
			break;
		default:
			printf("Invalid selection\n");
			exit(1);
	}
	

	run_mem_sim(fp, head, num_processes, mem_size, page_size);

	fclose(fp);
	printf("Simulation completed.\n");

	// let malloc'd memory free itself on program exit...

	return 0;
}
