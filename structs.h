
typedef struct main_state
{
	
	int game_md;
	
	/** title menu vars: **/
	
	/* [var to hold sound bite] */
	SDL_Surface **ani_a; /* vars for animated menu graphcs.  ani_a would be intro, ani_b would be looping */
	int ani_a_frame_cnt;
	SDL_Surface **ani_b;
	int ani_b_frame_cnt;
	int ani_cur_frame;
	char *options[6]; /* up to 6 string options */
	int option_md[6]; /* corresponding mode for each option.  these will end up be some enum. */
	
} main_state;

typedef struct xyz
{
	int x;
	int y;
	int z;
} xyz;

typedef struct sprite
{
	int cx;
	int cy;
	
	int transp;
	
	int frames;
	SDL_Surface **sprt_arr;
	int intrv;
	int loop;
	
	char name[32];
	
	
	/* attack/defend frames:  used as a way to express a region of attack for
	 * sprites used in character attack/defend animations.
	 * characters with their bounding box intersecting an attack frame's
	 * bounding box would be considered 'hurt'.
	 * the degree of being hurt, type of hurt, etc. will be determined by
	 * checking the parent character and its current mode, attack, defense, etc.  */
	
	int attk_frame_start;/* span of attack frame start.  0 >=.  time span includes start end values. */
	int attk_frame_end;
	int attk_frame_bbox_x; /* relative to center xy */
	int attk_frame_bbox_y;
	int attk_frame_bbox_w;
	int attk_frame_bbox_h;
	int attk_frame_bbox_z;
	
	int dfnd_frame_start;/* span of defend frame start.  0 >=.  time span includes start end values. */
	int dfnd_frame_end;
	int dfnd_frame_bbox_x; /* relative to center xy */
	int dfnd_frame_bbox_y;
	int dfnd_frame_bbox_w;
	int dfnd_frame_bbox_h;
	int dfnd_frame_bbox_z;
	
	xyz *drift; /* sprite may animate parent character*/
} sprite;

/* used for actual sprite instances. */
typedef struct sprite_active
{
	sprite *base;
	int active;
	int done;
	int cntr;
	int cur;
} sprite_active;

typedef struct actor
{
	int x;
	int y;
	int z;
	
	int dx;
	int dy;
	int dz;
	
	int bbox_w;
	int bbox_h; /* bounding box width/height.
				 * center of bbox is always x,y,z. 
				 * (more like a plane).
				 * 
				 * this is used for collisions/movement clipping.
				 */
	
	int md;

	int jump;
	
	int type;

	int active;
	
	/* for each of these, in order:
	 * 	facing left, 
	 * 	right, 
	 * 	up,
	 * 	down,
	 * 	up-left,
	 * 	up-right,
	 * 	down-left,
	 * 	down-right
	 */
	sprite *gfx_stand[8]; /* we should focus on these the most for now */
	sprite *gfx_walk[8];
	sprite *gfx_jump[8];
	
	sprite *gfx_run[8];
	
	sprite *main;
	
	char dir; /* uses DIR_ enums */
	char prev_dir;
} actor;

typedef struct ani_obj
{
	int idx; /* -1 for inactive*/
	int x, y;
} ani_obj;


typedef struct ani_cmd
{
	int cmd;
	char *fn;
	int idx;
	int x;
	int y;
	struct ani_cmd *next;
	
} ani_cmd;


typedef struct ani
{	
	ani_cmd *cmd;
	
	ani_obj *obj;/*64*/
	
	int obj_cnt;
	
	ani_cmd *cmd_cur;
	
	SDL_Surface **gfx_dat;/* 64 */
	
	int gfx_cnt;
	
	int done;
	
} ani;


typedef struct cam
{
	int x; /* x,y are the 'center' of the camera. */
	int y;
	
	int use_bounds;
	
	int bx0; /* top-left in world coordinates */
	int by0;
	int bx1; /* bottom-right in world coordinates */
	int by1;
	
	struct chara_active *target; /* if not null, override x,y and focus target as center. */
} cam;

typedef struct tilemap
{
	int x;/*x,y in global coords */
	int y;
	
	int w;
	int h;
	int ntiles;
	int **arr; /* [row] [column]*/
	SDL_Surface **tiles;
} tilemap;


typedef struct render_sprite
{
	short cx;
	short cy;
	int bx;
	int by;
	int x;
	int y;
	int transp; /* 0=no effect, 1|2=do shad effect */
	SDL_Surface *sprt;
	int active;
	
	struct render_sprite *next;
} render_sprite;

typedef struct render_sprite_head
{
	int shad_cnt;
	int sprt_cnt;
	render_sprite *next;
} render_sprite_head;


typedef struct controller
{
	int up;
	int down;
	int left;
	int right;
	
	int jump;
	
	int attk_a; /* jabs */
	int attk_b; /* mid  */
	int attk_c; /* full */
	
	int we; /* toggle through weapons */
	
	int inv; /* menu pop up, should be referred as "start" also. */
	
	int key_up;
	int key_down;
	int key_left;
	int key_right;
	int key_jump;
	int key_attk_a;
	int key_attk_b;
	int key_attk_c;
	int key_we;
	int key_inv;
	
	int set;
	
} controller;

typedef struct alignment
{
	int earth;
	int water;
	int wind;
	int fire;
	int light;
	int dark;
	
} alignment;

typedef struct sprite_lib_node
{
	sprite *dat;
	struct sprite_lib_node *next;
	
} sprite_lib_node;

typedef struct sprite_lib
{
	int cnt;
	sprite_lib_node *head;
	
} sprite_lib;

typedef struct chara_template
{
	sprite **gfx;
	int gfx_cnt; /* number of sprites character has. */
	
	int max_hp;
	int max_mp;
	
	int attack;
	int defend;
	
	int bbox_w;
	int bbox_h;
	int bbox_z; /* actual height, with bbox_w+h as a base. */
	
	int type;
	
} chara_template;

typedef struct chara_active
{
	alignment algnmt;
	chara_template *base;
	sprite_active **gfx;  /* sprite instances  */
	
	/* includes action/variation of an action, including walking/running
	 * in a particular direction, falling back from being hit, attacking
	 * with specific weapon, etc.
	 */
	int md;
	
	int max_hp;
	int max_mp;
	
	int hp;
	int mp;
	
	int attack;
	int defend;
	
	int lvl;
	
	xyz pos;
	
	xyz dpos;
	
} chara_active;

/* item roster; keeps track of items, grabbable or in inventory, */
typedef struct item
{
	int type;
	struct item *next;
	
	int in_world; /* if 1, item is viewed as a grabbable "in-world" item. */
	
} item;

typedef struct inventory
{
	item *store;
	int cnt;
	
} inventory;

typedef struct rexit
{
	int idx;
	struct room *dst; /* this eventually replaces idx as game data is processed. */
	xyz *loc; /* in global coords */
	
} rexit;

typedef struct room
{
	tilemap *main;
	
	tilemap *mult; /* use to tint sprites in specific locals around room, e.g. dark cave with spotty lighting. */
	
	/* exits */
	rexit *exits;
	int nexits;
	
	/* items */
	item *items;
	int nitems;
	
	/* npc's */
} room;

typedef struct world
{
	room **rooms;
} world;
