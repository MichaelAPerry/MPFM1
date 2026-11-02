/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#include "plat.h"
#define APP_NAME "STAMP"
#define APP_SPLASH_RGB RGB(255,190,70)
#define APP_BLOCK 256u
#define APP_CANVAS_PX (240u*112u)
enum { OBJ_SONG0, OBJ_SONG1, OBJ_SONG2, OBJ_SONG3, OBJ_SONG4, OBJ_SONG5, OBJ_SONG6, OBJ_SONG7, OBJ_PANEL, OBJ_AUTO, OBJ_NOBJ };
#define APP_STORE_MAP {0x97000u,0x99000u,0x9B000u,0x9D000u,0x9F000u,0xA1000u,0xA3000u,0xA5000u,0xDC000u,0xA7000u}
#define APP_BACKUP_OBJS {OBJ_SONG0,OBJ_SONG1,OBJ_SONG2,OBJ_SONG3,OBJ_SONG4,OBJ_SONG5,OBJ_SONG6,OBJ_SONG7}
#define APP_BACKUP_MAGIC 0x504D5453u /* "STMP" */
#define APP_BACKUP_VERSION 1u
#define APP_BACKUP_WHAT "replaces all songs"
int app_backup_playing(void);
void app_backup_flush(void);
void app_backup_hold(int);
void app_backup_restored(void);
void app_init(void);
void app_render(int32_t *,uint32_t,uint32_t);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
#define APP_REDRAW
void app_redraw(void);
void app_say(const char *,const char *);
#define APP_HOST_EXPECT
int app_expect(const char *,int *);


#ifdef PLAT_HOST
#define APP_HOST_CMD
int app_host_cmd(const char *,const char *,const char *,const char *,const char *);
#endif
