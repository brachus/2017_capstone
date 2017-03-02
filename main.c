
#include <stdio.h>
#include <string.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "jsmn/jsmn.h"



#ifdef LINUX
#include <unistd.h>
#else
#include <windows.h>
#endif


#ifdef _MSC_VER
  #pragma warning(disable: 4996 4026)
#endif

#define SWIDTH 320
#define SHEIGHT 240

#define SMULT (flag_screen_mult)

#define TILESIZE 16

#define TILEMAPSIZE 1024
#define TILEMAPNTILES 32

#define FPS 60

/* definitions for quick'n'dirty parser for setting up structs */
#define QUICK_PARSE_DECLARATIONS \
	int buf_tmp, arg_idx, is_int[8], nargs, ln, i; \
	char buf[8][256];

#define QUICK_PARSE_BEFORE_LINE \
	buf[0][0]=buf[1][0]=buf[2][0]=buf[3][0] = \
		buf[4][0]=buf[5][0]=buf[6][0]=buf[7][0] = '\0'; \
	is_int[0]=is_int[1]=is_int[2]=is_int[3] =  \
		is_int[4]=is_int[5]=is_int[6]=is_int[7] = 1; \
	buf_tmp=arg_idx=0; \
	ln=strlen(script); \
	nargs=i=0; \
	while (i<ln) { \
		if (script[i] == ' ' || script[i] == '\n' || script[i] == '\t') \
		{ \
			if (buf_tmp!=0) \
			{ \
				buf_tmp=0; \
				if (arg_idx<7) \
					arg_idx++; \
			} \
		} \
		else if (script[i] == ';') \
		{

#define QUICK_PARSE_AFTER_LINE \
		is_int[0]=is_int[1]=is_int[2]=is_int[3]= \
			is_int[4]=is_int[5]=is_int[6]=is_int[7]=1; \
		arg_idx=buf_tmp=0; \
	} \
	else \
	{ \
		if (!((script[i] == '-' && buf_tmp==0) || (script[i]>='0' && script[i]<='9')) ) \
			is_int[arg_idx] = 0; \
		buf[arg_idx][buf_tmp++] = script[i]; \
		if (buf_tmp>=256) \
			buf_tmp--; \
		buf[arg_idx][buf_tmp] = '\0'; \
		nargs = arg_idx + 1; \
	}i++;}


/* to simplify surface manipulations. */
#define PW_DECLARE \
	SDL_PixelFormat *pw_fmt; \
	int pw_bpp, pw_pitch; \
	uint32_t pw_ucol; \
	uint8_t *pw_pixel;

#define PW_INIT(dst) \
	pw_fmt = dst->format; \
	pw_bpp = pw_fmt->BytesPerPixel; \
	pw_pitch = dst->pitch; \
	if (pw_bpp!=4) \
		return;

#define PW_PIXEL_SETCOL(r,g,b)  \
	pw_ucol = SDL_MapRGB(pw_fmt, r,g,b);
	
#define PW_PIXEL_SET(x,y,dst)  \
	{pw_pixel = (uint8_t*) dst->pixels;  \
	pw_pixel += ((y) * pw_pitch) + ((x) * pw_bpp);  \
	*((uint32_t*)pw_pixel) = pw_ucol;}

#define PW_PIXEL_MULPIXEL(x,y,r,g,b,a,dst)  \
	{pw_pixel = (uint8_t*) dst->pixels;  \
	pw_pixel += ((y) * pw_pitch) + ((x) * pw_bpp);  \
	SDL_GetRGBA(*((uint32_t*)pw_pixel), pw_fmt, (Uint8*) &r, (Uint8 *)&g, (Uint8 *)&b, (Uint8 *)&a);  \
	if (a>0) \
		*((uint32_t*)pw_pixel) = SDL_MapRGBA(pw_fmt, r*mult[0]>>8,g*mult[1]>>8,b*mult[2]>>8,a); \
	}

/* coordinates are global; y0 > y1 */
int check_bbox_intersect(
	int ax0, int ay0, int ax1, int ay1,
	int bx0, int by0, int bx1, int by1)
{
	if (
		((ax0 < bx1 && ax0 > bx0) || (ax1 < bx1 && ax1 > bx0) || (ax0 < bx0 && ax1 > bx1)) &&
		((ay0 < by0 && ay0 > by1) || (ay1 < by0 && ay1 > by1) || (ay0 > by0 && ay1 < by1))
			)
		return 1;
	return 0;
}
	

int flag_screen_mult = 1;

enum
{
	MD_CTLR_CHECK,
	MD_SET_CONTROLS,
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

enum
{
	ANI_LOAD,
	ANI_POST,
	ANI_UNPOST,
	ANI_NEXT
};

/* enum for each type of chara.*/
enum
{
	CHARA_CH0,
	CHARA_JAR,
	CHARA_FOOD_PICKUP_BOWL,
	CHARA_FOOD_PICKUP_MEAT,
};

enum
{
	CH0_MD_STAND_U,
	CH0_MD_STAND_D,
	CH0_MD_STAND_L,
	CH0_MD_STAND_R,
				
	CH0_MD_WALK_U,
	CH0_MD_WALK_D,
	CH0_MD_WALK_L,
	CH0_MD_WALK_R,
				
	CH0_MD_ATTK_U,
	CH0_MD_ATTK_D,
	CH0_MD_ATTK_L,
	CH0_MD_ATTK_R
};

enum
{
	CH_TYPE_PICKUP,
	CH_TYPE_INTERACT
}

enum
{
	DEBUG_SHOW_ACTION_FRAMES,
	DEBUG_COUNT
};

#include "structs.h"

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

sprite *new_sprite(char *script)
{
	int j;
	
	QUICK_PARSE_DECLARATIONS
	
	sprite *n=(sprite *) malloc(sizeof(sprite));
	
	n->cx=0;
	n->cy=0;
	n->transp=0;
	n->frames=0;
	n->sprt_arr=0;
	n->intrv=0;
	n->loop=0;
	n->name[0]='\0';
	n->attk_frame_start=0;
	n->attk_frame.cntr=0;
	n->attk_frame.id=0;
	n->attk_frame.type=0;
	n->attk_frame.next=0;
	n->attk_frame.pos.x=0;
	n->attk_frame.pos.y=0;
	n->attk_frame.pos.z=0;
	n->attk_frame.target.x=0;
	n->attk_frame.target.y=0;
	n->attk_frame.target.z=0;
	n->attk_frame.w=0;
	n->attk_frame.h=0;
	n->attk_frame.z=0;
	n->dfnd_frame_start;
	n->dfnd_frame.cntr=0;
	n->dfnd_frame.id=0;
	n->dfnd_frame.type=0;
	n->dfnd_frame.next=0;
	n->dfnd_frame.pos.x=0;
	n->dfnd_frame.pos.y=0;
	n->dfnd_frame.pos.z=0;
	n->dfnd_frame.target.x=0;
	n->dfnd_frame.target.y=0;
	n->dfnd_frame.target.z=0;
	n->dfnd_frame.w=0;
	n->dfnd_frame.h=0;
	n->dfnd_frame.z=0;
	n->drift=0; /* sprite may animate parent character*/
	n->next=0;
	
	
	QUICK_PARSE_BEFORE_LINE
	
		if (	!strcmp(buf[0], "img") &&
				n->frames!=0 &&
				nargs==3 &&
				is_int[0]==0 && 
				is_int[1] &&
				is_int[2]==0) /* load image for frame: img <int frame> <str fn>;*/
		{
			if (atoi(buf[1]) < n->frames)
			{
				n->sprt_arr[atoi(buf[1])] = IMG_Load(buf[2]);
				
				if (!n->sprt_arr[atoi(buf[1])])
					printf("error: \"%s\" failed to load.\n", buf[2]);
			}
		}
		else if (!strcmp(buf[0], "drift") &&
				n->frames!=0 &&
				is_int[0]==0 &&
				is_int[1] &&
				is_int[2] &&
				is_int[3] &&
				is_int[4] &&
				nargs==5) /* set drift for frame: img <int frame> <int x> <int y> <int z>;*/
		{
			if (atoi(buf[1]) < (n->frames * n->intrv))
			{
				n->drift[atoi(buf[1])].x = atoi(buf[2]);
				n->drift[atoi(buf[1])].y = atoi(buf[3]);
				n->drift[atoi(buf[1])].z = atoi(buf[4]);
			}
		}
			/* polymorphism: second arg may be string like "all", which would set all items in drift array. */
		else if (!strcmp(buf[0], "drift") &&
				n->frames!=0 &&
				is_int[0]==0 &&
				is_int[1]==0 &&
				is_int[2] &&
				is_int[3] &&
				is_int[4] &&
				nargs==5) /* set drift for frame: img <int frame> <int x> <int y> <int z>;*/
		{
			if ( !strcmp(buf[1], "all") )
			{
				for (j=0;j<n->frames * n->intrv;j++)
				{
					n->drift[j].x = atoi(buf[2]);
					n->drift[j].y = atoi(buf[3]);
					n->drift[j].z = atoi(buf[4]);
				}
			}
		}
		else if (!strcmp(buf[0], "cxy") &&
				nargs==3 &&
				is_int[0]==0 &&
				is_int[1] &&
				is_int[2])
		{
			n->cx=atoi(buf[1]);
			n->cy=atoi(buf[2]);
		}
		else if (!strcmp(buf[0], "frames") && /* frames <frames> <intrv> */
				nargs==3 &&
				is_int[0]==0 &&
				is_int[1] &&
				is_int[2]  )
		{
			n->frames=atoi(buf[1]);
			n->intrv=atoi(buf[2]);
			
			n->sprt_arr = (SDL_Surface**) malloc(sizeof(SDL_Surface*) * n->frames);
			for (j=0;j<n->frames;j++)
				n->sprt_arr[j]=0;		
			
			n->drift = (xyz*) malloc(sizeof(xyz) * n->frames * n->intrv);
			/* NOTE: drift, attack frame, and defend frames use *actual
			 * frames*, meaning all the ticks in between 'frames' specified
			 * by intrv
			 */
			 
			for (j=0;j<n->frames * n->intrv;j++)
				n->drift[j].x=n->drift[j].y=n->drift[j].z=0;
			
		}
			
		else if (!strcmp(buf[0], "loop") && nargs==2 && is_int[0]==0 && is_int[1])
			n->loop=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "transp") && nargs==2 && is_int[0]==0 && is_int[1])
			n->transp=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "name") && nargs==2 && is_int[0]==0 && is_int[1]==0 && strlen(buf[1]) < 31)
			strcpy(n->name, buf[1]);
		
		else if (!strcmp(buf[0], "intrv") && nargs==2 && is_int[0]==0 && is_int[1])
			n->intrv=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "attk_frame_start") && nargs==2 && is_int[0]==0 && is_int[1])
			n->attk_frame_start=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "attk_frame_len") && nargs==2 && is_int[0]==0 && is_int[1]) /* set attack frame counter */
			n->attk_frame.cntr=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "attk_frame_bbox") &&
				nargs==6 &&
				is_int[0]==0 &&
				is_int[1] &&
				is_int[2] &&
				is_int[3] &&
				is_int[4] &&
				is_int[5]) /*attk_frame_bbox <x> <y>   <h> <w> <z>;*/
		{
			n->attk_frame.pos.x=atoi(buf[1]);
			n->attk_frame.pos.y=atoi(buf[2]);
			n->attk_frame.pos.z=1;
						
			n->attk_frame.h=atoi(buf[3]);
			n->attk_frame.w=atoi(buf[4]);
			n->attk_frame.z=atoi(buf[5]);
		}
		else if (!strcmp(buf[0], "attk_frame_target") &&
				nargs==4 &&
				is_int[1] &&
				is_int[2] &&
				is_int[3]) /*attk_frame_bbox <x> <y> <z>;*/
		{
			n->attk_frame.target.x=atoi(buf[1]);
			n->attk_frame.target.y=atoi(buf[2]);
			n->attk_frame.target.z=atoi(buf[3]);
		}
			
		else if (!strcmp(buf[0], "dfnd_frame_start") &&
				nargs==2 &&
				is_int[0]==0 && 
				is_int[1])
			n->dfnd_frame_start=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "dfnd_frame_len") &&
				nargs==2 &&
				is_int[0]==0 &&
				is_int[1])
			n->dfnd_frame.cntr=atoi(buf[1]);
			
		else if (!strcmp(buf[0], "dfnd_frame_bbox") &&
				nargs==4 &&
				is_int[0]==0 &&
				is_int[1] &&
				is_int[2] &&
				is_int[3] &&
				is_int[4] &&
				is_int[5]) /* dfnd_frame_bbox <h> <w> <z>;*/
		{
			
			n->dfnd_frame.pos.x=atoi(buf[1]);
			n->dfnd_frame.pos.y=atoi(buf[2]);
			n->dfnd_frame.pos.z=1;
			
			n->dfnd_frame.h=atoi(buf[3]);
			n->dfnd_frame.w=atoi(buf[4]);
			n->dfnd_frame.z=atoi(buf[5]);
		}

	QUICK_PARSE_AFTER_LINE
	
	return n;
}

void sprite_add(world *in_world, char *sprt)
{	
	sprite *a = in_world->sprites;
	
	if (!a)
	{
		in_world->sprites = new_sprite(sprt);
		return;
	}
	
	while(a)
	{
		if (!(a->next))
		{
			a->next = new_sprite(sprt);
			break;
		}
		a=a->next;
	}
}

sprite *get_sprite(world *in_world, char *name)
{
	sprite *a = in_world->sprites;
	
	while (a)
	{
		if (!strcmp(a->name, name))
			break;
		a=a->next;
	}
	
	return a;
}

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
		if (line[line_nxt] == ' ' || line[line_nxt] == ';' || line[line_nxt] == '\t')
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
			if (!((line[line_nxt] >= '0' && line[line_nxt] <= '9') ||  (line[line_nxt] == '-' && line_nxt==0)))
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

ani *new_ani(char *script)
{
	int i, ln;
	ani *n;
	
	ln=strlen(script);
		
	n = (ani*) malloc(sizeof(ani));
	
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
	
	n->done=0;
	
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
				{
					in->gfx_dat[(in->gfx_cnt)++] = IMG_Load(in->cmd_cur->fn);

					if (!in->gfx_dat[(in->gfx_cnt)-1])
						exit(1);
					else
						printf("\"%s\" load ok!\n", in->cmd_cur->fn);
				}
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
	
	if (!in->cmd_cur)
		in->done=1;
	
	for (i=0;i<64;i++)
	{
		if (in->obj[i].idx >= 0 && in->gfx_dat[in->obj[i].idx])
		{
			
			tpos.x=in->obj[i].x;
			tpos.y=in->obj[i].y;
			if (!(in->gfx_dat[in->obj[i].idx]))
					printf("blit failed\n");
			else
					SDL_BlitSurface(in->gfx_dat[in->obj[i].idx],0 , dst, &tpos);
		}
	}
	
}

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
	in->type=1;
	in->jump=0;

	in->active=0;
	
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

tilemap * new_tilemap(int w, int h, int ntiles)
{
	int i;
	tilemap *n;
	
	n = (tilemap*) malloc(sizeof(tilemap));
	
	n->x = 0;
	n->y = 0;
	n->w = w;
	n->h = h;
	
	n->ntiles = ntiles;
	
	n->tsize = 16;
	
	n->arr = (int**) malloc(sizeof(int *) * n->h );
	
	for (i=0;i<n->w;i++)
		n->arr[i] = (int*) malloc(sizeof(int) * n->w);
	
	n->tiles = (SDL_Surface **) malloc(sizeof(SDL_Surface *) * n->ntiles);
	for (i=0;i<n->ntiles;i++)
		n->tiles[i] = 0;
	
	n->tile_block = (int*) malloc(sizeof(int) * n->ntiles);
	for (i=0;i<n->ntiles;i++)
		n->tile_block[i] = 0;
	
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



chara_template *new_chara_template(world *in_world, char *script)
{
	QUICK_PARSE_DECLARATIONS
	
	chara_template *n = (chara_template *) malloc(sizeof(chara_template));
	
	n->gfx=0;
	n->gfx_cnt=0;
	
	n->max_hp=0;
	n->max_mp=0;
	
	n->attack=0;
	n->defend=0;
	
	n->bbox_w=0;
	n->bbox_h=0;
	n->bbox_z=0;
	
	n->type=0;
	
	n->pickup = 0;
	
	n->next=0;
	
	QUICK_PARSE_BEFORE_LINE
		if (	!strcmp(buf[0], "max_hp") &&nargs==2 &&
				is_int[0]==0 && is_int[1])
			n->max_hp = atoi(buf[1]);
		else if (	!strcmp(buf[0], "max_mp") &&nargs==2 &&
				is_int[0]==0 && is_int[1])
			n->max_mp = atoi(buf[1]);
		else if (	!strcmp(buf[0], "attack") &&nargs==2 &&
				is_int[0]==0 && is_int[1])
			n->attack = atoi(buf[1]);
		else if (	!strcmp(buf[0], "defend") &&nargs==2 &&
				is_int[0]==0 && is_int[1])
			n->defend = atoi(buf[1]);
		else if (	!strcmp(buf[0], "bbox") &&nargs==4 &&
				is_int[0]==0 && is_int[1] && is_int[2] && is_int[3])
		{
			n->bbox_h = atoi(buf[1]);
			n->bbox_w = atoi(buf[2]);
			n->bbox_z = atoi(buf[3]);
		}
		else if (	!strcmp(buf[0], "type") && nargs==2 &&
				is_int[0]==0 && is_int[1])
			n->type = atoi(buf[1]);
		else if (	!strcmp(buf[0], "gfx_cnt") &&nargs==2 &&
				is_int[0]==0 && is_int[1])
		{
			n->gfx_cnt = atoi(buf[1]);
			
			n->gfx = (sprite**) malloc(sizeof(sprite*)*n->gfx_cnt);
		}
			
		else if (	!strcmp(buf[0], "gfx") && nargs==3 &&
				is_int[0]==0 && is_int[1] && is_int[2]==0 && n->gfx_cnt>0)
			n->gfx[atoi(buf[1])] = get_sprite(in_world, buf[2]);
		
		else if (!strcmp(buf[0], "name") &&
				nargs==2 &&
				is_int[0]==0 &&
				is_int[1]==0 &&
				strlen(buf[1]) < 31)
			strcpy(n->name, buf[1]);
		
		else if (!strcmp(buf[0], "pickup"))
			n->pickup = 1;
			
	QUICK_PARSE_AFTER_LINE
	
	return n;
}

sprite_active *new_sprite_active(sprite *a)
{
	sprite_active *n = (sprite_active*) malloc(sizeof(sprite_active));
	
	n->base = a;
	n->active=1;
	n->done=0;
	n->cntr=0;
	n->cur=0;
	
	return n;
};

chara_active *new_chara_active(chara_template *a)
{
	static int id_step = 0;
	int i;
	
	chara_active *n = (chara_active*) malloc(sizeof(chara_active));
	
	n->algnmt.earth=n->algnmt.fire=n->algnmt.wind=
		n->algnmt.water=n->algnmt.light=n->algnmt.dark=0;
	
	n->base = a;
	
	n->gfx = (sprite_active**) malloc(sizeof(sprite_active*) * a->gfx_cnt);
	
	for (i=0;i<a->gfx_cnt;i++)
		n->gfx[i] = new_sprite_active(a->gfx[i]);
	
	n->md=0;
	n->md_prev=0;
	
	n->max_hp = a->max_hp;
	n->max_mp = a->max_mp;
	
	n->hp = n->max_hp;
	n->mp = n->max_mp;
	
	n->attack = a->attack;
	n->defend = a->defend;
	
	n->lvl = 1;
	
	n->pos.x=n->pos.y=n->pos.z=0;
	n->dpos.x=n->dpos.y=n->dpos.z=0;
	
	n->invisi_cntr=0;
	n->clips=1;
	
	n->effect_cntr=0; /* for glint effect */
	n->effect_type=0;
	
	n->in_world=1;
	
	n->in_room=0;
	
	n->active=1; /* this may replace in_world */
	
	n->id = id_step++;
	
	for (i=0;i<8;i++)
		n->info[i]=0;
	
	for (i=0;i<8;i++)
		n->cntr[i]=0;
	
	for (i=0;i<32;i++)
		n->af_hit[i]=0;
	
	
	n->drift_dec=0;
	n->drift_intrv=0; /* 0 = decrement drift by drift_dec every frame. */
	n->drift_state=0; /* 0 = use drift dec. */
	n->drift.x = 0;
	n->drift.y = 0;
	n->drift.z = 0;
	
	
	
	n->next=0;
	
	return n;
}

void chara_active_apply_drift(chara_active *a)
{
	a->dpos.x += a->drift.x;
	a->dpos.y += a->drift.y;
	a->dpos.z += a->drift.z;
	
	#define DEC_DRIFT(b) \
		if (b < 0) \
		{ \
			b += a->drift_dec; \
			b = (b > 0) ? 0 : b; \
		} \
		else if (b > 0) \
		{ \
			b -= a->drift_dec; \
			b = (b < 0) ? 0 : b; \
		}
	
	if (!a->drift_state)
	{
		DEC_DRIFT(a->drift.x);
		DEC_DRIFT(a->drift.y);
		DEC_DRIFT(a->drift.z);
		
		a->drift_state = a->drift_intrv;
	}
	else
		a->drift_state--;
}

void step_sprite_active(sprite_active *a)
{
	sprite * sprt = a->base;
	
	if (++a->cntr >= sprt->intrv)
	{
		a->cntr=0;
		
		a->cur++;
		
		if (a->cur >= sprt->frames)
		{
			if (sprt->loop)
				a->cur=0;
			else
				a->cur=sprt->frames - 1;
		}
	}
}

void add_sprite_auto_shadow(
	render_sprite_head *in,
	sprite_active *asprt,
	sprite *shad,
	cam *incam,
	int x,
	int y,
	int z  )
{
	int rx,ry,bry,sx,sy,cx,cy;
	
	sprite *sprt = asprt->base;
	
	if (z<0)
		return;
	
	/* adjust cam */
	if (!incam)
		cx=cy=0;
		
	else if (incam->target)
	{
		cx = incam->target->pos.x;
		cy = incam->target->pos.y;
	}
	else
	{
		cx = incam->x; /* take into account cam bounding box here ? */
		cy = incam->y;
	}
	
	rx = (SWIDTH/2) + x - cx;
	ry = (SHEIGHT/2) - y - z + cy;
	bry = (SHEIGHT/2) - y + cy;
	
	#define NEW_RENDER_SPRITE(a,b,c,d,e,f,g)  \
	{  \
		a = (render_sprite*) malloc(sizeof(render_sprite));  \
		a->transp = b->transp;  \
		a->sprt = b->sprt_arr[g->cur];  \
		a->x = d;  \
		a->y = e;  \
		a->by = f;  \
		a->cx = b->cx;  \
		a->cy = b->cy;  \
		a->next = c;  \
	}
	
	if (!(in->next))
		NEW_RENDER_SPRITE(in->next,sprt,0,rx,ry,bry,asprt)
	
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
				bry,asprt)
		}
		else
		{
			while (tmp)
			{
				if (tmp->by > ry)
				{
					if (!tmpprev)
						NEW_RENDER_SPRITE(in->next,sprt,tmp,rx,ry,bry,asprt)
					else
						NEW_RENDER_SPRITE(tmpprev->next,sprt,tmp,rx,ry,bry,asprt)
					break;
				}
				if (!tmp->next)
				{
					NEW_RENDER_SPRITE(tmp->next,sprt,0,rx,ry,bry,asprt)
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
			
			in->next = (render_sprite*) malloc(sizeof(render_sprite));
			in->next->transp = shad->transp;
			in->next->sprt = shad->sprt_arr[0];
			in->next->x = sx;
			in->next->y = sy;
			in->next->by = sy;
			in->next->cx = shad->cx;
			in->next->cy = shad->cy;
			in->next->next = tmp;
			
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


void apply_multrgb_to_surface(SDL_Surface *surf, int mult[3])
{
	PW_DECLARE;
	
	int i,j, rgba[4]={0,0,0,0};
		
	PW_INIT(surf);
	
	for (i=0;i<surf->w;i++)
		for (j=0;j<surf->h;j++)
			PW_PIXEL_MULPIXEL(i,j,rgba[0],rgba[1],rgba[2],rgba[3],surf);
	
}


void render_rsprite_list(SDL_Surface *surf, SDL_Surface *s_layer, render_sprite_head *sprh, int shad_tick)
{
	/* render sprites from render sprite list */
	SDL_Rect pos;
	render_sprite *tmp = sprh->next;
	
	int mult[3]={255,225,185}; /* multiplier. */
	
	/* eventually, take in a tile map with mult rgb values, and have each individual sprite
	 * be multiplied by this, instead of a global, static rgb multiply effect.
	 */
	
	while (tmp)
	{	
		pos.x = tmp->x - tmp->cx;
		pos.y = tmp->y - tmp->cy;
		SDL_BlitSurface(tmp->sprt, 0 , s_layer, &pos);
		tmp = tmp->next;
	}
	
	/* apply color multiplier to sprite layer before
	 * it is blitted to main layer
	 */
	
	apply_multrgb_to_surface(s_layer, mult);
	
	SDL_BlitSurface(s_layer, 0 , surf, 0);
}

void render_tilemap(SDL_Surface *surf, tilemap *intmap, cam *incam)
{
	int trow=0,tcol=0,camx,camy;
	SDL_Surface *ttmp;
	SDL_Rect pos;
	
	for (trow=0;trow<intmap->h;trow++)
		for (tcol=0;tcol<intmap->w;tcol++)
		{
			if (  intmap->arr[trow][tcol] >= 0 && 
				  intmap->arr[trow][tcol] < intmap->ntiles)
				ttmp = intmap->tiles[intmap->arr[trow][tcol]];
			else
				ttmp = 0;
				
			/* adjust cam */
			if (incam->target)
			{
				camx = incam->target->pos.x;
				camy = incam->target->pos.y;
			}
			else
			{
				camx = incam->x; /* take into account cam bounding box here ? */
				camy = incam->y;
			}
			
			pos.y = SHEIGHT/2 + (trow*intmap->tsize) + camy;
			pos.x = SWIDTH/2 + (tcol*intmap->tsize) - camx;
			
			if (pos.y+intmap->tsize<0 ||
				pos.y>SHEIGHT ||
				pos.x+intmap->tsize<0 ||
				pos.x>SWIDTH)
				continue;
			
			
			if (pos.y<SHEIGHT && (pos.y+intmap->tsize) >= 0 && 
				pos.x<SWIDTH && (pos.x+intmap->tsize) >= 0 && ttmp!=0)
				SDL_BlitSurface(ttmp,0 , surf, &pos);
		}
}

#define TEXT_CENTERED 1
#define TEXT_BACKING 2
#define TEXT_OUTLINE 4
#define TEXT_BOLD 8

void render_text(SDL_Surface *dst, SDL_Color *fg, SDL_Color *bg, TTF_Font *font, char *msg, int x, int y, int flags)
{
	SDL_Rect tmppos;
	SDL_Surface *fs;
	
	fs = TTF_RenderText_Solid(font, msg, *bg);
	
	if (flags & TEXT_CENTERED)
	{
		tmppos.x=x - fs->w/2;
		tmppos.y=y - fs->h/2;
	}
	else
	{
		tmppos.x=x;
		tmppos.y=y;
	}
	
	if (flags & TEXT_BACKING)
	{
		tmppos.w=fs->w + 2;
		tmppos.h=fs->h;
		
		tmppos.x-=2;
		
		SDL_FillRect( dst, &tmppos, SDL_MapRGBA( dst->format, bg->r, bg->g, bg->b, 128	 ) );
		
		tmppos.x+=2;
		tmppos.w=tmppos.h=0;
	}
	
	if (flags & TEXT_OUTLINE)
	{
		tmppos.x++;
		SDL_BlitSurface(fs,0 , dst, &tmppos);
		
		tmppos.x--;
	}
	
	SDL_FreeSurface(fs);
	
	
	
	fs = TTF_RenderText_Solid(font, msg, *fg);
	
	SDL_BlitSurface(fs,0 , dst, &tmppos);
	
	if (flags & TEXT_BOLD)
	{
		tmppos.x--;
		SDL_BlitSurface(fs,0 , dst, &tmppos);
	}
	
	SDL_FreeSurface(fs);
	
}


/* both a and b range from 0 - 255 */
void render_hud_health_left(SDL_Surface *dst, sprite *sprt_src, float a, float b, SDL_Color *fg, SDL_Color *bg, TTF_Font *font, char *msg)
{
	#define L_HUD_HEALTH_TOPLEFT_X 39
	#define L_HUD_HEALTH_TOPLEFT_Y 24
	#define L_HUD_HEALTH_TOPLEFT_LN 75
	
	PW_DECLARE;
	
	SDL_Surface *src;
	int tmpln, i, j;
	
	src = sprt_src->sprt_arr[0];
	
	PW_INIT(dst);
		
	SDL_BlitSurface(src,0 , dst, 0);
	
	
	PW_PIXEL_SETCOL(201,228,246);
	
	tmpln = (int) (a * L_HUD_HEALTH_TOPLEFT_LN);
	
	
	i=L_HUD_HEALTH_TOPLEFT_X;
	
	
	while (i < L_HUD_HEALTH_TOPLEFT_X + tmpln + 2)
	{
		j=L_HUD_HEALTH_TOPLEFT_Y;
		if (i<L_HUD_HEALTH_TOPLEFT_X+75)
			PW_PIXEL_SET(i, j,dst);
		j++;
		if (i<L_HUD_HEALTH_TOPLEFT_X+76)
			PW_PIXEL_SET(i, j,dst);
		j++;
		PW_PIXEL_SET(i, j,dst);
		i++;
		
	}
	
	/* render text */
	render_text(dst, fg, bg, font, msg, 42,8, TEXT_OUTLINE | TEXT_BOLD );
	
	
}

/* applies dpos to pos in active character
 */
void chara_active_apply_dpos_clip( chara_active *a, chara_active *all, tilemap *tm)
{
	int step[3] = {0,0,0}, acc_step[3] = {0,0,0}, acc_step_prev[3] = {0,0,0}, i;
	b_blocker *bbtmp;
	
	
	/*
	 * to do clipping, step pos 1px until at dpos,
	 * checking each step if bbox collides or not.
	 */
	
	/* how are we stepping ? */
	if (a->dpos.x > 0)   step[0]=1;
	if (a->dpos.x < 0)   step[0]=-1;
	if (a->dpos.y > 0)   step[1]=1;
	if (a->dpos.y < 0)   step[1]=-1;
	if (a->dpos.z > 0)   step[2]=1;
	if (a->dpos.z < 0)   step[2]=-1;
	
	
	
	/*printf("dpos %d %d %d \n",a->dpos.x,a->dpos.y,a->dpos.z);*/
	
	 
	/* step accumulators are already cleared. */
	
	for (i=0;i<32;i++)
	{
		
		if (acc_step[0] != a->dpos.x)
			acc_step[0] += step[0];
		if (acc_step[1] != a->dpos.y)
			acc_step[1] += step[1];
		if (acc_step[2] != a->dpos.z)
			acc_step[2] += step[2];
		
			
		/* for now, only test four corners of bounding box */
		
		
		{ 
			bbtmp = tm->b_block;
			
			/*printf("%d %d %d %d\n", a->pos.x + acc_step[0] - (a->base->bbox_w / 2),
						a->pos.y + acc_step[0] + (a->base->bbox_h / 2),
						a->pos.x + acc_step[0] + (a->base->bbox_w / 2),
						a->pos.y + acc_step[0] - (a->base->bbox_h / 2));*/
			
			while (bbtmp)
			{
				/*printf("  %d %d %d %d\n", bbtmp->x,
						bbtmp->y,
						bbtmp->x - bbtmp->w,
						bbtmp->y - bbtmp->h);*/
				if (  check_bbox_intersect(
						a->pos.x + acc_step[0] - (a->base->bbox_w / 2),
						a->pos.y + acc_step[1] + (a->base->bbox_h / 2),
						a->pos.x + acc_step[0] + (a->base->bbox_w / 2),
						a->pos.y + acc_step[1] - (a->base->bbox_h / 2),
						bbtmp->x,
						bbtmp->y,
						bbtmp->x + bbtmp->w,
						bbtmp->y - bbtmp->h)  )
					goto doclip;
				bbtmp = bbtmp->next;
			}
		}
		
		
		/* test against tilemap b_blockers */
		
		goto noclip;
		
	  doclip:
		/*printf("%d %d %d %d with %d %d %d %d:",a->pos.x + acc_step[0] - (a->base->bbox_w / 2),
						a->pos.y + acc_step[1] + (a->base->bbox_h / 2),
						a->pos.x + acc_step[0] + (a->base->bbox_w / 2),
						a->pos.y + acc_step[1] - (a->base->bbox_h / 2),
						bbtmp->x,
						bbtmp->y,
						bbtmp->x + bbtmp->w,
						bbtmp->y - bbtmp->h);
		printf("do clip -> %d %d %d \n",acc_step_prev[0],acc_step_prev[1],acc_step_prev[2]);*/
		
		a->dpos.x = acc_step_prev[0]; /* we're done. */
		a->dpos.y = acc_step_prev[1];
		a->dpos.z = acc_step_prev[2];
		break;
		
	  noclip:
	  
		acc_step_prev[0] = acc_step[0];
		acc_step_prev[1] = acc_step[1];
		acc_step_prev[2] = acc_step[2];
		
	}
	
	
	
	
	
	/* apply dpos */
	
	a->pos.x += a->dpos.x;
	a->pos.y += a->dpos.y;
	a->pos.z += a->dpos.z;
	
	a->dpos.x = a->dpos.y = a->dpos.z = 0;
	
}


void reset_controller(controller *a)
{
	a->up = 0;
	a->down = 0;
	a->left = 0;
	a->right = 0;
	a->jump = 0;
	a->attk_a = 0;
	a->attk_b = 0;
	a->attk_c = 0;
	a->we = 0;
	a->inv = 0;
	
	a->tap_up = 0;
	a->tap_down = 0;
	a->tap_left = 0;
	a->tap_right = 0;
	a->tap_jump = 0;
	a->tap_attk_a = 0;
	a->tap_attk_b = 0;
	a->tap_attk_c = 0;
	a->tap_we = 0;
	a->tap_inv = 0;
	
	a->key_up = 0;
	a->key_down = 0;
	a->key_left = 0;
	a->key_right = 0;
	a->key_jump = 0;
	a->key_attk_a = 0;
	a->key_attk_b = 0;
	a->key_attk_c = 0;
	a->key_we = 0;
	a->key_inv = 0;
	a->set=0;
}

int file_exists(char *fn)
{
	#ifdef LINUX
		if (access(fn, F_OK) != -1)
			return 1;
		return 0;
	#else
		FILE *fp;
		fp = fopen(fn, "r");
		if (!fp)
			return 0;
		fclose(fp);
		return 1;
	#endif
}

/* read controller settings from file */
void read_ctlr_from_file(char *fn, controller *a)
{
	
	int i;
	
	FILE *fp=0;
	
	if (!file_exists(fn))
	{
		printf("could not find \"%s\"\n",fn);
		return;
	}
	
	fp = fopen(fn, "r");
	
	for (i=0;i<10;i++)
	{
		switch(i)
		{
		case 0:fscanf(fp, "%d;", &(a->key_up));  break;
		case 1:fscanf(fp, "%d;", &(a->key_down));  break;
		case 2:fscanf(fp, "%d;", &(a->key_left));  break;
		case 3:fscanf(fp, "%d;", &(a->key_right));  break;
		case 4:fscanf(fp, "%d;", &(a->key_jump));  break;
		case 5:fscanf(fp, "%d;", &(a->key_attk_a));  break;
		case 6:fscanf(fp, "%d;", &(a->key_attk_b));  break;
		case 7:fscanf(fp, "%d;", &(a->key_attk_c));  break;
		case 8:fscanf(fp, "%d;", &(a->key_we));  break;
		case 9:fscanf(fp, "%d;", &(a->key_inv));  break;
		}
	}
	
	a->set=1;
	
	fclose(fp);
	
}

/* write controller settings to file */
void write_ctlr_to_file(char *fn, controller *a)
{
	
	int i;
	
	FILE *fp=0;
	
	fp = fopen(fn, "w");
	
	for (i=0;i<10;i++)
	{
		switch(i)
		{
		case 0:fprintf(fp, "%d;", (a->key_up));  break;
		case 1:fprintf(fp, "%d;", (a->key_down));  break;
		case 2:fprintf(fp, "%d;", (a->key_left));  break;
		case 3:fprintf(fp, "%d;", (a->key_right));  break;
		case 4:fprintf(fp, "%d;", (a->key_jump));  break;
		case 5:fprintf(fp, "%d;", (a->key_attk_a));  break;
		case 6:fprintf(fp, "%d;", (a->key_attk_b));  break;
		case 7:fprintf(fp, "%d;", (a->key_attk_c));  break;
		case 8:fprintf(fp, "%d;", (a->key_we));  break;
		case 9:fprintf(fp, "%d;", (a->key_inv));  break;
		}
	}
	
	fclose(fp);
}

int keys_get_first(int a[8])
{
	int i;
	
	for (i=0;i<8;i++)
		if (a[i])
			return a[i];
			
	return 0;
}

int keys_null(int a[8])
{
	int i;
	for (i=0;i<8;i++)
		if (a[i])
			return 0;
	return 1;
}

void ctlr_update(int a[8], controller *ctlr)
{
	int i, j;
	
	ctlr->tap_up=
	ctlr->tap_down=
	ctlr->tap_left=
	ctlr->tap_right=
	ctlr->tap_jump=
	ctlr->tap_attk_a=
	ctlr->tap_attk_b=
	ctlr->tap_attk_c=
	ctlr->tap_we=
	ctlr->tap_inv=0;
	
	#define DO_CHECK_KEY(aa,b,c) \
		j=0; \
		for (i=0;i<8;i++) \
			if (a[i] == aa) \
			{ \
				j=1; \
				if (!c) \
					b=1; \
				c=1; \
				break; \
			} \
		if (!j) \
			c=0;
	
	DO_CHECK_KEY(ctlr->key_up,ctlr->tap_up,ctlr->up);
	DO_CHECK_KEY(ctlr->key_down,ctlr->tap_down,ctlr->down);
	DO_CHECK_KEY(ctlr->key_left,ctlr->tap_left,ctlr->left);
	DO_CHECK_KEY(ctlr->key_right,ctlr->tap_right,ctlr->right);
	DO_CHECK_KEY(ctlr->key_jump,ctlr->tap_jump,ctlr->jump);
	DO_CHECK_KEY(ctlr->key_attk_a,ctlr->tap_attk_a,ctlr->attk_a);
	DO_CHECK_KEY(ctlr->key_attk_b,ctlr->tap_attk_b,ctlr->attk_b);
	DO_CHECK_KEY(ctlr->key_attk_c,ctlr->tap_attk_c,ctlr->attk_c);
	DO_CHECK_KEY(ctlr->key_we,ctlr->tap_we,ctlr->we);
	DO_CHECK_KEY(ctlr->key_inv,ctlr->tap_inv,ctlr->inv);
}

void reset_sprite_active(sprite_active *a)
{
	a->cur=0;
	a->cntr=0;
	a->done=0;
}

void apply_dpos_chara_sprite_active(chara_active *a, sprite_active *b)
{
	int rframe = (b->cur * b->base->intrv) + b->cntr;

	a->dpos.x = b->base->drift[rframe].x;
	a->dpos.y = b->base->drift[rframe].y;
	a->dpos.z = b->base->drift[rframe].z;
}


world * new_world()
{
	world *n = (world *) malloc(sizeof(world));
	
	n->rooms = 0;
	n->chara_temps = 0;
	n->sprites = 0;
	
	return n;
}

void chara_template_add(world *in_world, char* script)
{
	chara_template *tmp=in_world->chara_temps;
	
	if (!tmp)
	{
		in_world->chara_temps = new_chara_template(in_world, script);
		return;
	}
	while (tmp)
	{
		if (!tmp->next)
		{
			tmp->next = new_chara_template(in_world, script);
			break;
		}
		tmp = tmp->next;
	}
}

chara_template * get_chara_template(world *in_world, char* name)
{
	chara_template *tmp = in_world->chara_temps;
	
	while (tmp)
	{
		if (!strcmp(name, tmp->name))
			break;
		tmp = tmp->next;
	}
	
	return tmp;
}

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
				cur_node->float_dat = (float) atof(cur_node->str_dat);
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

int json_tree_get_int_from_obj(json_parse_node *a, const char *b)
{
	int i;
	if (a->type != JSON_PARSE_ARR && a->type != JSON_PARSE_OBJ)
		return -1;
	
	for (i=0;i<a->cnt;i++)
	{
		if (!strcmp(a->keys[i], b))
			return a->values[i]->int_dat;
	}
		
	return -1;
}

json_parse_node * json_tree_get_node_from_obj(json_parse_node *a, const char *b)
{
	int i;
	if (a->type != JSON_PARSE_ARR && a->type != JSON_PARSE_OBJ)
		return 0;
	for (i=0;i<a->cnt;i++)
		if (!strcmp(a->keys[i], b))
			return a->values[i];
	return 0;
}

json_parse_node * json_tree_get_node_from_arr(json_parse_node *a, int b)
{
	if (a->type != JSON_PARSE_ARR && a->type != JSON_PARSE_OBJ)
		return 0;
	return a->values[b];
}

/* uses strcpy */
void json_tree_get_str_from_obj(json_parse_node *a, const char *b, char *dst)
{
	int i;
	if (a->type != JSON_PARSE_ARR && a->type != JSON_PARSE_OBJ)
		return;
	for (i=0;i<a->cnt;i++)
		if (!strcmp(a->keys[i], b))
		{
			strcpy(dst, a->values[i]->str_dat);
			return;
		}
}

room * get_room(world *in_world, char *a)
{
	room *tmp = in_world->rooms;
	
	while (tmp)
	{
		if (!strcmp(a, tmp->name))
			return tmp;
		tmp=tmp->next;
	}
	return (room *) 0;
} 

int get_room_idx(world *in_world, char *a)
{
	room *tmp = in_world->rooms;
	int i=0;
	
	while (tmp)
	{
		if (!strcmp(a, tmp->name))
			return i;
		tmp=tmp->next;
		i++;
	}
	return -1;
}

room * get_room_by_idx(world *in_world, int a)
{
	room *tmp = in_world->rooms;
	int i=0;
	
	while (tmp)
	{
		if (a==i)
			return tmp;
		tmp=tmp->next;
		i++;
	}
	return 0;
}

int new_placer(world *in_world, char * script, room *rtmp)
{
	QUICK_PARSE_DECLARATIONS
	
	placer *n = (placer*) malloc(sizeof(placer));
	int success=0;
	
	n->c=0; /* if null, not a chara temp placer. */
	n->c_name[0]='\0';
	n->c_start_mode=0; /* starting mode for chara_template.  if < 0, don't use. */
	n->playr=0;/* if < 0, not a player placer. if -2, npc.  more codes may be added. */
	n->type = -1;
	n->to; /* exit */
	
	n->pos.x=n->pos.y=n->pos.z=0;
	
	n->next=0;
	
	
	QUICK_PARSE_BEFORE_LINE
	
			/* npc|player|exit <chara template>|<room name (-> int # )|player #> x y z */
	
			/* FIXME: all placers need starting mode specifier.
			 * 			exits need chara templates still.
			 */
			
			if (	nargs==6 &&
					!strcmp("npc",buf[0]) &&
					strlen(buf[1]) < 31 &&
					is_int[2] && is_int[3] && is_int[4])
			{
				if (!in_world)
				{
					free(n);
					return 0;
				}
				n->type = PLACER_NPC;
				strcpy(n->c_name, buf[1]); /* just the name */
				n->pos.x = atoi(buf[2]);
				n->pos.y = atoi(buf[3]);
				n->pos.z = atoi(buf[4]);
				rtmp = get_room(in_world, buf[5]);
			}
			else if (	nargs==2 &&
					!strcmp("npc",buf[0]) &&
					strlen(buf[1]) < 31)
			{
				n->type = PLACER_NPC;
				strcpy(n->c_name, buf[1]); /* just the name */
			}
			else if (	nargs==3 &&
					!strcmp("npc",buf[0]) &&
					strlen(buf[1]) < 31 &&
					is_int[2])
			{
				n->type = PLACER_NPC;
				strcpy(n->c_name, buf[1]); /* just the name */
				n->c_start_mode = atoi(buf[2]);
			}


			else if (	nargs==6 &&
					!strcmp("player",buf[0]) &&
					is_int[1] && is_int[2] && is_int[3] && is_int[4])
			{
				if (!in_world)
				{
					free(n);
					return 0;
				}
				n->type = PLACER_PLAYER;
				n->playr = atoi(buf[1]);
				n->pos.x = atoi(buf[2]);
				n->pos.y = atoi(buf[3]);
				n->pos.z = atoi(buf[4]);
				rtmp = get_room(in_world, buf[5]);
			}
			else if (	nargs==2 &&
					!strcmp("player",buf[0]) &&
					is_int[1])
			{
				n->type = PLACER_PLAYER;
				n->playr = atoi(buf[1]);
			}
			else if (	nargs==1 &&
					!strcmp("player",buf[0]))
			{
				n->type = PLACER_PLAYER;
				n->playr = 0;
			}

			else if (	nargs==6 &&
					!strcmp("exit",buf[0]) &&
					is_int[2] && is_int[3] && is_int[4])
			{
				if (!in_world)
				{
					free(n);
					return 0;
				}
				n->type = PLACER_EXIT;
				n->to = get_room_idx(in_world, buf[1]);
				n->pos.x = atoi(buf[2]);
				n->pos.y = atoi(buf[3]);
				n->pos.z = atoi(buf[4]);
				rtmp = get_room(in_world, buf[5]);
			}
			else if (	nargs==2 &&
					!strcmp("exit",buf[0]) &&
					strlen(buf[1]) < 31)
			{
				n->type = PLACER_EXIT;
				strcpy(n->c_name, buf[1]); /* just the name */
			}
			
	QUICK_PARSE_AFTER_LINE
	
	
	if (rtmp && n->type>=0)
	{
		placer *ptmp=rtmp->placers;
		if (!ptmp)
		{
			rtmp->placers = n;
			return 1;
		}
		while (ptmp)
		{
			if (!ptmp->next)
			{
				ptmp->next = n;
				success=1;
				break;
			}
			ptmp = ptmp->next;
		}
	}
	else
		free(n);
	
	return success;
}

void placer_add(world *in_world, char* script)
{
	new_placer(in_world, script, 0); /* this automatically handles adding new structure */
}

tilemap * load_tilemap_from_json(char *fn, room *inroom) /* we need room for gathering placers. */
{
	FILE *fp=0;
	int ln=0, tw, th, ntiles, tsize, r, i, j, k, tmpw, tmph,
		ts_load_cur_tile, tmpx, tmpy;
	char *dat=0, tmpc;
	tilemap *tm = 0;
	json_parse_node *test_tree = 0, *tmp=0, *tmp1=0, *tmp2=0;
	SDL_Rect tmp_clip_rect;
	SDL_Surface **tilesets;
	b_blocker *btmp;
	char tmp_str[256];
	
	jsmn_parser p;
	jsmntok_t *t=0;
	placer *pltmp;
	
	fp = fopen(fn,"r");
	
	if (!fp)
		goto ld_tilemap_ff_done;
	
	fseek(fp, 0L, SEEK_END);
	ln = ftell(fp);
	
	if (!ln)
		goto ld_tilemap_ff_done;
	
	
	dat = (char*) malloc(ln + 1);

	t = (jsmntok_t *) malloc(sizeof(jsmntok_t) * ln);
	
	
	
	rewind(fp);
	
	i=0;
	while(i<ln)
	{
		tmpc=fgetc(fp);
		if (tmpc==EOF)
		{
			dat[i]='\0';
			break;
		}
		else
			dat[i++] = tmpc;
	}
	/*fread(dat, ln, 1, fp);*/
	
	
	fclose(fp);
	fp=0;
	
	jsmn_init(&p);
	r = jsmn_parse(&p, dat, ln, t, ln);
	
	/* We're assuming json tilemap was made in Tiled ~ver 0.18.2,
	 * has right-down render order,
	 * has at least 1 tilelayer same size as tilemap,
	 * and image for tileset exists.
	 */
	
	
	if (r < 0)
		goto ld_tilemap_ff_done;
	
	test_tree = json_parse_mk_tree(dat, t, r);
	
	th = json_tree_get_int_from_obj(test_tree, "height");
	tw = json_tree_get_int_from_obj(test_tree, "width");
	tsize = json_tree_get_int_from_obj(test_tree, "tilewidth");
	
	/*printf("th %d tw %d tsize %d\n", th,tw,tsize);*/
	
	/* load tilesets (images), count up n tiles, cut tilesets into tiles for tilemap */
	
	/* TODO: load blocker info from properties*/
	tmp = json_tree_get_node_from_obj(test_tree, "tilesets");
	
	tilesets = (SDL_Surface **) malloc(sizeof(SDL_Surface *) * tmp->cnt);
	
	/*printf("tilesets cnt %d\n", tmp->cnt);*/
	
	ntiles=0;
	

	for (i=0;i<tmp->cnt;i++)
	{
		tmp1 = json_tree_get_node_from_arr(tmp, i);
		tmpw = json_tree_get_int_from_obj(tmp1,"imagewidth") / tsize;
		tmph = json_tree_get_int_from_obj(tmp1,"imageheight") / tsize;
		
		json_tree_get_str_from_obj(tmp1,"image",tmp_str);
		
		/*printf("%s %d %d\n", tmp_str, tmpw,tmph);*/
		tilesets[i] = IMG_Load(tmp_str);
		printf("tile set %d \"%s\" loaded!\n", i, tmp_str);
		
		if (!tilesets[i])
		{
			printf("couldn't load \"%s\".\n",tmp_str);
			goto ld_tilemap_ff_done;
		}
			
		
		ntiles += tmpw * tmph;
	}
	
	tm = new_tilemap(tw,th,ntiles);

	tm->tsize = tsize;
	
	tm->b_block = 0;
	
	/*printf("ntiles %d\n", ntiles);*/
	
	tmp_clip_rect.w = tsize;
	tmp_clip_rect.h = tsize;
	tmp_clip_rect.x = 0;
	tmp_clip_rect.y = 0;
	
	ts_load_cur_tile=0;
	
	for (i=0;i<ntiles;i++)
		tm->tiles[i] = SDL_CreateRGBSurface(0,tsize,tsize,32,0,0,0,0);

	
	for (i = 0; i < tmp->cnt; i++)
	{
		tmp_clip_rect.y = 0;
		while (tmp_clip_rect.y  + tsize-1< tilesets[i]->h)
		{
			tmp_clip_rect.x = 0;
			while (tmp_clip_rect.x + tsize-1 < tilesets[i]->w && ts_load_cur_tile < ntiles)
			{
				
				SDL_BlitSurface(tilesets[i], &tmp_clip_rect, tm->tiles[ts_load_cur_tile], 0);
				
				printf("filled tile %d %d at %d %d in tileset %d %p\n",
					ts_load_cur_tile,
					tm->tiles[ts_load_cur_tile],
					tmp_clip_rect.x,
					tmp_clip_rect.y,
					i,
					tilesets[i]);
				
				tmp_clip_rect.x+=tsize;
				
				ts_load_cur_tile++;
			}
			tmp_clip_rect.y+=tsize;
		}
	}
	
	
	tmp = json_tree_get_node_from_obj(test_tree, "layers");
	for (i=0;i<tmp->cnt;i++)
	{
		tmp1 = json_tree_get_node_from_arr(tmp, i);

		json_tree_get_str_from_obj(tmp1,"type",tmp_str);

		if (!strcmp(tmp_str, "tilelayer"))
		{		/* load first tile layer for tilemap. */
			printf("%s: found tilelayer\n", fn);

			tmp1 = json_tree_get_node_from_obj(tmp1, "data");
			tmpx=tmpy=j=0;
			for (tmpy=0;tmpy < tm->h;tmpy++)
				for (tmpx=0;tmpx < tm->w;tmpx++)
					tm->arr[tmpy][tmpx] = tmp1->values[j++]->int_dat - 1;
		}

		else if (!strcmp(tmp_str, "objectgroup"))
		{	/* look through other layers for object layers that may be used as placers */
			printf("%s: found objectgroup\n", fn);

			tmp1 = json_tree_get_node_from_obj(tmp1, "objects");

			for (j=0;j<tmp1->cnt;j++)
			{
				tmp2 = json_tree_get_node_from_arr(tmp1, j);
				
				json_tree_get_str_from_obj(tmp2,"name",tmp_str);
				
				if (!strcmp(tmp_str, "wall")) /* collision wall ?*/
				{
					tmpx = json_tree_get_int_from_obj(tmp2, "x");
					tmpy = json_tree_get_int_from_obj(tmp2, "y");
					tmpw = json_tree_get_int_from_obj(tmp2, "width");
					tmph = json_tree_get_int_from_obj(tmp2, "height");
					
					printf("wall: %d %d %d %d\n",tmpx,tmpy,tmpw,tmph);
					
					#define NEW_BBLOCK(atmp) \
						atmp = (b_blocker *) malloc(sizeof(b_blocker)); \
						atmp->x = tmpx; \
						atmp->y = -tmpy; \
						atmp->w = tmpw; \
						atmp->h = tmph; \
						atmp->next = 0;
					
					if (!tm->b_block)
					{
						NEW_BBLOCK(tm->b_block);
					}
					else
					{
						btmp = tm->b_block;
						
						while (btmp)
						{
							if (!btmp->next)
							{
								NEW_BBLOCK(btmp->next);
								break;
							}
							btmp=btmp->next;
						}
					}
				}
				else /* placer ? */
				{
					tmpx = json_tree_get_int_from_obj(tmp2, "x");
					tmpy = json_tree_get_int_from_obj(tmp2, "y");
					tmpx += json_tree_get_int_from_obj(tmp2, "width")/2;
					tmpy += json_tree_get_int_from_obj(tmp2, "height")/2;
					k=strlen(tmp_str); /* append semicolon */
					if (k<255)
					{
						tmp_str[k]=';';
						tmp_str[k+1]='\0';
					}
					if (new_placer(0, tmp_str, inroom)==1) /* if placer added successfully */
					{
						pltmp = inroom->placers;
						while (pltmp)
						{
							if (pltmp->next==0)
							{
								pltmp->pos.x = tmpx;
								pltmp->pos.y = -tmpy;
							}
							pltmp=pltmp->next;
						}
					}
				}
					
			}
		}
	}
	
	
	/* TODO: free test_tree, tilesets */
	
	ld_tilemap_ff_done:
	if (t)
		free(t);
	
	if (dat)
		free(dat);
	
	if (fp)
		fclose(fp);

	return tm;
}

room * new_room(char* script)
{
	QUICK_PARSE_DECLARATIONS
	
	room *n = (room*) malloc(sizeof(room));
	
	n->tm_main;
	n->tm_mult;
	n->bgm_id=-1;
	n->next = 0;
	n->placers = 0;
	n->name[0]='\0';
	
	QUICK_PARSE_BEFORE_LINE
	
			if (	nargs==3 &&
					!is_int[0] &&
					!is_int[1] &&
					!is_int[2] &&
					strlen(buf[0]) < 31 &&
					strlen(buf[1]) < 255 &&
					strlen(buf[2]) < 255)
			{
				strcpy(n->name, buf[0]);
				
				n->tm_main = load_tilemap_from_json(buf[1], n);
				n->tm_mult = load_tilemap_from_json(buf[2], n);
			}
			
	QUICK_PARSE_AFTER_LINE
	
	return n;
}

void room_add(world *in_world, char* script)
{
	room *tmp=in_world->rooms;
	
	if (!tmp)
	{
		in_world->rooms = new_room(script);
		return;
	}
	while (tmp)
	{
		if (!tmp->next)
		{
			tmp->next = new_room(script);
			break;
		}
		tmp = tmp->next;
	}
}



void show_action_frames(action_frame *af_list_head, cam *incam, SDL_Surface *s)
{
	int cx,cy,x,y;
	action_frame *tmp = af_list_head->next;
	SDL_Rect rtmp;

	/* adjust cam */
	if (!incam)
		cx=cy=0;
		
	else if (incam->target)
	{
		cx = incam->target->pos.x;
		cy = incam->target->pos.y;
	}
	else
	{
		cx = incam->x; /* take into account cam bounding box here ? */
		cy = incam->y;
	}
	
	
	while (tmp)
	{
		if (tmp->cntr>0)
		{
			x=(tmp->owner->pos.x + tmp->pos.x) - tmp->w/2;
			y=(tmp->owner->pos.y + tmp->pos.y) + tmp->h/2 + tmp->z;
			
			rtmp.x = (SWIDTH/2) + x - cx;
			rtmp.y = (SHEIGHT/2) - y + cy;
			rtmp.w = tmp->w;
			rtmp.h = tmp->h;

			SDL_FillRect(s, &rtmp, 0xff00ffff);
		}

		tmp=tmp->next;
	}
}

void active_chara_sprite_tick_action_frame(action_frame *af_list_head, chara_active *ca, sprite_active *sa)
{
	action_frame *tmp;
	
	/* from given active sprite, if on attack frame start */
	if (sa->cntr == 0 && sa->cur == sa->base->attk_frame_start && sa->base->attk_frame.cntr > 0)
	{
				
		tmp=af_list_head->next;
		
		if (!tmp)
			tmp=af_list_head;
		
		while (tmp)
		{
			if (tmp->owner==ca && tmp->source==sa)
				tmp->cntr--;

			else if (!tmp->next)
			{
				tmp->next = (action_frame*) malloc(sizeof(action_frame));
				tmp=tmp->next;
				
				tmp->next=0;
				tmp->owner = ca;
				tmp->source = sa;
				tmp->cntr = sa->base->attk_frame.cntr + 1;
				tmp->type = sa->base->attk_frame.type;
				tmp->pos.x=sa->base->attk_frame.pos.x;
				tmp->pos.y=sa->base->attk_frame.pos.y;
				tmp->pos.z=sa->base->attk_frame.pos.z;
				tmp->w = sa->base->attk_frame.w;
				tmp->h = sa->base->attk_frame.h;
				tmp->z = sa->base->attk_frame.z;
				tmp->target.x = sa->base->attk_frame.target.x;
				tmp->target.y = sa->base->attk_frame.target.y;
				tmp->target.z = sa->base->attk_frame.target.z;
				
				break;
			}
			tmp=tmp->next;
		}
	}
}

void action_frame_check_if_hit(action_frame *af_list_head, chara_active *ca)
{
	action_frame *tmp = af_list_head->next;
	int ca_bbox[4];  /*xy0 xy1*/
	int af_bbox[4];
	
	/* bboxes are in global coords, as
	 * check_bbox_intersect tests in
	 * global coords.
	 */
	
	ca_bbox[0] = ca->pos.x - ca->base->bbox_w/2;
	ca_bbox[1] = ca->pos.y + ca->base->bbox_h/2;
	ca_bbox[2] = ca->pos.x + ca->base->bbox_w/2;
	ca_bbox[3] = ca->pos.y - ca->base->bbox_h/2;
	
	while (tmp)
	{
		
		af_bbox[0] = (tmp->owner->pos.x + tmp->pos.x) - tmp->w/2;
		af_bbox[1] = (tmp->owner->pos.y + tmp->pos.y) + tmp->h/2 + tmp->z;
		af_bbox[2] = af_bbox[0] + tmp->w;
		af_bbox[3] = af_bbox[1] - tmp->h;
		
		if (
				check_bbox_intersect(
					ca_bbox[0],ca_bbox[1],ca_bbox[2],ca_bbox[3],
					af_bbox[0],af_bbox[1],af_bbox[2],af_bbox[3]   ) &&
				ca->af_hit_cnt < 32
			)
		{
			ca->af_hit[ca->af_hit_cnt] = tmp;
			ca->af_hit_cnt++;
		}
			
		
		tmp=tmp->next;
	}
}

void chara_active_af_apply_drift(chara_active *ca)
{
	int i;
	
	for (i=0;i<ca->af_hit_cnt;i++)
	{
		
		ca->drift_dec=1;
		ca->drift_intrv=8; /* 0 = decrement drift by drift_dec every frame. */
		ca->drift_state=0; /* 0 = use drift dec. */
		
		ca->drift.x += ca->af_hit[i]->target.x;
		ca->drift.y += ca->af_hit[i]->target.y;
		ca->drift.z += ca->af_hit[i]->target.z;
		
	}
}

void action_frame_clear_chara(action_frame *af_list_head, chara_active *ca)
{
	action_frame *tmp = af_list_head->next, *prev=0;
	
	while (tmp)
	{
		if (tmp->owner == ca)
		{
			if (prev)
			{
				prev->next = tmp->next;
				free(tmp);
				tmp=prev;
			}
			else
			{
				af_list_head->next = tmp->next;
				free(tmp);
				tmp=af_list_head;
			}
		}
		tmp=tmp->next;
	}
}

int main(void)
{
	/* initialize things: */
	
	int i, j, actor_cnt=0,
		k_w=0, k_a=0, k_s=0,
		k_d=0,k_space=0, tick_shad = -1, run = 1,
		game_mode, cntr_a, cntr_b, cntr_c, game_mode_first_loop,
		key_tmp, player_focus, nplayers;

	/* character jump table.  this holds the "arc" for jumping.
	 * it's only half of it, and there's code below that finishes
	 * the job. */
	int ch_jump_ani_table[64] = {3,3,3,2,2,2,1,1,1,0,1,0,1,0,1,0,1,-999};
	
	int keys_down[8] = {0,0,0,0,0,0,0,0};
	
	int debug_f[DEBUG_COUNT];
	
	SDL_Event e;
	SDL_Surface *main_display, *tmpd, *s_layer;
	SDL_Window  *win;
	SDL_Rect 	pos;
	sprite  *sprt_shad, *sprt_logo;
	render_sprite_head rstest;
	tilemap *tmap_main_select;
	chara_active *c_active = 0, *c_a_last = 0, *catmp;
	cam testcam;
	sprite *sprt_lhud;
	float ch0_health=1.0;
	ani *test_ani;
	controller ctlr_main;
	world test_world;
	player players[16];
	world *select_world;
	action_frame aframe_list;
	
	SDL_Color font_default = {255,255,255,255};
	SDL_Color font_outline = {32,32,128,255};
	TTF_Font *font = 0;

	/* fader mechanism */
	int fader_md=0, fader_cnt=0, fade_speed=(10);

	#define SET_FADE_IN() {fader_md=1;fader_cnt=255;}
	#define SET_FADE_OUT() {fader_md=2;fader_cnt=0;}
	#define SET_FADE_OFF() {fader_md=0;fader_cnt=0;}
	#define SET_FADE_ON() {fader_md=0;fader_cnt=255;}
	
	/* reset debug flags */
	for (i=0;i<DEBUG_COUNT;i++)
		debug_f[i] = 0;

	/* reset action frame list */
	aframe_list.next=0;

	reset_controller(&ctlr_main);
	
	
	reset_cam(&testcam);
	
	rstest.next = 0;
	clear_render_sprites(&rstest);

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
	
	test_world.rooms = 0;
	test_world.chara_temps=0;
	test_world.sprites=0;
	test_world.attk_frames=0;
	test_world.dfnd_frames=0;
	
	room_add(&test_world, "main desert-test.json desert-test-lightmap.json;");
	
	/* load sprites */
	sprite_add(&test_world,"frames 1 10;name logo;  cxy 0 0;transp 0;intrv 10;loop 0;img 0 logo.png;");
	sprite_add(&test_world,"frames 1 10;name hud_health_l;  cxy 0 0;transp 0;frames 1;intrv 10;loop 0;img 0 hud_health_l.png;");
	sprite_add(&test_world,"frames 1 10;name jar;  cxy 8 14;transp 0;frames 1;intrv 10;loop 0;img 0 jar.png;");
	
	sprite_add(&test_world,"frames 2 10;name ch0_left_move;  cxy 16 28;transp 0;loop 1;img 0 l_1.png;img 1 l_0.png;drift all -1 0 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_left_stand;  cxy 16 28;transp 0;loop 0;img 0 l_0.png;");
	sprite_add(&test_world,"frames 2 10;name ch0_right_move;  cxy 16 28;transp 0;loop 1;img 0 r_1.png;img 1 r_0.png;drift all 1 0 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_right_stand;  cxy 16 28;transp 0;loop 0;img 0 r_0.png;");
	sprite_add(&test_world,"frames 2 10;name ch0_up_move;  cxy 16 28;transp 0;loop 1;img 0 u_1.png;img 1 u_0.png;drift all 0 1 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_up_stand;  cxy 16 28;transp 0;loop 0;img 0 u_0.png;");
	sprite_add(&test_world,"frames 2 10;name ch0_down_move;  cxy 16 28;transp 0;loop 1;img 0 d_1.png;img 1 d_0.png;drift all 0 -1 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_down_stand;  cxy 16 28;transp 0;loop 0;img 0 d_0.png;");

	sprite_add(&test_world,"frames 1 10;name ch0_up_attk_basic;  cxy 16 28;transp 0;loop 0;img 0 u_1.png;attk_frame_start 0;attk_frame_len 2; attk_frame_bbox 0 10    20 20 10;attk_frame_target 0 3 0;   drift 0 0 2 0;drift 1 0 2 0;drift 2 0 2 0;drift 3 0 2 0;    drift 4 0 1 0;drift 5 0 1 0;drift 6 0 0 0;drift 7 0 0 0;drift 8 0 0 0;drift 9 0 0 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_down_attk_basic;  cxy 16 28;transp 0;loop 0;img 0 d_1.png;attk_frame_start 0;attk_frame_len 2; attk_frame_bbox 0 -10 20 20 10;attk_frame_target 0 -3 0; drift 0 0 -2 0;drift 1 0 -2 0;drift 2 0 -2 0;drift 3 0 -2 0;drift 4 0 -1 0;drift 5 0 -1 0;drift 6 0 0 0;drift 7 0 0 0;drift 8 0 0 0;drift 9 0 0 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_left_attk_basic;  cxy 16 28;transp 0;loop 0;img 0 l_1.png;attk_frame_start 0;attk_frame_len 2; attk_frame_bbox -10 0 20 20 10;attk_frame_target -3 0 0; drift 0 -2 0 0;drift 1 -2 0 0;drift 2 -2 0 0;drift 3 -2 0 0;drift 4 -1 0 0;drift 5 -1 0 0;drift 6 0 0 0;drift 7 0 0 0;drift 8 0 0 0;drift 9 0 0 0;");
	sprite_add(&test_world,"frames 1 10;name ch0_right_attk_basic;  cxy 16 28;transp 0;loop 0;img 0 r_1.png;attk_frame_start 0;attk_frame_len 2; attk_frame_bbox 10 0 20 20 10;attk_frame_target 3 0 0;drift 0 2 0 0;drift 1 2 0 0;drift 2 2 0 0;drift 3 2 0 0;    drift 4 1 0 0;drift 5 1 0 0;drift 6 0 0 0;drift 7 0 0 0;drift 8 0 0 0;drift 9 0 0 0;");
		
	chara_template_add(&test_world, "name ch0;max_hp 40;max_mp 20;attack 10;defend 10;bbox 10 10 30;type 0;gfx_cnt 32;gfx  0 ch0_left_move;gfx  1 ch0_left_stand;gfx  2 ch0_right_move;gfx  3 ch0_right_stand;gfx  4 ch0_up_move;gfx  5 ch0_up_stand;gfx  6 ch0_down_move;gfx  7 ch0_down_stand;gfx  8 ch0_up_attk_basic;gfx  9 ch0_down_attk_basic;gfx  10 ch0_left_attk_basic;gfx  11 ch0_right_attk_basic;");
	chara_template_add(&test_world, "name jar;max_hp 40;max_mp 20;attack 10;defend 10;bbox 10 10 30;type 1;gfx_cnt 1;gfx  0 jar;");
		
	
	sprt_lhud = get_sprite(&test_world, "hud_health_l");
	sprt_logo = get_sprite(&test_world, "logo");
	
	/*placer_add(&test_world, "player 0 70 -70 1 main;");
	placer_add(&test_world, "npc jar 70 -70 1 main;");*/
	
	
	
	
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
			SWIDTH * SMULT, SHEIGHT * SMULT,
			0
		);
	
	/* get window surface; create framebuffer */
	main_display = SDL_GetWindowSurface(win);
	
	tmpd = SDL_CreateRGBSurface(0, SWIDTH, SHEIGHT,32,0,0,0,0);
	s_layer = SDL_CreateRGBSurface(0, SWIDTH, SHEIGHT,32,0xff000000,0xff0000,0xff00,0xff);
	
	SDL_SetSurfaceBlendMode(tmpd, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceBlendMode(s_layer, SDL_BLENDMODE_BLEND);
	
	SDL_SetSurfaceBlendMode(main_display, SDL_BLENDMODE_BLEND);
	
	
	/* create shadow sprite*/
	sprite_add(&test_world,"frames 1 10;name item_shadow;  cxy 8 2;transp 1;frames 1;intrv 10;loop 0;img 0 shad.png;");
	sprt_shad = get_sprite(&test_world, "item_shadow");
	
	game_mode = MD_CTLR_CHECK;
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
				key_tmp=1;
				for (i=0;i<8;i++)
					if (keys_down[i]==e.key.keysym.sym)
					{
						key_tmp=0;
						break;
					}
				if (key_tmp)
					for (i=0;i<8;i++)
						if (keys_down[i]==0)
						{
							keys_down[i]=e.key.keysym.sym;
							break;
						}
				switch(e.key.keysym.sym)
				{ case SDLK_ESCAPE: run=0;  break; }
				break;
				
			case SDL_KEYUP:
				for (i=0;i<8;i++)
					if (keys_down[i] == e.key.keysym.sym)
						keys_down[i]=0;
				break;	
			}
		
		ctlr_update(keys_down, &ctlr_main);
		
		switch(game_mode)
		{
		case MD_CTLR_CHECK:
			read_ctlr_from_file("controls.cfg", &ctlr_main);
			
			if (ctlr_main.set)
				game_mode = MD_LOGO;
			else
				game_mode = MD_SET_CONTROLS;
			
			cntr_c=0;
			
				
			break;
		case MD_SET_CONTROLS:
			SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0, 0, 0, 255 ) );
			
			render_text(tmpd,&font_default,&font_outline,font,"Control Setup", 10,10, TEXT_OUTLINE | TEXT_BOLD);
			

			#define MD_SET_CONTROLS_CHECK(a,b) \
					render_text(tmpd,&font_default,&font_outline,font,a, (SWIDTH/2)-20,SHEIGHT/2, TEXT_OUTLINE|TEXT_BOLD); \
					i=keys_get_first(keys_down); \
					if (i) \
					{ \
						ctlr_main.b = i; \
						cntr_c++; \
					}
			
			#define MD_SET_CONTROLS_NEXT() \
					if (keys_null(keys_down)) \
						cntr_c++;
			
			switch(cntr_c)
			{
			case 0:  MD_SET_CONTROLS_CHECK("Press key for UP",key_up);break;
			case 1:  MD_SET_CONTROLS_NEXT();  break;
			case 2:  MD_SET_CONTROLS_CHECK("Press key for DOWN",key_down);  break;
			case 3:  MD_SET_CONTROLS_NEXT();  break;
			case 4:  MD_SET_CONTROLS_CHECK("Press key for LEFT",key_left);  break;
			case 5:  MD_SET_CONTROLS_NEXT();  break;
			case 6:  MD_SET_CONTROLS_CHECK("Press key for RIGHT",key_right);  break;
			case 7:  MD_SET_CONTROLS_NEXT();  break;
			case 8:  MD_SET_CONTROLS_CHECK("Press key for JUMP",key_jump);  break;
			case 9:  MD_SET_CONTROLS_NEXT();  break;
			case 10:  MD_SET_CONTROLS_CHECK("Press key for ATTACK A",key_attk_a);  break;
			case 11:  MD_SET_CONTROLS_NEXT();  break;
			case 12:  MD_SET_CONTROLS_CHECK("Press key for ATTACK B",key_attk_b);  break;
			case 13:  MD_SET_CONTROLS_NEXT();  break;
			case 14:  MD_SET_CONTROLS_CHECK("Press key for ATTACK C",key_attk_c);  break;
			case 15:  MD_SET_CONTROLS_NEXT();  break;
			case 16:  MD_SET_CONTROLS_CHECK("Press key for WEAPON SELECT",key_we);  break;
			case 17:  MD_SET_CONTROLS_NEXT();  break;
			case 18:  MD_SET_CONTROLS_CHECK("Press key for START",key_inv);  break;
			case 19:  MD_SET_CONTROLS_NEXT();  break;
			case 20:
				render_text(tmpd,&font_default,&font_outline,font,"Saving controller settings ...", (SWIDTH/2)-20,SHEIGHT/2, TEXT_OUTLINE|TEXT_BOLD);
				cntr_c++;
				break;
			case 21:
				write_ctlr_to_file("controls.cfg", &ctlr_main);
				cntr_c++;
				break;
			case 22:
				render_text(tmpd,&font_default,&font_outline,font,"Saving controller settings ... OK", (SWIDTH/2)-20,SHEIGHT/2, TEXT_OUTLINE|TEXT_BOLD);
				cntr_c++;
				game_mode=MD_LOGO;
				break;
			}
			
			break;
		case MD_LOGO:
			SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0, 0, 0, 255 ) );
			pos.x = (SWIDTH/2) - ((sprt_logo->sprt_arr[0]->w) / 2);
			pos.y = (SHEIGHT/2) - ((sprt_logo->sprt_arr[0]->h) / 2);
			SDL_BlitSurface(sprt_logo->sprt_arr[0], 0 , tmpd, &pos);
			
			if ((ctlr_main.inv || ctlr_main.jump) && cntr_a>0 && cntr_a < FPS)
				cntr_a=FPS-1;

			if (fader_cnt==0 && cntr_a==0)
				cntr_a=1;
				
			if (cntr_a>0 && cntr_a<=FPS)
				cntr_a++;
			
			if (cntr_a==FPS)
				SET_FADE_OUT();

			if (cntr_a > FPS && fader_cnt == 255)
			{
				game_mode_first_loop=1;
				game_mode = MD_MENU;
				
				SET_FADE_ON();
			}
			break;
		case MD_MENU:
			if (game_mode_first_loop)
			{				
				SET_FADE_IN();
				
				game_mode_first_loop=0;
				
				test_ani = new_ani("l titlebg.png;l brawllords.png;l bl_smoke_left.png;l bl_smoke_right.png;p 0 0 0;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;n;u 1;p 1 71 0;n;u 1;p 1 71 5;n;u 1;p 1 71 10;n;u 1;p 1 71 15;n;u 1;p 1 71 20;n;u 1;p 1 71 25;n;u 1;p 1 71 30;n;u 1;p 1 71 35;n;u 1;p 1 71 40;n;u 1;p 1 71 45;n;u 1;p 1 71 50;n;u 1;p 1 71 55;n;u 1;p 1 71 60;n;u 1;p 1 71 65;n;u 1;p 1 71 70;n;u 1;p 1 71 75;n;u 1;p 1 71 80;n;u 1;p 1 71 85;n;u 1;p 1 71 90;n;u 1;p 1 71 95;n;u 1;p 1 71 100;n;u 1;p 1 71 105;n;u 1;p 1 71 110;n;u 1;p 1 71 115;n;u 1;p 1 71 120;n;u 1;p 1 71 126;n;u 1;p 1 71 128;n;u 1;p 1 71 131;n;u 1;p 1 71 141;n;u 1;p 1 71 151;n;u 2;p 2 52 165;u 3;p 3 240 165;u 0;p 0 0 2;u 1;p 1 71 153;;n;;u 0;p 0 0 0;u 1;p 1 71 151;n;;u 0;p 0 0 2;u 1;p 1 71 153;n;u 2;p 2 42 165;u 3;p 3 250 165;;u 0;p 0 0 0;u 1;p 1 71 151;n;;u 0;p 0 0 2;u 1;p 1 71 153;n;;u 0;p 0 0 0;u 1;p 1 71 151;n;u 2;p 2 32 165;u 3;p 3 260 165;;u 0;p 0 0 2;u 1;p 1 71 153;n;;u 0;p 0 0 0;u 1;p 1 71 151;n;;u 0;p 0 0 2;u 1;p 1 71 153;n;u 2;u 3;n");

				cntr_c=0;
				
				ani_frame(test_ani, tmpd);
			}
			
			
			if (cntr_c==0 && fader_cnt==0)
				cntr_c=1;
			
			if (cntr_c==1)
				ani_frame(test_ani, tmpd);
				
			if (test_ani->done && cntr_c==1)
				cntr_c=2;
			
			if (cntr_c==2)
			{
				
				if (((++cntr_b) % (FPS)) < (FPS/2))
					render_text(
						tmpd,
						&font_default,
						&font_outline,
						font,
						"Press Start",
						(SWIDTH/2),
						SHEIGHT-40,
						TEXT_OUTLINE|TEXT_BOLD|TEXT_CENTERED|TEXT_BACKING
						);
				else
					render_text(
						tmpd,
						&font_default,
						&font_outline,
						font,
						"           ",
						(SWIDTH/2),
						SHEIGHT-40,
						TEXT_OUTLINE|TEXT_BOLD|TEXT_CENTERED|TEXT_BACKING
						);
					
			}
			
			if (cntr_c==2 && (ctlr_main.inv||ctlr_main.jump))
			{
				SET_FADE_OUT();
				cntr_c++;
			}
			
			if (cntr_c==3 && fader_cnt==255)
			{
				/* setup death match mode START*/
				
				select_world = &test_world;
				
				nplayers = 1;
				
				player_focus = 0;
				
				for (i=0;i<16;i++)
				{
					players[i].chara=0;
					players[i].type=0;
				}
				
				players[0].type=1;
				
				/* directly convert all chara templates in world to chara actives in c_active (linked-list)*/
				c_active = 0;
				c_a_last = 0;
				
				#define ADD_CHARA_ACTIVE(a) \
					if (!c_active) \
					{ \
						c_active = a; \
						c_a_last=a; \
					} \
					else if (a) \
					{ \
						c_a_last->next = a; \
						c_a_last = a; \
					}
				
				{
					int tmp_player_assgn = 0;
					room *rmtmp;
					placer *pltmp;
					chara_active *catmp;
					chara_template *cattmp;
					
					/* only use room 0 for now. */
					rmtmp = select_world->rooms;
					
					/* for each room */
					
					pltmp = rmtmp->placers;
					/* for each placer in room */
					
					while (pltmp)
					{

						if (pltmp->type == PLACER_PLAYER && tmp_player_assgn < nplayers)
						{	
							catmp = new_chara_active(get_chara_template(&test_world,"ch0"));
							catmp->md = pltmp->c_start_mode;
							catmp->pos.x = pltmp->pos.x;
							catmp->pos.y = pltmp->pos.y;
							catmp->pos.z = pltmp->pos.z;
							
							ADD_CHARA_ACTIVE(catmp);
							
							players[tmp_player_assgn].chara = catmp;
							
							if (tmp_player_assgn == player_focus)
								testcam.target = catmp;
							
							tmp_player_assgn++;
							
						}
						else if (pltmp->type == PLACER_NPC)
						{	
							cattmp = get_chara_template(&test_world,pltmp->c_name);
							
							if (cattmp)
							{
								catmp = new_chara_active(cattmp);
								catmp->md = pltmp->c_start_mode;
								catmp->pos.x = pltmp->pos.x;
								catmp->pos.y = pltmp->pos.y;
								catmp->pos.z = pltmp->pos.z;
							
								ADD_CHARA_ACTIVE(catmp);
							}
							else
							{
								printf("couldn't find chara \"%s\"\n", pltmp->c_name);
							}
						}

						/* for exits, use c_name for dst room name */

						pltmp=pltmp->next;
					}
				}
				
				/* set up room */
				
				/* assign tilemap ptrs */
				tmap_main_select = select_world->rooms->tm_main;
				
				
				/* select bgm */
				
				/*END*/
				
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
				
			clear_render_sprites(&rstest);
			
			
			catmp = c_active;
			while (catmp)
			{
				switch (catmp->base->type)
				{
				case 0: /* CH0 */
					j=catmp->md;
					catmp->md_changed = 0;
					
					if ((catmp->md == CH0_MD_STAND_U || 
						 catmp->md == CH0_MD_STAND_D || 
						 catmp->md == CH0_MD_STAND_L || 
						 catmp->md == CH0_MD_STAND_R || 
						 catmp->md == CH0_MD_WALK_U  || 
						 catmp->md == CH0_MD_WALK_D  || 
						 catmp->md == CH0_MD_WALK_L  || 
						 catmp->md == CH0_MD_WALK_R)  &&
						 players[player_focus].chara == catmp && 
						cntr_c <= 0  )
					{
						if (ctlr_main.up)
							catmp->md = CH0_MD_WALK_U;
						if (ctlr_main.down)
							catmp->md = CH0_MD_WALK_D;
						if (ctlr_main.left)
							catmp->md = CH0_MD_WALK_L;
						if (ctlr_main.right)
							catmp->md = CH0_MD_WALK_R;
						
						if (!ctlr_main.up &&
							!ctlr_main.down &&
							!ctlr_main.left &&
							!ctlr_main.right)
							switch (catmp->md)
							{
							case CH0_MD_WALK_U: catmp->md=CH0_MD_STAND_U;break;
							case CH0_MD_WALK_D: catmp->md=CH0_MD_STAND_D;break;
							case CH0_MD_WALK_L: catmp->md=CH0_MD_STAND_L;break;
							case CH0_MD_WALK_R: catmp->md=CH0_MD_STAND_R;break;
							}
						
						if (ctlr_main.tap_attk_a)
						{
							if (catmp->md == CH0_MD_WALK_U || catmp->md == CH0_MD_STAND_U)
								catmp->md = CH0_MD_ATTK_U;
							else if (catmp->md == CH0_MD_WALK_D || catmp->md == CH0_MD_STAND_D)
								catmp->md = CH0_MD_ATTK_D;
							else if (catmp->md == CH0_MD_WALK_L || catmp->md == CH0_MD_STAND_L)
								catmp->md = CH0_MD_ATTK_L;
							else if (catmp->md == CH0_MD_WALK_R || catmp->md == CH0_MD_STAND_R)
								catmp->md = CH0_MD_ATTK_R;
							catmp->cntr[0] = 20;
						}
					}
					else if (catmp->md == CH0_MD_ATTK_U)
					{
						if (--catmp->cntr[0] <= 0)
							catmp->md=CH0_MD_STAND_U;
					}
					else if (catmp->md == CH0_MD_ATTK_D)
					{
						if (--catmp->cntr[0] <= 0)
							catmp->md=CH0_MD_STAND_D;
					}
					else if (catmp->md == CH0_MD_ATTK_L)
					{
						if (--catmp->cntr[0] <= 0)
							catmp->md=CH0_MD_STAND_L;
					}
					else if (catmp->md == CH0_MD_ATTK_R)
					{
						if (--catmp->cntr[0] <= 0)
							catmp->md=CH0_MD_STAND_R;
					}
					
					catmp->u_sprt = 0;
						
					switch(catmp->md)
					{case CH0_MD_STAND_U: catmp->u_sprt = catmp->gfx[5]; break;
					case CH0_MD_STAND_D:  catmp->u_sprt = catmp->gfx[7]; break;
					case CH0_MD_STAND_L:  catmp->u_sprt = catmp->gfx[1]; break;
					case CH0_MD_STAND_R:  catmp->u_sprt = catmp->gfx[3]; break;
					case CH0_MD_WALK_U:   catmp->u_sprt = catmp->gfx[4]; break;
					case CH0_MD_WALK_D:   catmp->u_sprt = catmp->gfx[6]; break;
					case CH0_MD_WALK_L:   catmp->u_sprt = catmp->gfx[0]; break;
					case CH0_MD_WALK_R:   catmp->u_sprt = catmp->gfx[2]; break;
					case CH0_MD_ATTK_U:   catmp->u_sprt = catmp->gfx[8]; break;
					case CH0_MD_ATTK_D:   catmp->u_sprt = catmp->gfx[9]; break;
					case CH0_MD_ATTK_L:   catmp->u_sprt = catmp->gfx[10]; break;
					case CH0_MD_ATTK_R:   catmp->u_sprt = catmp->gfx[11]; break;}
					
					if (catmp->invisi_cntr>0)
						catmp->invisi_cntr--;
						
					if (j!=catmp->md)
						catmp->md_changed = 1;
					
					break;
					
				case 1: /* jar */
					
					for (i=0;i<32;i++)
						catmp->af_hit[i] = 0;
					
					catmp->af_hit_cnt = 0;
					
					switch(catmp->md)
					{
					case 0:
						if (catmp->invisi_cntr <= 0 && catmp->md==0)
						{
							action_frame_check_if_hit(&aframe_list, catmp);
							
							if (catmp->af_hit_cnt > 0)
							{
								/* apply drift effect if hit by any action frames */
								chara_active_af_apply_drift(catmp);
								
								catmp->invisi_cntr = FPS / 2;
								
								catmp->md++; /* step mode to deactivate */
							}
						}
						break;
					case 1:
						if (catmp->invisi_cntr <= 0)
							catmp->active = 0;
						break;
					}
					
					catmp->u_sprt = catmp->gfx[0];
					
					if (catmp->invisi_cntr>0)
						catmp->invisi_cntr--;
					
					break;
				}
				
				
				/* add sprite */
				if (catmp->u_sprt && catmp->active )
				{
					/* mode changed? reset sprite animation */
					if (catmp->md_changed)
					{
						reset_sprite_active(catmp->u_sprt);
						
						/* clear all action frames associated with active chara */
						action_frame_clear_chara(&aframe_list, catmp);
					}
					
					/* apply sprite drift at frame to dpos */
					apply_dpos_chara_sprite_active(catmp, catmp->u_sprt);
					
					/* apply drift effect (should be only be used for props )*/
					chara_active_apply_drift(catmp);
					
					/* apply dpos to pos, while clipping dpos if another
					 * chara or b_blocker or block tile in way.
					 */
					chara_active_apply_dpos_clip(catmp, catmp, tmap_main_select);
					
					/* tick action frame for sprite */
					active_chara_sprite_tick_action_frame(&aframe_list, catmp, catmp->u_sprt);
					
					if (!(catmp->invisi_cntr > 0 && tick_shad<0))
						/* add sprite to render list as a render sprite */
						add_sprite_auto_shadow(
							&rstest,
							catmp->u_sprt,
							0,  
							&testcam, 
							catmp->pos.x, 
							catmp->pos.y, 
							catmp->pos.z);
					
					
					/* step sprite */
					step_sprite_active(catmp->u_sprt);
				}
				
				
				catmp=catmp->next;
			}
			
			/* start render process: */
		
			/* clear framebuffers */
			SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0,0,0,255 ) );
			
			/* render tile map */
			render_tilemap(tmpd, tmap_main_select, &testcam);
		
			SDL_FillRect( s_layer, 0, SDL_MapRGBA( s_layer->format, 0, 0, 0, 0 ) );
			
			/* render sprites from render sprite list */
			render_rsprite_list(tmpd, s_layer, &rstest, tick_shad);
			
			render_hud_health_left(tmpd, sprt_lhud, ch0_health, 1.0, &font_default, &font_outline, font, "Test Character");

			if (debug_f[DEBUG_SHOW_ACTION_FRAMES])
				show_action_frames(&aframe_list, &testcam, tmpd);
			
			/* tick flicker shadow effect */
			switch(tick_shad)
			{
			case -1:tick_shad=0;break;
			case 0:tick_shad=1;break;
			case 1:tick_shad=-1;break;
			}
			
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
		
		SDL_BlitSurface(tmpd,0 , main_display, 0);

		SDL_SetSurfaceAlphaMod( tmpd, 255-fader_cnt );

		SDL_FillRect( main_display, 0, SDL_MapRGBA( main_display->format, 0, 0, 0, 255 ) );
		
		SDL_BlitScaled(tmpd,0,main_display,0);
		
		SDL_UpdateWindowSurface(win);
		
		tick_frame(FPS);
		
		
	}
	
	/* terminate things: */
	
	SDL_DestroyWindow(win);
	IMG_Quit();
	SDL_Quit();
	
	return 0;
}
