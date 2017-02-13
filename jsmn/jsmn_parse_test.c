#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"

/*
 * A small example of jsmn parsing when JSON structure is known and number of
 * tokens is predictable.
 */

static const char *JSON_STRING =
"{\
	 \"renderorder\":\"right-down\",\
 \"tileheight\":24,\
 \"tilesets\":[\
        {\
         \"firstgid\":1,\
         \"image\":\"desert_example.png\",\
         \"imageheight\":256,\
         \"imagewidth\":256,\
         \"margin\":0,\
         \"name\":\"desert_example\",\
         \"properties\":\
            {\
\
            },\
         \"spacing\":0,\
         \"tileheight\":24,\
         \"tilewidth\":24\
        }],\
 \"tilewidth\":24,\
 \"version\":1,\
 \"width\":32\
}";

struct json_parse_stack_node
{
	int size;/* values from jsmntok_t */
	int type;
	
	int left;
	
	int start_idx;
	int end_idx; /* last item */
};

struct json_parse_stack
{
	struct json_parse_stack_node dat[8];
	int lvl;
	int cur;
	int size;
};

#define CURLVL stack->dat[stack->lvl] 

void json_parse_print_current(struct json_parse_stack *stack, jsmntok_t *t, const char *json)
{
	if (stack->lvl<0)
		return;
	
	if (CURLVL.type == JSMN_OBJECT)
	{
		printf("%.*s : %.*s\n",
				t[stack->cur].end - t[stack->cur].start,
				json + t[stack->cur].start,
				t[stack->cur+1].end - t[stack->cur+1].start,
				json + t[stack->cur+1].start
				);
	}
	
	else if (CURLVL.type == JSMN_ARRAY)
	{
		printf("%.*s",
				t[stack->cur].end - t[stack->cur].start,
				json + t[stack->cur].start
				);
	}
}

void json_parse_next_item_same_level(struct json_parse_stack *stack, jsmntok_t *t)
{
	int target = stack->lvl;
	
	
	
	while (stack->cur < stack->size)
	{
		if (CURLVL.type == JSMN_OBJECT)
		{
			CURLVL.left--;
			if (t[stack->cur + 1].type == JSMN_OBJECT ||
				t[stack->cur + 1].type == JSMN_ARRAY)
			{
				stack->lvl++;
				CURLVL.type = t[stack->cur + 1].type;
				CURLVL.left = t[stack->cur + 1].size;
			}
			
			stack->cur+=2;
			
			
		}
		else if (CURLVL.type == JSMN_ARRAY)
		{
			CURLVL.left--;
			
			if (t[stack->cur].type == JSMN_OBJECT ||
				t[stack->cur].type == JSMN_ARRAY)
			{
				
				stack->lvl++;
				CURLVL.type = t[stack->cur].type;
				CURLVL.left = t[stack->cur].size;
			}	
				
			stack->cur++;
		}
		
		if (CURLVL.left==0)
			while (--(stack->lvl))
				if (CURLVL.left > 0)
					break;
				
		if (target == stack->lvl)
			return;
	}
}

int json_get(const char *json, jsmntok_t *tok)
{
	if (tok->type==JSMN_UNDEFINED)
		printf("und %d: ",tok->size);
	if (tok->type==JSMN_OBJECT)
		printf("obj %d: ",tok->size);
	if (tok->type==JSMN_ARRAY)
		printf("arr %d: ",tok->size);
	if (tok->type==JSMN_STRING)
		printf("str %d: ",tok->size);
	if (tok->type==JSMN_PRIMITIVE)
		printf("prm %d: ",tok->size);
	printf("%.*s\n", tok->end - tok->start, json + tok->start);
}

int main() {
	int i;
	int r;
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	struct json_parse_stack stack;

	jsmn_init(&p);
	r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t, sizeof(t)/sizeof(t[0]));
	if (r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return 1;
	}
	
	stack.lvl=0;
	stack.cur=1;
	stack.size=r;
	stack.dat[0].type = t[0].type;
	stack.dat[0].left = t[0].size;

	/* Loop over all keys of the root object */
	while(1)
	{
		json_parse_print_current(&stack, t, JSON_STRING);
		
		json_parse_next_item_same_level(&stack, t);	
		
		
		if (stack.lvl<0)
			break;
	}
		
		
	return EXIT_SUCCESS;
}
