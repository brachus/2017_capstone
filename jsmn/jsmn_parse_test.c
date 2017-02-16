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

typedef struct json_parse_node
{
	int type; /* obj|arr|num|bool|null|str*/
	
	struct json_parse_node **values;
	char **keys;
	
	int cnt;
	
	int cur;/* for building tree */
	
	int int_dat;/*num|bool|null(-1)*/
	float float_dat;/*num*/
	char *str_dat;/*str*/
	
	struct json_parse_node *parent;
} json_parse_node;

enum
{
	JSON_PARSE_UND,
	JSON_PARSE_OBJ,
	JSON_PARSE_ARR,
	JSON_PARSE_NUM,
	JSON_PARSE_BOOL,
	JSON_PARSE_NULL,
	JSON_PARSE_STR
};

json_parse_node *new_json_parse_node(int type, int c, json_parse_node *parent)
{
	json_parse_node *n =
		(json_parse_node*) malloc(sizeof(json_parse_node));
	if (c<0)
		c=0;
	n->values=0;
	if (c)
		n->values=(json_parse_node**) malloc(sizeof(json_parse_node*) * c);
	n->keys=0;
	if (type==JSON_PARSE_OBJ)
		n->keys=(char **) malloc(sizeof(char*) * c);
	n->cnt=c;
	n->cur=0;
	n->type = type;
	n->int_dat=0;
	n->float_dat=.0;
	n->str_dat=0;
	n->parent = parent;
	return n;
}

json_parse_node *json_parse_mk_tree(const char *json, jsmntok_t *t, int ntokens)
{
	int cur = 1;
	char tchar[256];
	json_parse_node *root, *cur_node, *tmp;
	
	#define DO_TCHAR(curtoken, assignto) \
		sprintf(tchar, "%.*s", curtoken.end - curtoken.start, json + curtoken.start); \
		assignto = (char*)malloc(strlen(tchar)+2); \
		strcpy(assignto,  tchar );
	
	root = new_json_parse_node(JSON_PARSE_OBJ, t[0].size, 0);	
	cur_node = root;
		
	while (cur < ntokens && cur_node)
	{
		if (cur_node->type == JSON_PARSE_OBJ)
		{
			/* copy key string. */
			DO_TCHAR(t[cur], cur_node->keys[cur_node->cur] );		
			cur++;
		}
		if (cur_node->type == JSON_PARSE_ARR || cur_node->type == JSON_PARSE_OBJ)	
		{
			switch(t[cur].type)
			{
			case JSMN_OBJECT:
				tmp = new_json_parse_node(
					JSON_PARSE_OBJ,
					t[cur].size,
					cur_node);
				break;
			case JSMN_ARRAY:
				tmp = new_json_parse_node(
					JSON_PARSE_ARR,
					t[cur].size,
					cur_node);
				break;
			case JSMN_UNDEFINED:
				tmp = new_json_parse_node(JSON_PARSE_UND, 1, cur_node);
				break;
				/* if non-array/non-obj, fill in 
				 * data on next loop; don't step
				 * cur
				 */
			case JSMN_PRIMITIVE:
				tmp = new_json_parse_node(JSON_PARSE_NUM, 1, cur_node);
				DO_TCHAR(t[cur], tmp->str_dat );
				break;
			case JSMN_STRING:
				tmp = new_json_parse_node(JSON_PARSE_STR, 1, cur_node);
				DO_TCHAR(t[cur], tmp->str_dat );
				break;
			}
			cur_node->values[cur_node->cur] = tmp;
			cur_node->cur++;
			if (t[cur].type == JSMN_OBJECT ||
				t[cur].type == JSMN_ARRAY ||
				t[cur].type == JSMN_PRIMITIVE)
				cur_node=tmp;
			if (t[cur].type == JSMN_OBJECT ||
				t[cur].type == JSMN_ARRAY ||
				t[cur].type == JSMN_UNDEFINED ||
				t[cur].type==JSMN_STRING)
				cur++;
		}
		else if (cur_node->type == JSON_PARSE_NUM)
		{
			if (cur_node->str_dat[0] == 'f')
			{
				cur_node->int_dat = 0;
				cur_node->type = JSON_PARSE_BOOL;
			}
			else if (cur_node->str_dat[0] == 't')
			{
				cur_node->int_dat = 1;
				cur_node->type = JSON_PARSE_BOOL;
			}
			else if (cur_node->str_dat[0] == 'n')
			{
				cur_node->int_dat = 0;
				cur_node->type = JSON_PARSE_NULL;
			}
			else /* must be number */
			{
				cur_node->int_dat = atoi(cur_node->str_dat);
				cur_node->float_dat = atof(cur_node->str_dat);
				cur_node->type = JSON_PARSE_NUM;
			}
			free(cur_node->str_dat);
			cur_node->str_dat = 0;
			cur_node->cur++; /* will go back to parent */
			cur++;
		}
		while (cur_node && cur_node->cur >= cur_node->cnt)
			cur_node = cur_node->parent;
	}
	return root;
}

void json_parse_tree_print(json_parse_node *root)
{
	int i;
	switch (root->type)
	{
	case JSON_PARSE_ARR:
		printf("[");
		for (i=0;i<root->cnt;i++)
		{
			json_parse_tree_print(root->values[i]);
			if (i<root->cnt-1)
				printf(", ");
		}
		printf("]");
		break;
	case JSON_PARSE_OBJ:
		printf("{");
		for (i=0;i<root->cnt;i++)
		{
			if (root->keys[i])
				printf("%s : ", root->keys[i]);
			json_parse_tree_print(root->values[i]);
			if (i<root->cnt-1)
				printf(", ");
		}
		printf("}");
		break;
	case JSON_PARSE_NULL:
		printf("null");
		break;
	case JSON_PARSE_NUM:
		printf("%d",root->int_dat);
		break;
	case JSON_PARSE_STR:
		printf("\"%s\"",root->str_dat);
		break;
	case JSON_PARSE_BOOL:
		printf("%s",(root->int_dat) ? "true" : "false" );
		break;
	case JSON_PARSE_UND:
		printf("undef" );
		break;
	}
}

int main()
{
	int i, r;
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */

	jsmn_init(&p);
	r = jsmn_parse(&p, JSON_STRING, strlen(JSON_STRING), t, sizeof(t)/sizeof(t[0]));
	
	if (r < 0) 
	{
		printf("Failed to parse JSON: %d\n", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT)
	{
		printf("Object expected\n");
		return 1;
	}
	
	json_parse_node *test_tree = json_parse_mk_tree(JSON_STRING, t, r);
	
	json_parse_tree_print(test_tree);
	printf("\n");
	
	return EXIT_SUCCESS;
}
