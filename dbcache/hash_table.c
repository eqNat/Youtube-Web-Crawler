#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "hash_table.h"

pthread_mutex_t realloc_lock;
pthread_mutex_t data_lock;

struct id_128 {
	int64_t lhalf;
	int64_t rhalf;
};

#define V_POWER 22
#define C_POWER 21
int64_t *v_hashmap[1 << V_POWER];
struct id_128 *c_hashmap[1 << C_POWER];

uint8_t v_column_power[1 << V_POWER]; // of base 2
uint8_t c_column_power[1 << C_POWER]; //

__attribute__ ((constructor))
void init_v_hashmap()
{
	printf("initializing hashmaps...\n");
	for (int64_t i = 0; i < (1 << V_POWER); i++)
		v_hashmap[i] = (int64_t *) calloc(sizeof(int64_t), 1);

	for (int64_t i = 0; i < (1 << C_POWER); i++)
		c_hashmap[i] = (struct id_128 *) calloc(sizeof(struct id_128), 1);
}

uint64_t v_table_count = 0;
uint64_t c_table_count = 0;

_Bool video_insert(int64_t data)
{
	int64_t row = data & ((1 << V_POWER) - 1);
	int64_t column = 0;

	while (1) { // loops if another thread steals its insertion spot
		while (v_hashmap[row][column]) {
			if (data == v_hashmap[row][column])
				return 0;
			if (++column != (1 << v_column_power[row]))
				continue;

			pthread_mutex_lock(&realloc_lock);
			// make sure another thread didn't already reallocate
			if (column != (1 << v_column_power[row])) { 
				pthread_mutex_unlock(&realloc_lock);
				continue;
			}
			// double array
			int64_t extra_bytes = sizeof(int64_t)*column;
			v_hashmap[row] = (int64_t *) reallocarray(v_hashmap[row], extra_bytes, 2);
			memset(&v_hashmap[row][column], 0, extra_bytes);
			v_column_power[row]++;
			pthread_mutex_unlock(&realloc_lock);
		}

		pthread_mutex_lock(&data_lock);
		// make sure another thread didn't steal its spot
		if (!v_hashmap[row][column]) { 
			v_hashmap[row][column] = data;
			v_table_count++;
			pthread_mutex_unlock(&data_lock);
			return 1;
		}
		pthread_mutex_unlock(&data_lock);
	}
}

_Bool channel_insert(int64_t lhalf, int64_t rhalf)
{
	int64_t row = rhalf & ((1 << C_POWER) - 1);
	int64_t column = 0;

	while (1) { // loops if another thread steals its insertion spot
		while (c_hashmap[row][column].lhalf || c_hashmap[row][column].rhalf) {
			if (lhalf == c_hashmap[row][column].lhalf && rhalf == c_hashmap[row][column].rhalf)
				return 0;
			if (++column != (1 << c_column_power[row]))
				continue;

			pthread_mutex_lock(&realloc_lock);
			// make sure another thread didn't already reallocate
			if (column != (1 << c_column_power[row])) { 
				pthread_mutex_unlock(&realloc_lock);
				continue;
			}
			// double array
			int64_t extra_bytes = sizeof(struct id_128)*column;
			c_hashmap[row] = (struct id_128 *) reallocarray(c_hashmap[row], extra_bytes, 2);
			memset(&c_hashmap[row][column], 0, extra_bytes);
			c_column_power[row]++;
			pthread_mutex_unlock(&realloc_lock);
		}

		pthread_mutex_lock(&data_lock);
		// make sure another thread didn't steal its spot
		if (!(c_hashmap[row][column].lhalf || c_hashmap[row][column].rhalf)) { 
			c_hashmap[row][column].lhalf = lhalf;
			c_hashmap[row][column].rhalf = rhalf;
			c_table_count++;
			pthread_mutex_unlock(&data_lock);
			return 1;
		}
		pthread_mutex_unlock(&data_lock);
	}
}
