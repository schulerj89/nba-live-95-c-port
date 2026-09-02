#include "nba_tipoff.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint8_t tile_pixel(const uint8_t *tile,int x,int y){int bit=7-x;return(uint8_t)(
    ((tile[y*2]>>bit)&1)|(((tile[y*2+1]>>bit)&1)<<1)|
    (((tile[16+y*2]>>bit)&1)<<2)|(((tile[17+y*2]>>bit)&1)<<3));}

int main(int argc,char **argv) {
    NbaAssetPack pack={0};NbaSession session;NbaTipoff game;NbaInput input={0};
    int16_t sx=0,sy=0;
    /* Native `$87:A3BB-$A3DC` witnesses, including negative remainders. */
    nba_court_project_actor(8,3,0,-128,-124,&sx,&sy);
    if(sx!=139 || sy!=122)return 13;
    nba_court_project_actor(-16,-83,0,-128,-124,&sx,&sy);
    if(sx!=29 || sy!=107)return 14;
    nba_court_project_actor(71,-33,0,-174,-144,&sx,&sy);
    if(sx!=212 || sy!=118)return 15;
    nba_court_project_actor(-62,12,7,-174,-144,&sx,&sy);
    if(sx!=124 || sy!=155)return 16;
    if(!nba_court_actor_visible(-20,-20,0,false) ||
       !nba_court_actor_visible(275,287,0,false) ||
       !nba_court_actor_visible(100,300,20,false) ||
       nba_court_actor_visible(100,300,12,false) ||
       nba_court_actor_visible(-21,0,0,false) ||
       nba_court_actor_visible(276,0,0,false))return 17;
    if(!nba_court_actor_visible(11,11,0,true) ||
       !nba_court_actor_visible(244,217,0,true) ||
       nba_court_actor_visible(10,100,0,true) ||
       nba_court_actor_visible(245,100,0,true))return 18;
    if(argc<2 || !nba_assets_load(&pack,argv[1]))return 2;
    nba_session_init(&session);
    if(!nba_tipoff_init(&game,&pack,&session))return 3;
    for(unsigned frame=1;frame<=16000;++frame) {
        NbaCourtPresentation previous=game.court_presentation;
        NbaCourtStream stream=game.court_stream;
        nba_tipoff_update(&game,&input);
        if(game.camera.presentation_ticks_0564==0) {
            nba_court_presentation_update(&previous,game.camera_x,game.camera_y,
                game.period_raw_0926,game.team_context[0].anchor_x_raw_0a,game.team_context[1].anchor_x_raw_0a);
            if(memcmp(&previous,&game.court_presentation,sizeof(previous)))return 4;
            if(!nba_court_stream_update(&stream,&pack,game.camera_x,game.camera_y,
                game.camera.previous_x,game.camera.previous_y,NULL,NULL))return 5;
            stream.scroll_x=stream.next_scroll_x;stream.scroll_y=stream.next_scroll_y;
            if(memcmp(&stream,&game.court_stream,sizeof(stream)))return 6;
        } else if(memcmp(&stream,&game.court_stream,sizeof(stream)))return 7;
        if(game.court_stream.source!=(uint16_t)(0x8006+game.camera.coarse_x*104+game.camera.coarse_y*2))return 8;
    }
    /* Render the actual port with actors/ball outside the viewport. This is
     * an isolated renderer fixture, not a natural-gameplay claim. */
    for(unsigned a=0;a<10;++a){
        game.actors[a].x_fp=10000*256;game.actors[a].y_fp=10000*256;
        /* The fixture mutates world state outside an update pass. Native OAM
         * keeps its old submissions, so explicitly hide the latched sprites
         * when validating the panorama alone. */
        game.player_screen_visible[a]=false;
    }
    game.ball.x_fp=10000*256;game.ball.y_fp=10000*256;
    game.frame=989;
    /* Isolate the static map/CHR compositor from the sampled fan overlay.
     * Different native home captures carry different crowd phases. The
     * button/Tipoff frame smoke keeps the real overlay enabled throughout. */
    NbaAssetPack static_court_pack=pack;
    for(unsigned i=0;i<static_court_pack.item_count;++i)
        if(static_court_pack.items[i].id==NBA_ASSET_GAMEPLAY_CROWD_TILES)
            static_court_pack.items[i].size=0;
    game.assets=&static_court_pack;
    NbaRenderer renderer;nba_renderer_init(&renderer);
    const int xs[]={-582,-128,74,75,135,194,328},ys[]={-242,-220,-124,-53};
    uint64_t verified_bg2=0,verified_backdrop=0,verified_bg3=0,verified_bg1=0;
    for(unsigned team=0;team<29;++team) {
        const NbaAssetItem *map_item=nba_assets_gameplay_court_map(&pack,(uint8_t)team);
        if(!map_item || map_item->size!=15398u)return 9;
        const uint8_t *map=(const uint8_t *)map_item->data;
        session.right_team=(uint8_t)team;
        /* Isolated post-initialization panorama fixture: the renderer reads
         * the native home context's $00 identity. Changing only the session
         * selection no longer changes that already initialized match state. */
        game.team_context[0].strategy_team_raw_00=(uint16_t)team;
        const uint8_t *vram=NULL,*cgram=NULL;
        if(!nba_assets_gameplay_ppu_input(&pack,(uint8_t)team,&vram,&cgram))return 9;
        for(unsigned xi=0;xi<7;++xi)for(unsigned yi=0;yi<4;++yi) {
            game.camera_x=(int16_t)xs[xi];game.camera_y=(int16_t)ys[yi];
            nba_court_presentation_update(&game.court_presentation,
                game.camera_x,game.camera_y,game.period_raw_0926,
                game.team_context[0].anchor_x_raw_0a,
                game.team_context[1].anchor_x_raw_0a);
            nba_tipoff_render(&game,&renderer);
            int px=xs[xi]+582,py=ys[yi]+243;
            for(int y=0;y<224;++y)for(int x=0;x<256;++x){
                int world_x=px+x,world_y=py+y;
                uint16_t entry=u16(map+6+((world_x>>3)*52+(world_y>>3))*2);
                int tx=world_x&7,ty=world_y&7;
                if(entry&0x4000)tx=7-tx;
                if(entry&0x8000)ty=7-ty;
                uint8_t color=tile_pixel(vram+0x4000+(entry&0x3ff)*32,tx,ty);
                NbaSnesMode1Pixel pixel;
                if(!nba_snes_mode1_pixel(&renderer,x,y,&pixel))return 10;
                if(pixel.layer==NBA_SNES_LAYER_BG2){
                    uint8_t palette=(uint8_t)(((entry>>10)&7)*16+color);
                    if(!color || pixel.color_index!=color ||
                       pixel.palette_index!=palette ||
                       pixel.priority!=((entry>>13)&1) ||
                       pixel.argb!=nba_snes_cgram_color(cgram,palette,15,0,0,0)) {
                        fprintf(stderr,"COURT pixel mismatch home=%u camera=(%d,%d) pixel=(%d,%d) tile=%u index=%u/%u palette=%u/%u argb=%08x/%08x\n",
                            team,xs[xi],ys[yi],x,y,entry&0x3ffu,pixel.color_index,color,
                            pixel.palette_index,palette,pixel.argb,
                            nba_snes_cgram_color(cgram,palette,15,0,0,0));
                        return 10;
                    }
                    ++verified_bg2;
                } else if(pixel.layer==NBA_SNES_LAYER_BACKDROP){
                    if(color || pixel.argb!=nba_snes_cgram_color(cgram,0,15,0,0,0))return 10;
                    ++verified_backdrop;
                } else if(pixel.layer==NBA_SNES_LAYER_BG3)++verified_bg3;
                else if(pixel.layer==NBA_SNES_LAYER_BG1)++verified_bg1;
            }
            if(argc>=3 && team==18 && xs[xi]==135 && ys[yi]==-220 &&
               !nba_renderer_save_bmp(&renderer,argv[2]))return 19;
        }
    }
    if(!verified_bg2 || !verified_backdrop || !verified_bg3 || !verified_bg1)return 20;
    /* 8E28 loads current period 0926, NOT quarter-length setting 1701.
     * Give them opposite half selections to catch a wrong caller binding.
     * These are isolated integration scenarios, not natural period flow. */
    for(unsigned period=0;period<4;++period) {
        session.config.main_values[3]=(uint16_t)(period<2?3:0);
        if(!nba_tipoff_init(&game,&pack,&session))return 11;
        game.period_raw_0926=(uint16_t)period;
        for(unsigned frame=0;frame<300;++frame)nba_tipoff_update(&game,&input);
        unsigned side=(period<2 ? game.camera_x>=0 : game.camera_x<0)?1u:0u;
        if(game.court_presentation.basket_x_3fef!=
           (uint16_t)game.team_context[side].anchor_x_raw_0a)return 12;
    }
    printf("COURT runtime PASS:16000 caller frames;812 indexed viewports across29 teams; BG2=%llu BG3=%llu BG1=%llu backdrop=%llu;4 period scenarios\n",
        (unsigned long long)verified_bg2,(unsigned long long)verified_bg3,
        (unsigned long long)verified_bg1,(unsigned long long)verified_backdrop);
    nba_assets_free(&pack);return 0;
}
