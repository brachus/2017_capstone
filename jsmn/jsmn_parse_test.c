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

struct json_parse_stack_node
{
	int size;/* values from jsmntok_t */
	int type;
	
	int left;
	
	int start_idx;
	int end_idx; /* last item */
	
	json_parse_node tree_node;
};

struct json_parse_stack
{
	struct json_parse_stack_node dat[8];
	int lvl;
	int cur;
	int size;
};

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



json_parse_node *new_json_parse_node(int type, int children, json_parse_node *parent)
{
	json_parse_node *n = (json_parse_node*) malloc(sizeof(json_parse_node));
	
	if (children<0)
		children=0;
	
	n->values=0;
	if (children)
		n->values=(json_parse_node**) malloc(sizeof(json_parse_node*) * children);
	
	n->keys=0;
	if (type==JSON_PARSE_OBJ)
		n->keys=(char **) malloc(sizeof(char*) * children);
		
	
	n->cnt=children;
	
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
	json_parse_node *root = new_json_parse_node(JSON_PARSE_OBJ, t[0].size, 0);	
	json_parse_node *cur_node = root, *tmp;
	
	#define DO_TCHAR(curtoken, assignto) \
		sprintf(tchar, "%.*s", curtoken.end - curtoken.start, json + curtoken.start); \
		assignto = (char*)malloc(strlen(tchar)+2); \
		strcpy(assignto,  tchar ); \
		
	while (cur < ntokens && cur_node)
	{
		printf("cur %d\n",cur);
		
		if (cur_node->type == JSON_PARSE_OBJ)
		{
			/* Copy key string. */
			DO_TCHAR(t[cur], cur_node->keys[cur_node->cur] );
			
			printf("  keystr: %s\n",cur_node->keys[cur_node->cur]);
			
			cur++;
		}
		if (cur_node->type == JSON_PARSE_ARR || cur_node->type == JSON_PARSE_OBJ)	
		{
			switch(t[cur].type)
			{
			case JSMN_OBJECT:
				printf("  new obj %d\n",t[cur].size	);
				tmp = new_json_parse_node(JSON_PARSE_OBJ, t[cur].size, cur_node); break;
			case JSMN_ARRAY:
				printf("  new arr\n");
				tmp = new_json_parse_node(JSON_PARSE_ARR, t[cur].size, cur_node); break;
			case JSMN_UNDEFINED:
				tmp = new_json_parse_node(JSON_PARSE_UND, 1, cur_node);
				break; /* if non-array/non-obj, fill in data on next loop; don't step cur */
			case JSMN_PRIMITIVE:
				tmp = new_json_parse_node(JSON_PARSE_NUM, 1, cur_node);
				DO_TCHAR(t[cur], tmp->str_dat );
				printf("  primitive %s\n",tmp->str_dat);
				break;
			case JSMN_STRING:
				tmp = new_json_parse_node(JSON_PARSE_STR, 1, cur_node);
				DO_TCHAR(t[cur], tmp->str_dat );
				printf("  string %s\n",tmp->str_dat);
				break;
			}
			cur_node->values[cur_node->cur] = tmp;
			cur_node->cur++;
			
			if (t[cur].type == JSMN_OBJECT || t[cur].type == JSMN_ARRAY || t[cur].type == JSMN_PRIMITIVE)
				cur_node=tmp;
			
			if (t[cur].type == JSMN_OBJECT || t[cur].type == JSMN_ARRAY || t[cur].type == JSMN_UNDEFINED || t[cur].type==JSMN_STRING)
				cur++;
				
		}
		else if (cur_node->type == JSON_PARSE_NUM)
		{
			printf("parsenum\n");
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
		
		while (cur_node->cur >= cur_node->cnt)
		{
			printf("cc %d %d %d ?\n", cur_node, cur_node->cur, cur_node->cnt);
			cur_node = cur_node->parent;
			if (!cur_node)
				break;
		}
			
	}
	
	return root;
}

void q_s_spc(int cnt)
{
	while (cnt--)
		printf(" ");
}

void json_parse_tree_print(json_parse_node *root, int lvl)
{
	int i;
	
	switch (root->type)
	{
	case JSON_PARSE_ARR:
		q_s_spc(lvl);
		printf("[\n");
		for (i=0;i<root->cnt;i++)
		{
			json_parse_tree_print(root->values[i], lvl+1);
			if (i<root->cnt-1)
				printf(",\n");
		}
		printf("\n"); q_s_spc(lvl); printf("]\n");
			
		break;
	case JSON_PARSE_OBJ:
		q_s_spc(lvl);
		printf("{\n");
		for (i=0;i<root->cnt;i++)
		{
			q_s_spc(lvl);
			if (root->keys[i])
				printf("%s : ", root->keys[i]);
			json_parse_tree_print(root->values[i], 0);
			if (i<root->cnt-1)
				printf(",\n");
		}
		printf("\n"); q_s_spc(lvl); printf("}\n");
		break;
	case JSON_PARSE_NULL:
		q_s_spc(lvl);
		printf("null ");
		break;
	case JSON_PARSE_NUM:
		printf("%d (%f) ",root->int_dat, root->float_dat);
		break;
	case JSON_PARSE_STR:
		printf("\"%s\" ",root->str_dat);
		break;
	case JSON_PARSE_BOOL:
		printf("%s ",(root->int_dat) ? "true" : "false" );
		break;
	case JSON_PARSE_UND:
		printf("undef " );
		break;
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
	
	json_parse_node *test_tree = json_parse_mk_tree(JSON_STRING, t, r);
	
	json_parse_tree_print(test_tree, 0);
		
		
	return EXIT_SUCCESS;
}
