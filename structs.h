
typedef struct
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


typedef struct
{
	int cx;
	int cy;
	
	int transp;
	
	int frame_cnt;
	int frame_cur;
	char *fn_arr[256];
	SDL_Surface **sprt_arr;
	int intrv;
	int loop;
	int cnt0;
	
	int active;
} sprite;

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
	
	actor *target; /* if not null, override x,y and focus target as center. */
} cam;

typedef struct tilemap
{
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

