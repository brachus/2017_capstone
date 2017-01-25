
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#define SWIDTH 320
#define SHEIGHT 240

#define TILESIZE 16

#define TILEMAPSIZE 1024
#define TILEMAPNTILES 32

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
	
	int md;

	int jump;
	
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
	
	char dir; /* uses DIR_ enums */
	char prev_dir;
} actor;

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
	
	if (z>0)
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

void render_text(SDL_Surface *dst, TTF_Font *font, char *msg, int x, int y)
{
	SDL_Rect tmppos;
	SDL_Surface *font_surf;
	SDL_Color fg = {255,255,255,255}, bg = {32,32,128,255};
	
	tmppos.x=x; tmppos.y=y;
	
	font_surf = TTF_RenderText_Solid(font, msg, bg);
	SDL_BlitSurface(font_surf,0 , dst, &tmppos);
	SDL_FreeSurface(font_surf);
	
	tmppos.x--; tmppos.y++;
	
	font_surf = TTF_RenderText_Solid(font, msg, fg);
	
	SDL_BlitSurface(font_surf,0 , dst, &tmppos);
	
	tmppos.x--;
	SDL_BlitSurface(font_surf,0 , dst, &tmppos);
	
	
	SDL_FreeSurface(font_surf);
}




int main(void)
{
	/* initialize things: */
	
	int i, j, k, actor_cnt=0,
		k_w=0, k_a=0, k_s=0,
		k_d=0,k_space=0, tick_shad = -1, run = 1,
		game_mode, cntr_a, cntr_b, cntr_c;

	/* character jump table.  this holds the "arc" for jumping.
	 * it's only half of it, and there's code below that finishes
	 * the job. */
	int ch_jump_ani_table[64] = {2,2,2,1,1,1,1,1,1,0,1,0,1,0,1,0,1,-999};
	
	SDL_Event e;
	SDL_Surface  *sprt_shadow, *main_display, *tmpd;
	SDL_Window  *win;
	SDL_Rect 	pos;
	SDL_Rect 	pos0;
	sprite apple,  sprt_shad;
	render_sprite_head rstest;
	tilemap * tmaptest;
	actor *actors, *key_wasd_cont;
	cam testcam;
	sprite *ch0_sprites_walk[4];
	sprite *ch0_sprites_stand[4];

	SDL_Color font_default = {255,255,255,255};
	SDL_Surface *font_surf;
	TTF_Font *font=0;
	
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

		printf("%d\n",ch_jump_ani_table[i]);
	}
	
	
	tmaptest->tiles[0] = IMG_Load("w0_t0.png");
	tmaptest->tiles[1] = IMG_Load("w0_t1.png");
	tmaptest->tiles[2] = IMG_Load("w0_t2.png");
	tmaptest->tiles[3] = IMG_Load("w0_t3.png");
	
	tilemap_box_modify(tmaptest, 2,0,8,0,  2);
	tilemap_box_modify(tmaptest, 2,1,8,1,  3);
	tilemap_box_modify(tmaptest, 2,2,8,8,  1);
	tilemap_box_modify(tmaptest, 9,5,14,8,  1);
	
	
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
	
	
	/* add test actor*/
	actor_cnt++;
	reset_actor(&actors[0]);
	
	actors[0].z=1;
	actors[0].x=50;
	actors[0].y=-50;
	
	testcam.target = &actors[0];
	
	
	
	/* load sprites */
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
			SDL_WINDOW_OPENGL
		);
	
	/* get window surface; create framebuffer */
	main_display = SDL_GetWindowSurface(win);
	tmpd = SDL_CreateRGBSurface(0, SWIDTH, SHEIGHT, 32, 0, 0, 0, 0);
	
	
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
	sprt_shad.sprt_arr[0] = IMG_Load("shad.png");;
	sprt_shad.intrv=0;
	sprt_shad.loop=0;
	
	game_mode = MD_LOGO;

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
			

		for (i=0;i < actor_cnt;i++)
		{
			actors[i].prev_dir = actors[i].dir;
			actors[i].md = CH_STAND;
			
		}
			
			
		if (key_wasd_cont)
		{
			
			if (k_w)
			{
				key_wasd_cont->md = CH_WALK;
				key_wasd_cont->y++;
				key_wasd_cont->dir = DIR_UP;
			}
			if (k_a)
			{
				key_wasd_cont->md = CH_WALK;
				key_wasd_cont->x--;
				key_wasd_cont->dir = DIR_LEFT;
			}
			if (k_s)
			{
				key_wasd_cont->y--;
				key_wasd_cont->md = CH_WALK;
				key_wasd_cont->dir = DIR_DOWN;
			}
			if (k_d)
			{
				key_wasd_cont->x++;
				key_wasd_cont->md = CH_WALK;
				key_wasd_cont->dir = DIR_RIGHT;
			}

			if (k_space && key_wasd_cont->jump==0)
				key_wasd_cont->jump=1;

			if (key_wasd_cont->jump)
			{
				/* jump: change z depending on values in ch_jump_ani_table */
				if (ch_jump_ani_table[(key_wasd_cont->jump)-1] != -999)
					key_wasd_cont->z +=
						ch_jump_ani_table[(key_wasd_cont->jump++)-1];
				else
					key_wasd_cont->jump=0;

			}
			
		}
		
        /* fill example render sprites */
		clear_render_sprites(&rstest);
		
		/* for each actor */
        for (i=0;i < actor_cnt;i++)
        {
			sprite *use_sprite = 0;
			
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
				add_sprite_auto_shadow(&rstest, use_sprite, &sprt_shad, &testcam, actors[i].x,actors[i].y,actors[i].z);
			
		}
		
		/* start render process: */
		
		/* clear framebuffers */
		SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0x0F, 0x0F, 0x0F, 255 ) );
        SDL_FillRect( main_display, 0, SDL_MapRGB( main_display->format, 0x0F, 0x0F, 0xFF ) );

		
		
		
		
		/* render tile map */
		render_tilemap(tmpd, tmaptest, &testcam);
		
		/* render sprites from render sprite list */
		render_rsprite_list(tmpd, &rstest, tick_shad);
		
		/* render text */
		render_text(tmpd, font, "testing", 10,10);
		
		/* tick flicker shadow effect */
		tick_shad *= -1;
		
		/* copy framebuffer to surface of window.  update window. */
		SDL_BlitSurface(tmpd,0 , main_display, &pos0);
		SDL_UpdateWindowSurface(win);
		
		tick_frame(60);
		
	}
	
	/* terminate things: */
	
	SDL_DestroyWindow(win);
	IMG_Quit();
	SDL_Quit();
	
	return 0;
}
