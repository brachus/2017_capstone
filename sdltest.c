#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SWIDTH 300
#define SHEIGHT 200

#define MAXSPRITES 256


void tick_frame(int fps)
{
	static unsigned int prevtime;
	static int first = 1;
	if (!first)
		SDL_Delay( (1000/fps) - (SDL_GetTicks() - prevtime) );
	else
		first = 0;
	prevtime = SDL_GetTicks();
}

struct sprite
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
};


struct render_sprite
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
};

struct render_sprite_head
{
	int shad_cnt;
	int sprt_cnt;
	struct render_sprite *next;
};


void clear_render_sprites(struct render_sprite_head *in)
{
	struct render_sprite *tmpa, *tmpb;
	
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
	struct render_sprite_head *in,
	struct sprite *sprt,
	struct sprite *shad,
	int x,
	int y,
	int z  )
{
	int i,rx,ry,bry,sx,sy;
	
	if (z<0)
		return;
	
	rx = (SWIDTH/2) + x;
	ry = (SHEIGHT/2) - y - z;
	bry = (SHEIGHT/2) - y;
	
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
		a = (struct render_sprite*) malloc(sizeof(struct render_sprite));  \
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
		struct render_sprite *tmp, *tmpprev;
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
		sx = (SWIDTH/2) + x;
		sy = (SHEIGHT/2) - y;
		
		{
			struct render_sprite *tmp;
			
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


int main(void)
{
	/* initialize things: */

	SDL_Event e;
	SDL_Surface *sprt_test, *sprt_shadow, *main_display, *tmpd;
	SDL_Window  *win;
	SDL_Rect 	pos;
	SDL_Rect 	pos0;
	
	pos0.x = pos0.y = 0;
	
	struct sprite apple,  shad;
	
	struct render_sprite_head rstest;
	
	clear_render_sprites(&rstest);
	
	int i;
	
	enum
	{
		_CH_LEFT_GO,
		_CH_LEFT_REST,
		_CH_RIGHT_GO,
		_CH_RIGHT_REST,
		_CH_UP_GO,
		_CH_UP_REST,
		_CH_DOWN_GO,
		_CH_DOWN_REST,
		_CH_LEN
	};
	
	struct sprite ch_dat[_CH_LEN];
	
	char ch_dir = 'd', ch_dir_prev = 'd', ch_cur_sprt = _CH_LEFT_REST;
	
	#define SPRITE_SET(a,b,c,d,e,f,g,h)  \
		a[b].cx = c;  \
		a[b].cy = d;  \
		a[b].transp = e;  \
		a[b].frame_cnt = f;  \
		a[b].frame_cur = 0;  \
		a[b].sprt_arr = (SDL_Surface**) malloc( sizeof (SDL_Surface*) * f );  \
		a[b].intrv=g;  \
		a[b].loop=h;  \
		a[b].cnt0=0;
	
	#define SPRITE_LOAD_IMAGE(a,b,c,d)  \
		a[b].sprt_arr[c] = IMG_Load(d);
	
	SPRITE_SET(ch_dat,_CH_LEFT_GO, 16,28, 0, 2, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_LEFT_GO,0,"l_0.png");
	SPRITE_LOAD_IMAGE(ch_dat,_CH_LEFT_GO,1,"l_1.png");
	
	SPRITE_SET(ch_dat,_CH_LEFT_REST, 16,28, 0, 1, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_LEFT_REST,0,"l_0.png");
	
	SPRITE_SET(ch_dat,_CH_RIGHT_GO, 16,28, 0, 2, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_RIGHT_GO,0,"r_0.png");
	SPRITE_LOAD_IMAGE(ch_dat,_CH_RIGHT_GO,1,"r_1.png");
	
	SPRITE_SET(ch_dat,_CH_RIGHT_REST, 16,28, 0, 1, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_RIGHT_REST,0,"r_0.png");
	
	SPRITE_SET(ch_dat,_CH_UP_GO, 16,28, 0, 2, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_UP_GO,0,"u_0.png");
	SPRITE_LOAD_IMAGE(ch_dat,_CH_UP_GO,1,"u_1.png");
	
	SPRITE_SET(ch_dat,_CH_UP_REST, 16,28, 0, 1, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_UP_REST,0,"u_0.png");
	
	SPRITE_SET(ch_dat,_CH_DOWN_GO, 16,28, 0, 2, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_DOWN_GO,0,"d_0.png");
	SPRITE_LOAD_IMAGE(ch_dat,_CH_DOWN_GO,1,"d_1.png");
	
	SPRITE_SET(ch_dat,_CH_DOWN_REST, 16,28, 0, 1, 20,1);
	SPRITE_LOAD_IMAGE(ch_dat,_CH_DOWN_REST,0,"d_0.png");
	
	
	
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		printf("SDL init failed...\n");
		return 1;
	}
	
	IMG_Init(IMG_INIT_PNG);
	
	
	
	
	/* create things: */


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
	
	
	/* load image data */
	sprt_test = IMG_Load("apple.png");
	sprt_shadow = IMG_Load("shad.png");
	
	/* create apple sprite */
	apple.cx=7;apple.cy=10;
	apple.transp=0;
	apple.frame_cnt=1;
	apple.frame_cur=0;
	apple.sprt_arr = (SDL_Surface**) malloc( sizeof (SDL_Surface*) );
	apple.sprt_arr[0] = sprt_test;
	apple.intrv=0;
	apple.loop=0;
	
	/* create shadow sprite*/
	shad.cx=8;shad.cy=2;
	shad.transp=1;
	shad.frame_cnt=1;
	shad.frame_cur=0;
	shad.sprt_arr = (SDL_Surface**) malloc( sizeof (SDL_Surface*) );
	shad.sprt_arr[0] = sprt_shadow;
	shad.intrv=0;
	shad.loop=0;
	
	
	int k_w=0,k_a=0,k_s=0,k_d=0,k_zup=0,k_zdown=0;
	
	int tmpx,tmpy,tmpz;
	tmpx=tmpy=tmpz=1;
	
	int tick_shad = -1;
	
	int run=1;

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
				case SDLK_w:k_w=1;break;
				case SDLK_a:k_a=1;break;
				case SDLK_s:k_s=1;break;
				case SDLK_d:k_d=1;break;
				case SDLK_UP:k_zup=1;break;
				case SDLK_DOWN:k_zdown=1;break;
				case SDLK_ESCAPE:run=0;break;
				}
				break;
			case SDL_KEYUP:
				switch(e.key.keysym.sym)
				{
				case SDLK_w:k_w=0;break;
				case SDLK_a:k_a=0;break;
				case SDLK_s:k_s=0;break;
				case SDLK_d:k_d=0;break;
				case SDLK_UP:k_zup=0;break;
				case SDLK_DOWN:k_zdown=0;break;
				}
				break;
			}
			
		if (k_w){tmpy++;ch_dir='u';}
		if (k_a){tmpx--;ch_dir='l';}
		if (k_s){tmpy--;ch_dir='d';}
		if (k_d){tmpx++;ch_dir='r';}
		if (k_zdown){tmpz--;}
		if (k_zup){tmpz++;}
		
		
		SDL_FillRect( tmpd, 0, SDL_MapRGBA( tmpd->format, 0xFF, 0xFF, 0xFF, 255 ) );
        SDL_FillRect( main_display, 0, SDL_MapRGB( main_display->format, 0xFF, 0xFF, 0xFF ) );
        
        /* determine which sprite to use depending of character direction */
        char tmp_dir_idx=_CH_LEFT_GO;
		switch(ch_dir)
		{
		case 'u':tmp_dir_idx=_CH_UP_GO;break;
		case 'd':tmp_dir_idx=_CH_DOWN_GO;break;
		case 'l':tmp_dir_idx=_CH_LEFT_GO;break;
		case 'r':tmp_dir_idx=_CH_RIGHT_GO;break;
		}
        
        /* fill example render sprites */
		
		clear_render_sprites(&rstest);
		
		add_sprite_auto_shadow(&rstest, &ch_dat[tmp_dir_idx], &shad, tmpx,tmpy,tmpz);
		
		add_sprite_auto_shadow(&rstest, &apple, &shad, -58, -50, 1);
		add_sprite_auto_shadow(&rstest, &apple, &shad, -55, -48, 1);
		add_sprite_auto_shadow(&rstest, &apple, &shad, -52, -43, 1);
		add_sprite_auto_shadow(&rstest, &apple, &shad, -59, -38, 1);
		add_sprite_auto_shadow(&rstest, &apple, &shad, -50, -33, 1);
		{
			struct render_sprite *tmp = rstest.next;
			
			while (tmp)
			{
				pos.x = tmp->x - tmp->cx;
				pos.y = tmp->y - tmp->cy;
				
				if (tmp->transp == 0 || tick_shad > 0)
					SDL_BlitSurface(tmp->sprt,0 , tmpd, &pos);
				
				tmp = tmp->next;
			}
		}
		
		tick_shad *= -1;
		
		SDL_BlitSurface(tmpd,0 , main_display, &pos0);
		SDL_UpdateWindowSurface(win);
		tick_frame(60);
	}
	
	/* terminate things: */
	
	SDL_FreeSurface(sprt_test);
	
	SDL_DestroyWindow(win);

	IMG_Quit();

	SDL_Quit();
	
	return 0;
}
