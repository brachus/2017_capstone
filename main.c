
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdio.h>
#include <string.h>

#define SWIDTH 320
#define SHEIGHT 240

#define TILESIZE 16

#define TILEMAPSIZE 1024
#define TILEMAPNTILES 32

#define FPS 60

enum
{
	MD_LOGO,
	MD_MENU,
	MD_OPTIONS,
	MD_SEL_CHARS,
	MD_DEATHMATCH,
	
	SBMD_PAUSED,
	
	SBMD_TRANS_A_OUT,
	SBMD_TRANS_A_IN
};

enum
{
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UP,
	DIR_DOWN,
	DIR_UPLEFT,
	DIR_UPRIGHT,
	DIR_DOWNLEFT,
	DIR_DOWNRIGHT
};

enum
{
	CH_STAND,
	CH_WALK,
	CH_JUMP
};

enum
{
	FADE_NULL,
	FADE_IN,
	FADE_OUT
};

void tick_frame(int fps)
{
	static unsigned int prevtime;
	static int first = 1;
	int tmp;
	if (!first)
	{
		tmp = (1000/fps) - (SDL_GetTicks() - prevtime);
		if (tmp>0)
			SDL_Delay( tmp );
	}
		
	else
		first = 0;
	prevtime = SDL_GetTicks();
}

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

enum
{
	ANI_LOAD,
	ANI_POST,
	ANI_UNPOST,
	ANI_NEXT
};

typedef struct ani_cmd
{
	int cmd;
	char *fn;
	int idx;
	int x;
	int y;
	struct ani_cmd *next;
	
} ani_cmd;

ani_cmd *new_ani_cmd()
{
	ani_cmd *n = (ani_cmd *) malloc(sizeof(ani_cmd));
	
	n->cmd = -1;
	n->fn = 0;
	n->idx = -1;
	n->x = 0;
	n->y = 0;
	n->next = 0;
	
	return n;
}

/* pointer passed must be set to 0 outside of function */
void free_ani_cmd(ani_cmd *head)
{
	ani_cmd *a, *b;
	
	if (!head)
		return;
	
	a = head;
	
	while (a)
	{
		b = a->next;
		if (a->fn)
			free(a->fn);
		free(a);
		a=b;
	}
}

int parse_for_ani_cmd(ani_cmd *cmd, const char *line, int start)
{
	char buf[256];
	int buf_nxt = 0;
	int buf_is_int = 1;
	int int_arg = 0;
	int ln;
	
	int line_nxt = start;
	
	ln=strlen(line);
	
	while (line_nxt < ln)
	{
		if (line[line_nxt] == ' ' || line[line_nxt] == ';')
		{
			/* make comparisons if buffer has some stuff in it */
			if (buf_nxt > 0)
			{
				buf[buf_nxt]='\0';
				
				if (cmd->cmd==-1)
				{
					if (!strcmp(buf, "load") || !strcmp(buf, "l"))
						cmd->cmd = ANI_LOAD;
					if (!strcmp(buf, "post") || !strcmp(buf, "p"))
						cmd->cmd = ANI_POST;
					if (!strcmp(buf, "unpost") || !strcmp(buf, "u"))
						cmd->cmd = ANI_UNPOST;
					if (!strcmp(buf, "next") || !strcmp(buf, "n"))
						cmd->cmd = ANI_NEXT;
				}
				else
				{
					if (buf_is_int)
					{
						if (int_arg==0)
							cmd->idx = atoi(buf);
						if (int_arg==1)
							cmd->x = atoi(buf);
						if (int_arg==2)
							cmd->y = atoi(buf);
							
						int_arg++;
					}
					else
					{
						cmd->fn = malloc(strlen(buf) + 2);
						strcpy(cmd->fn, buf);
					}
				}
				
				buf_nxt=0;
				buf_is_int=1;
			}
		}
		else
		{
			if (!(line[line_nxt] >= '0' && line[line_nxt] <= '9'))
				buf_is_int = 0;
			
			buf[buf_nxt] = line[line_nxt];
			
			if (buf_nxt<255)
				buf_nxt++;
		}
		
		if (line[line_nxt] == ';')
		{
			cmd->next = new_ani_cmd();
			cmd = cmd->next;
			int_arg = 0;
		}
		
		line_nxt++;
	}
	
	return line_nxt;
}

typedef struct ani
{	
	ani_cmd *cmd;
	
	ani_obj *obj;/*64*/
	
	int obj_cnt;
	
	ani_cmd *cmd_cur;
	
	SDL_Surface **gfx_dat;/* 64 */
	
	int gfx_cnt;
	
} ani;

ani *new_ani(char *script)
{
	int i, ln;
	
	ln=strlen(script);
		
	ani *n = (ani*) malloc(sizeof(ani));
	
	n->obj = (ani_obj*) malloc(sizeof(ani_obj) * 64);
	
	for (i=0;i<64;i++)
	{
		n->obj[i].idx=-1;
		n->obj[i].x=0;
		n->obj[i].y=0;
	}
	
	n->obj_cnt = 0;
		
	n->cmd=new_ani_cmd();
		
	parse_for_ani_cmd(n->cmd,script,0);
		
	n->cmd_cur = n->cmd;
	
	n->gfx_dat =  (SDL_Surface**) malloc(sizeof(SDL_Surface*) * 64);
	
	n->gfx_cnt = 0;
	
	return n;
	
}

void free_ani(ani *in)
{
	int i;
	
	free_ani_cmd(in->cmd);
	
	free(in->obj);
	
	for (i=0;i<64;i++)
		if (in->gfx_dat[i])
			SDL_FreeSurface(in->gfx_dat[i]);
	
	free(in->gfx_dat);
	
	free(in);
}

void ani_frame(ani *in, SDL_Surface *dst)
{
	int i, lp = 1;
	SDL_Rect tpos;
	
	while (in->cmd_cur && lp)
	{
		switch(in->cmd_cur->cmd)
		{
		case ANI_LOAD:
			if (in->cmd_cur->fn)
				if (in->gfx_cnt < 64)
					in->gfx_dat[(in->gfx_cnt)++] = IMG_Load(in->cmd_cur->fn);
			break;
		case ANI_POST:
			for(i=0;i<64;i++)
			{
				if (in->obj[i].idx == -1)
				{
					in->obj[i].idx = in->cmd_cur->idx;
					in->obj[i].x = in->cmd_cur->x;
					in->obj[i].y = in->cmd_cur->y;
					
					break;
				}
			}
			break;
		case ANI_UNPOST:
			for(i=0;i<64;i++)
				if (in->obj[i].idx == in->cmd_cur->idx)
					in->obj[i].idx = -1;
			break;
		case ANI_NEXT:
			lp=0;
			break;
		}
		
		in->cmd_cur = in->cmd_cur->next;
	}
	
	for (i=0;i<64;i++)
	{
		if (in->obj[i].idx >= 0 && in->gfx_dat[in->obj[i].idx])
		{
			tpos.x=in->obj[i].x;
			tpos.y=in->obj[i].y;
			SDL_BlitSurface(in->gfx_dat[in->obj[i].idx],0 , dst, &tpos);
		}
	}
	
	
	
}

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

void reset_actor(actor *in)
{
	int i;
	
	in->x=0;
	in->y=0;
	in->z=0;
	in->dx=0;
	in->dy=0;
	in->dz=0;
	
	in->bbox_w=0;
	in->bbox_h=0;
	
	in->dir=DIR_DOWN;
	in->prev_dir=DIR_DOWN;
	in->md=CH_STAND;
	in->jump=0;
	
	for (i=0;i<8;i++)
	{
		in->gfx_stand[i] = 0;
		in->gfx_walk[i] = 0;
		in->gfx_jump[i] = 0;
		in->gfx_run[i] = 0;
	}
	
}

void reset_cam(cam *in)
{
	in->x=0;
	in->y=0;
	in->use_bounds=0;
	in->target=0;
}

void clear_tilemap(tilemap *in)
{
	int x, y;

	for (x=0;x<256;x++)
		for (y=0;y<256;y++)
			in->arr[x][y] = 0;
	
	for (x=0;x<32;x++)
		in->tiles[x] = 0;
}

tilemap * new_tilemap()
{
	int i;
	tilemap *n;
	
	n = (tilemap*) malloc(sizeof(tilemap));
	
	n->w = TILEMAPSIZE;
	n->h = TILEMAPSIZE;
	
	n->ntiles = TILEMAPNTILES;
	
	n->arr = (int**) malloc(sizeof(int *) * n->h );
	
	for (i=0;i<n->w;i++)
		n->arr[i] = (int*) malloc(sizeof(int) * n->w);
	
	n->tiles = (SDL_Surface **) malloc(sizeof(SDL_Surface *) * n->ntiles);
	
	return n;
		
};



void clear_render_sprites(render_sprite_head *in)
{
	render_sprite *tmpa, *tmpb;
	
	tmpa = in->next;
	
	while (tmpa)
	{
		tmpb=tmpa->next;
		free(tmpa);
		tmpa=tmpb;
	}
	
	in->next = 0;	
	in->shad_cnt=0;
	in->sprt_cnt=0;
}

void add_sprite_auto_shadow(
	render_sprite_head *in,
	sprite *sprt,
	sprite *shad,
	cam *incam,
	int x,
	int y,
	int z  )
{
	int i,rx,ry,bry,sx,sy,cx,cy;
	
	if (z<0)
		return;
	
	/* adjust cam */
	if (incam->target)
	{
		cx = incam->target->x;
		cy = incam->target->y;
	}
	else
	{
		cx = incam->x; /* take into account cam bounding box here ? */
		cy = incam->y;
	}
	
	
	rx = (SWIDTH/2) + x - cx;
	ry = (SHEIGHT/2) - y - z + cy;
	bry = (SHEIGHT/2) - y + cy;
	
	/* step animation */
	if (sprt->cnt0++ >= sprt->intrv)
	{
		sprt->cnt0=0;
		
		sprt->frame_cur++;
		
		if (sprt->frame_cur >= sprt->frame_cnt)
		{
			if (sprt->loop)
				sprt->frame_cur=0;
			else
				sprt->frame_cur=sprt->frame_cnt-1;
		}
			
	}
	
	#define NEW_RENDER_SPRITE(a,b,c,d,e,f)  \
	{  \
		a = (render_sprite*) malloc(sizeof(render_sprite));  \
		a->transp = b->transp;  \
		a->sprt = b->sprt_arr[b->frame_cur];  \
		a->x = d;  \
		a->y = e;  \
		a->by = f;  \
		a->cx = b->cx;  \
		a->cy = b->cy;  \
		a->next = c;  \
	}
	
	if (!(in->next))
		NEW_RENDER_SPRITE(in->next,sprt,0,rx,ry,bry)
	
	else
	{	
		render_sprite *tmp, *tmpprev;
		int j = in->shad_cnt;
		
		tmpprev=0;
		tmp = in->next;
		
		while (j>0) /* skip over 'shadow' sprites */
		{
			tmpprev = tmp;
			tmp = tmpprev->next;
			j--;
		}
		
		if (!tmp) /* only shadow sprites */
		{
			NEW_RENDER_SPRITE(
				tmpprev->next,
				sprt,
				0,
				rx,ry,
				bry)
		}
		else
		{
			while (tmp)
			{
				if (tmp->by > ry)
				{
					if (!tmpprev)
						NEW_RENDER_SPRITE(in->next,sprt,tmp,rx,ry,bry)
					else
						NEW_RENDER_SPRITE(tmpprev->next,sprt,tmp,rx,ry,bry)
					break;
				}
				if (!tmp->next)
				{
					NEW_RENDER_SPRITE(tmp->next,sprt,0,rx,ry,bry)
					break;
				}
				
				tmpprev=tmp;
				tmp = tmp->next;
			}
		}
	}
	
	/* do shadow */
	
	if (z>0 && shad)
	{
		sx = (SWIDTH/2) + x - cx;
		sy = (SHEIGHT/2) - y + cy;
		
		{
			render_sprite *tmp;
			
			/* bring to front of render sprite list */
			tmp = in->next;
			
			if (!tmp)
				NEW_RENDER_SPRITE(in->next,shad,0,sx,sy,sy)
			else
				NEW_RENDER_SPRITE(in->next,shad,tmp,sx,sy,sy)
			
			(in->shad_cnt)++;
		}	
	}	
}

void tilemap_box_modify(tilemap *in, int x0, int y0, int x1, int y1, int val)
{
	int trow, tcol;
	
	for (trow=y0;trow<=y1;trow++)
		for(tcol=x0;tcol<=x1;tcol++)
			in->arr[trow][tcol] = val;
	
}


void render_rsprite_list(SDL_Surface *surf, render_sprite_head *sprh, int shad_tick)
{
	/* render sprites from render sprite list */
	SDL_Rect pos;
	render_sprite *tmp = sprh->next;
	
	while (tmp)
	{
		pos.x = tmp->x - tmp->cx;
		pos.y = tmp->y - tmp->cy;
		
		if (tmp->transp == 0 || shad_tick > 0)
			SDL_BlitSurface(tmp->sprt,0 , surf, &pos);
		
		tmp = tmp->next;
	}
}

void render_tilemap(SDL_Surface *surf, tilemap *intmap, cam *incam)
{
	int trow=0,tcol=0,camx,camy;
	SDL_Surface *ttmp;
	SDL_Rect pos;
	
	for (trow=0;trow<TILEMAPSIZE;trow++)
		for (tcol=0;tcol<TILEMAPSIZE;tcol++)
		{
			if (  intmap->arr[trow][tcol] >= 0 && 
				  intmap->arr[trow][tcol] < TILEMAPNTILES)
				ttmp = intmap->tiles[intmap->arr[trow][tcol]];
			else
				ttmp = 0;
				
			/* adjust cam */
			if (incam->target)
			{
				camx = incam->target->x;
				camy = incam->target->y;
			}
			else
			{
				camx = incam->x; /* take into account cam bounding box here ? */
				camy = incam->y;
			}
			
			pos.y = SHEIGHT/2 + (trow*TILESIZE) + camy;
			pos.x = SWIDTH/2 + (tcol*TILESIZE) - camx;
			
			if (pos.y<SHEIGHT && (pos.y+TILESIZE) >= 0 && 
				pos.x<SWIDTH && (pos.x+TILESIZE) >= 0 && ttmp)
				SDL_BlitSurface(ttmp,0 , surf, &pos);
		}
}

void render_text(SDL_Surface *dst, SDL_Color *fg, SDL_Color *bg, TTF_Font *font, char *msg, int x, int y)
{
	SDL_Rect tmppos;
	SDL_Surface *font_surf;
	
	tmppos.x=x; tmppos.y=y;
	
	font_surf = TTF_RenderText_Solid(font, msg, *bg);
	SDL_BlitSurface(font_surf,0 , dst, &tmppos);
	SDL_FreeSurface(font_surf);
	
	tmppos.x--; tmppos.y++;
	
	font_surf = TTF_RenderText_Solid(font, msg, *fg);
	
	SDL_BlitSurface(font_surf,0 , dst, &tmppos);
	
	tmppos.x--;
	SDL_BlitSurface(font_surf,0 , dst, &tmppos);
	
	
	SDL_FreeSurface(font_surf);
}

/* both a and b range from 0 - 255 */
void render_hud_health_left(SDL_Surface *dst, SDL_Surface *src, float a, float b, SDL_Color *fg, SDL_Color *bg, TTF_Font *font, char *msg)
{
	#define L_HUD_HEALTH_TOPLEFT_X 39
	#define L_HUD_HEALTH_TOPLEFT_Y 24
	#define L_HUD_HEALTH_TOPLEFT_LN 75
	
	SDL_PixelFormat *pw_fmt;
	SDL_Rect pos0;
	int pw_bpp, pw_pitch, tmpln, i, j;
	uint32_t pw_ucol;
	uint8_t *pw_pixel;
	
	pos0.x=pos0.y=0;
	
	
	pw_fmt = dst->format;
	pw_bpp = pw_fmt->BytesPerPixel;
	pw_pitch = dst->pitch;
	
	
	if (pw_bpp!=4)
		return;
		
	SDL_BlitSurface(src,0 , dst, &pos0);
	
	
	#define PW_PIXEL_SETCOL(r,g,b)  \
		pw_ucol = SDL_MapRGB(pw_fmt, r,g,b);
		
	#define PW_PIXEL_SET(x,y)  \
		{pw_pixel = (uint8_t*) dst->pixels;  \
		pw_pixel += ((y) * pw_pitch) + ((x) * pw_bpp);  \
		*((uint32_t*)pw_pixel) = pw_ucol;}
	
	PW_PIXEL_SETCOL(201,228,246);
	
	tmpln = a * L_HUD_HEALTH_TOPLEFT_LN;
	
	
	i=L_HUD_HEALTH_TOPLEFT_X;
	
	
	while (i < L_HUD_HEALTH_TOPLEFT_X + tmpln + 2)
	{
		j=L_HUD_HEALTH_TOPLEFT_Y;
		if (i<L_HUD_HEALTH_TOPLEFT_X+75)
			PW_PIXEL_SET(i, j);
		j++;
		if (i<L_HUD_HEALTH_TOPLEFT_X+76)
			PW_PIXEL_SET(i, j);
		j++;
		PW_PIXEL_SET(i, j);
		
		i++;
		
	}
	
	
	/* render text */
	render_text(dst, fg, bg, font, msg, 50,7);
	
	
}

/* applies dxyz to xyz vars in actor, while clipping
 * against other actors and a tilemap.
 */
void actor_apply_delta_doclip( actor *a, actor **all, tilemap *tm)
{
	a->x += a->dx;
	a->y += a->dy;
	a->z += a->dz;
	
	a->dx = a->dy = a->dz = 0;
	
}

/* applies dxyz to xyz vars in actor, while clipping
 * against other actors and a tilemap.
 */
void actor_apply_delta_noclip( actor *a)
{
	a->x += a->dx;
	a->y += a->dy;
	a->z += a->dz;
	
	a->dx = a->dy = a->dz = 0;
	
}


int main(void)
{
	/* initialize things: */
	
	int i, j, k, actor_cnt=0,
		k_w=0, k_a=0, k_s=0,
		k_d=0,k_space=0, tick_shad = -1, run = 1,
		game_mode, cntr_a, cntr_b, cntr_c, game_mode_first_loop;

	/* character jump table.  this holds the "arc" for jumping.
	 * it's only half of it, and there's code below that finishes
	 * the job. */
	int ch_jump_ani_table[64] = {3,3,3,2,2,2,1,1,1,0,1,0,1,0,1,0,1,-999};
	
	SDL_Event e;
	SDL_Surface  *sprt_shadow, *main_display, *tmpd, *sprt_logo, *sprt_titlescreen, *gfx_hud_health_l;
	SDL_Window  *win;
	SDL_Rect 	pos;
	SDL_Rect 	pos0;
	sprite apple,  sprt_shad;
	render_sprite_head rstest;
	tilemap * tmaptest;
	actor *actors, *key_wasd_cont;
	cam testcam;
	sprite *ch0_sprites_walk[4], *ch0_sprites_stand[4], *sprt_jar;
	float ch0_health=1.0;
	ani *test_ani;
	
	/* font stuff */
	SDL_Color font_default = {255,255,255,255};
	SDL_Color font_outline = {32,32,128,255};
	TTF_Font *font = 0;

	/* declare stuff for fader mechanism */
	int fader_md=0, fader_cnt=0, fade_speed=(10);
#define SET_FADE_IN() {fader_md=1;fader_cnt=255;}
#define SET_FADE_OUT() {fader_md=2;fader_cnt=0;}
#define SET_FADE_OFF() {fader_md=0;fader_cnt=0;}
	
	pos0.x = pos0.y = 0;
	
	tmaptest = new_tilemap();
	
	actors = (actor*) malloc(sizeof(actor) * 64 );
	key_wasd_cont = &(actors[0]);
	
	reset_cam(&testcam);
	
	rstest.next = 0;
	clear_render_sprites(&rstest);
	clear_tilemap(tmaptest);

	/* mirror the character jump table, because i'm too lazy */
	j=-1;
	for (i=0;i<64;i++)
	{
		if (j==-1 && ch_jump_ani_table[i] == -999)
			j=i-1;
		if (j>=0)
		{
			ch_jump_ani_table[i] = ch_jump_ani_table[j--] * -1;
			if (i+1 < 64)
				ch_jump_ani_table[i+1] = -999;
			if (j<0)
				break;
		}
	}
	
	
	tmaptest->tiles[0] = IMG_Load("w0_t0.png");
	tmaptest->tiles[1] = IMG_Load("w0_t1.png");
	tmaptest->tiles[2] = IMG_Load("w0_t2.png");
	tmaptest->tiles[3] = IMG_Load("w0_t3.png");
	
	tilemap_box_modify(tmaptest, 2,0,8,0,  2);
	tilemap_box_modify(tmaptest, 2,1,8,1,  3);
	tilemap_box_modify(tmaptest, 2,2,8,8,  1);
	tilemap_box_modify(tmaptest, 9,5,14,8,  1);

	sprt_logo = IMG_Load("logo.png");
	sprt_titlescreen = IMG_Load("titlescreen.png");
	
	gfx_hud_health_l = IMG_Load("hud_health_l.png");
	
	
	#define SPRITE_SET(a,c,d,e,f,g,h)  \
		a = (sprite *) malloc(sizeof (sprite));  \
		a->cx = c;  \
		a->cy = d;  \
		a->transp = e;  \
		a->frame_cnt = f;  \
		a->frame_cur = 0;  \
		a->sprt_arr = (SDL_Surface**) malloc( sizeof (SDL_Surface*) * f );  \
		a->intrv=g;  \
		a->loop=h;  \
		a->cnt0=0;
	
	#define SPRITE_LOAD_IMAGE(a,c,d)  \
		a->sprt_arr[c] = IMG_Load(d);
	
	
	
	
	/* add test actors */
	for (i=0;i<64;i++)
		reset_actor(&actors[i]);
	actor_cnt=64;
	
	actors[0].z=1;
	actors[0].x=50;
	actors[0].y=-50;
	actors[0].type=0; /*0=character, 1=other, 2=pickup-item*/
	
	/* jar */
	actors[1].z=1;
	actors[1].x=70;
	actors[1].y=-50;
	actors[1].type=1;
	
	/* jar */
	actors[1].z=1;
	actors[1].x=70;
	actors[1].y=-50;
	actors[1].type=1;
	
	testcam.target = &actors[0];
	
	
	
	/* load sprites */
	SPRITE_SET(sprt_jar, 8,14, 0, 1, 10,1);
	SPRITE_LOAD_IMAGE(sprt_jar,0,"jar.png");
	
	
	SPRITE_SET(ch0_sprites_walk[0], 16,28, 0, 2, 10,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[0],0,"l_0.png");
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[0],1,"l_1.png");
	
	SPRITE_SET(ch0_sprites_stand[0], 16,28, 0, 1, 10,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_stand[0],0,"l_0.png");
	
	SPRITE_SET(ch0_sprites_walk[1], 16,28, 0, 2, 10,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[1],0,"r_0.png");
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[1],1,"r_1.png");
	
	SPRITE_SET(ch0_sprites_stand[1], 16,28, 0, 1, 10,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_stand[1],0,"r_0.png");
	
	SPRITE_SET(ch0_sprites_walk[2], 16,28, 0, 2, 10,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[2],0,"u_0.png");
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[2],1,"u_1.png");
	
	SPRITE_SET(ch0_sprites_stand[2], 16,28, 0, 1, 20,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_stand[2],0,"u_0.png");
	
	SPRITE_SET(ch0_sprites_walk[3], 16,28, 0, 2, 10,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[3],0,"d_0.png");
	SPRITE_LOAD_IMAGE(ch0_sprites_walk[3],1,"d_1.png");
	
	SPRITE_SET(ch0_sprites_stand[3], 16,28, 0, 1, 20,1);
	SPRITE_LOAD_IMAGE(ch0_sprites_stand[3],0,"d_0.png");
	
	/* very ugly */
	actors[0].gfx_walk[0] = ch0_sprites_walk[0];
	actors[0].gfx_walk[1] = ch0_sprites_walk[1];
	actors[0].gfx_walk[2] = ch0_sprites_walk[2];
	actors[0].gfx_walk[3] = ch0_sprites_walk[3];
	actors[0].gfx_walk[4] = ch0_sprites_walk[0];
	actors[0].gfx_walk[5] = ch0_sprites_walk[1];
	actors[0].gfx_walk[6] = ch0_sprites_walk[2];
	actors[0].gfx_walk[7] = ch0_sprites_walk[3];
	actors[0].gfx_stand[0] = ch0_sprites_stand[0];
	actors[0].gfx_stand[1] = ch0_sprites_stand[1];
	actors[0].gfx_stand[2] = ch0_sprites_stand[2];
	actors[0].gfx_stand[3] = ch0_sprites_stand[3];
	actors[0].gfx_stand[4] = ch0_sprites_stand[0];
	actors[0].gfx_stand[5] = ch0_sprites_stand[1];
	actors[0].gfx_stand[6] = ch0_sprites_stand[2];
	actors[0].gfx_stand[7] = ch0_sprites_stand[3];
	
	actors[1].main = sprt_jar;
	
	
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		printf("SDL init failed...\n");
		return 1;
	}
	
	IMG_Init(IMG_INIT_PNG);
	
	/* create window and get surface : */
	
	win = SDL_CreateWindow
		(
			"win",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			SWIDTH, SHEIGHT,
			0
		);
	
	/* get window surface; create framebuffer */
	main_display = SDL_GetWindowSurface(win);
	tmpd = SDL_CreateRGBSurface(0, SWIDTH, SHEIGHT, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0xff);
	SDL_SetSurfaceBlendMode(tmpd, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceBlendMode(main_display, SDL_BLENDMODE_BLEND);
	
	
	/* create apple sprite */
	apple.cx=7;apple.cy=10;
	apple.transp=0;
	apple.frame_cnt=1;
	apple.frame_cur=0;
	apple.sprt_arr = (SDL_Surface**) malloc( sizeof (SDL_Surface*) );
	apple.sprt_arr[0] = IMG_Load("apple.png");;
	apple.intrv=0;
	apple.loop=0;
	
	/* create shadow sprite*/
	sprt_shad.cx=8;sprt_shad.cy=2;
	sprt_shad.transp=1;
	sprt_shad.frame_cnt=1;
	sprt_shad.frame_cur=0;
	sprt_shad.sprt_arr = (SDL_Surface**) malloc( sizeof (SDL_Surface*) );
	sprt_shad.sprt_arr[0] = IMG_Load("shad.png");
	sprt_shad.intrv=0;
	sprt_shad.loop=0;
	
	game_mode = MD_LOGO;
	game_mode_first_loop = 1;

	if(TTF_Init()==-1)
	{
		printf("TTF_Init: %s\n", TTF_GetError());
		return 2;
	}

	font=TTF_OpenFont("gohufont-11.ttf", 11);
	if(!font)
	{
		printf("TTF_OpenFont: %s\n", TTF_GetError());
		exit(3);
	}

	SET_FADE_IN();
	cntr_a=cntr_b=cntr_c=0;
	
	test_ani = new_ani("n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;l jar.png;p 0 50 50;n;\
u 0;p 0 50 50;n;n;n;\
u 0;p 0 60 50;n;n;n;\
u 0;p 0 70 50;n;n;n;\
u 0;p 0 80 50;n;n;n;\
u 0;p 0 90 50;n;n;n;\
u 0;p 0 100 50;n;n;n;");


	/* run things: */

	while (run)
	{
		while (SDL_PollEvent(&e))
			switch(e.type)
			{
			case SDL_QUIT:
				run=0;
				break;
			case SDL_KEYDOWN:
				switch(e.key.keysym.sym)
				{
				case SDLK_UP:k_w=1;break;
				case SDLK_LEFT:k_a=1;break;
				case SDLK_DOWN:k_s=1;break;
				case SDLK_RIGHT:k_d=1;break;
				case SDLK_SPACE:k_space=1;break;
				case SDLK_ESCAPE:run=0;break;
				}
				break;
			case SDL_KEYUP:
				switch(e.key.keysym.sym)
				{
				case SDLK_UP:k_w=0;break;
				case SDLK_LEFT:k_a=0;break;
				case SDLK_DOWN:k_s=0;break;
				case SDLK_RIGHT:k_d=0;break;
				case SDLK_SPACE:k_space=0;break;
				}
				break;
			}
		
		switch(game_mode)
		{
		case MD_LOGO:
			SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0, 0, 0, 255 ) );
			pos.x = (SWIDTH/2) - (sprt_logo->w / 2);
			pos.y = (SHEIGHT/2) - (sprt_logo->h / 2);
			SDL_BlitSurface(sprt_logo,0 , tmpd, &pos);

			if (k_space)
			{
				SET_FADE_OFF()
				game_mode = MD_DEATHMATCH;
			}
			
			cntr_a++;

			if (cntr_a == (FPS*2))
				SET_FADE_OUT();

			if (cntr_a > (FPS*2) && fader_cnt == 255)
			{
				game_mode_first_loop=1;
				game_mode = MD_MENU;
			}
			break;
		case MD_MENU:
			if (game_mode_first_loop)
			{
				SET_FADE_IN();
				game_mode_first_loop=0;
				
				test_ani = new_ani("\
l titlebg.png;l brawllords.png;l bl_smoke_left.png;l bl_smoke_right.png;p 0 0 0;n;\
n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;\
n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;\
u 1;p 1 71 0;n;\
u 1;p 1 71 5;n;\
u 1;p 1 71 10;n;\
u 1;p 1 71 15;n;\
u 1;p 1 71 20;n;\
u 1;p 1 71 25;n;\
u 1;p 1 71 30;n;\
u 1;p 1 71 35;n;\
u 1;p 1 71 40;n;\
u 1;p 1 71 45;n;\
u 1;p 1 71 50;n;\
u 1;p 1 71 55;n;\
u 1;p 1 71 60;n;\
u 1;p 1 71 65;n;\
u 1;p 1 71 70;n;\
u 1;p 1 71 75;n;\
u 1;p 1 71 80;n;\
u 1;p 1 71 85;n;\
u 1;p 1 71 90;n;\
u 1;p 1 71 95;n;\
u 1;p 1 71 100;n;\
u 1;p 1 71 105;n;\
u 1;p 1 71 110;n;\
u 1;p 1 71 115;n;\
u 1;p 1 71 120;n;\
u 1;p 1 71 126;n;\
u 1;p 1 71 128;n;\
u 1;p 1 71 131;n;\
u 1;p 1 71 141;n;\
u 1;p 1 71 151;n;\
u 2;p 2 52 165;u 3;p 3 240 165;u 0;p 0 0 2;u 1;p 1 71 153;;n;;u 0;p 0 0 0;u 1;p 1 71 151;n;;u 0;p 0 0 2;u 1;p 1 71 153;n;\
u 2;p 2 42 165;u 3;p 3 250 165;;u 0;p 0 0 0;u 1;p 1 71 151;n;;u 0;p 0 0 2;u 1;p 1 71 153;n;;u 0;p 0 0 0;u 1;p 1 71 151;n;\
u 2;p 2 32 165;u 3;p 3 260 165;;u 0;p 0 0 2;u 1;p 1 71 153;n;;u 0;p 0 0 0;u 1;p 1 71 151;n;;u 0;p 0 0 2;u 1;p 1 71 153;n;\
u 2;u 3;n");
				
				cntr_c=0;
				
				ani_frame(test_ani, tmpd);
			}
			
			
			if (cntr_c==0 && fader_cnt)
				cntr_c=1;
			
			if (cntr_c==1)
				ani_frame(test_ani, tmpd);
			
			if (cntr_c==1 && k_space)
			{
				SET_FADE_OUT();
				
				cntr_c++;
			}
				
			
			if (cntr_c==2 && fader_cnt==255)
			{
				game_mode_first_loop=1;
				game_mode = MD_DEATHMATCH;
			}
			
			break;
		case MD_DEATHMATCH:
			
			if (game_mode_first_loop)
			{
				SET_FADE_IN();
				cntr_c=FPS;
				game_mode_first_loop=0;
			}
			
			if (cntr_c > 0)
				cntr_c--;
			
			for (i=0;i < actor_cnt;i++)
			{
				actors[i].prev_dir = actors[i].dir;
				actors[i].md = CH_STAND;
			
				
				/* processing for wasd-controlled actor */
				if (&actors[i] == key_wasd_cont && cntr_c <= 0)
				{
					if (k_w)
					{
						key_wasd_cont->md = CH_WALK;
						key_wasd_cont->dy = 1;
						key_wasd_cont->dir = DIR_UP;
					}
					if (k_a)
					{
						key_wasd_cont->md = CH_WALK;
						key_wasd_cont->dx = -1;
						key_wasd_cont->dir = DIR_LEFT;
					}
					if (k_s)
					{
						key_wasd_cont->dy = -1;
						key_wasd_cont->md = CH_WALK;
						key_wasd_cont->dir = DIR_DOWN;
					}
					if (k_d)
					{
						key_wasd_cont->dx = 1;
						key_wasd_cont->md = CH_WALK;
						key_wasd_cont->dir = DIR_RIGHT;
					}
					
					if ((k_w && k_s) || (k_a && k_d))
					{
						key_wasd_cont->dx = key_wasd_cont->dy = 0;
						key_wasd_cont->md = CH_STAND;
					}
						
					

					if (k_space && key_wasd_cont->jump==0)
						key_wasd_cont->jump=1;
					
					/* if space is held and character has already jumped, do not allow jump. */
					if (!k_space && key_wasd_cont->jump==-1)
						key_wasd_cont->jump=0;

					if (key_wasd_cont->jump > 0)
					{
						/* jump: change z depending on values in ch_jump_ani_table */
						if (ch_jump_ani_table[(key_wasd_cont->jump)-1] != -999)
							key_wasd_cont->dz =
								ch_jump_ani_table[(key_wasd_cont->jump++)-1];
						else
							key_wasd_cont->jump=-1;

					}
					
					
				}
				else /* processing for other actors */
				{
					
				}
				
				actor_apply_delta_doclip(&actors[i], &actors, tmaptest);
			}
		
			/* fill example render sprites */
			clear_render_sprites(&rstest);
		
			/* for each actor */
			for (i=0;i < actor_cnt;i++)
			{
				sprite *use_sprite = actors[i].main;
				
				if (actors[i].type==0)
					switch(actors[i].md)
					{
					case CH_WALK:
						use_sprite = actors[i].gfx_walk[actors[i].dir];
						break;
					case CH_STAND:
						use_sprite = actors[i].gfx_stand[actors[i].dir];
						break;
					case CH_JUMP:
						use_sprite = actors[i].gfx_jump[actors[i].dir];
						break;
					}
			
				if (use_sprite)
					add_sprite_auto_shadow(
						&rstest,
						use_sprite,
						(actors[i].type!=1) ? &sprt_shad : 0,
						&testcam,
						actors[i].x,
						actors[i].y,
						actors[i].z
						);
			
			}
		
			/* start render process: */
		
			/* clear framebuffers */
			SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0,0,0,255 ) );
		
			/* render tile map */
			render_tilemap(tmpd, tmaptest, &testcam);
		
			/* render sprites from render sprite list */
			render_rsprite_list(tmpd, &rstest, tick_shad);
			
			render_hud_health_left(tmpd, gfx_hud_health_l, ch0_health, 1.0, &font_default, &font_outline, font, "Test Character");
			
		
			/* tick flicker shadow effect */
			tick_shad *= -1;
			
			
			break;
		}
		
		switch (fader_md)
		{
		case FADE_IN:
			fader_cnt -= fade_speed;
			if (fader_cnt<=0)
			{
				fader_md=0;
				fader_cnt=0;
			}
			break;
		case FADE_OUT:
			fader_cnt += fade_speed;
			if (fader_cnt>=255)
			{
				fader_md=0;
				fader_cnt=255;
			}
			break;
		}

		
		
		/* copy framebuffer to surface of window.  update window. */
		
		SDL_BlitSurface(tmpd,0 , main_display, &pos0);

		SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0, 0, 0, fader_cnt ) );

		SDL_BlitSurface(tmpd,0 , main_display, &pos0);

		SDL_UpdateWindowSurface(win);
		
		tick_frame(FPS);
		
	}
	
	/* terminate things: */
	
	SDL_DestroyWindow(win);
	IMG_Quit();
	SDL_Quit();
	
	return 0;
}
