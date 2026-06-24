/* ================================================================
 * NOVAPANEL — Main Header
 * ================================================================ */
#ifndef NOVAPANEL_H
#define NOVAPANEL_H

#include "lvgl.h"
#include "novapanel_theme.h"

/* ── Screens ──────────────────────────────────────────────────── */
typedef enum {
    NP_SCR_HOME = 0,
    NP_SCR_AGENDA,
    NP_SCR_MERCADO,
    NP_SCR_CASA,
    NP_SCR_ALARMES,
    NP_SCR_CLIMA,
    NP_SCR_TIMER,
    NP_SCR_CONFIG,
    NP_SCR_NOTIF,
    NP_SCR_COUNT
} np_screen_t;

/* ── App state ────────────────────────────────────────────────── */
typedef struct {
    np_screen_t current;
    bool        night_mode;
    bool        rail_open;
    bool        bt_enabled;
    int32_t     brilho;          /* 10-100 */
    int32_t     vol_system;      /* 0-100  */
    int32_t     vol_music;       /* 0-100  */
    int32_t     vol_alarm;       /* 0-100  */
    int32_t     timer_sec;       /* current countdown */
    int32_t     timer_total;     /* session length */
    bool        timer_running;
    /* IoT devices */
    bool        dev_luz_sala;
    bool        dev_luz_cozinha;
    bool        dev_luz_quarto;
    bool        dev_tomada_tv;
    bool        dev_tomada_cafe;
    bool        dev_ventilador;
    /* Alarms */
    bool        alarm_on[4];
} np_state_t;

extern np_state_t np_state;

/* ── Root objects ─────────────────────────────────────────────── */
extern lv_obj_t *np_root;
extern lv_obj_t *np_rail;
extern lv_obj_t *np_topbar;
extern lv_obj_t *np_content;
extern lv_obj_t *np_screens[NP_SCR_COUNT];

/* ── Core API ─────────────────────────────────────────────────── */
void np_init(void);
void np_navigate_to(np_screen_t screen);
void np_tick(void);                /* call every 1 s to refresh clock */

/* ── Screen builders ──────────────────────────────────────────── */
void np_screen_home(lv_obj_t *parent);
void np_screen_agenda(lv_obj_t *parent);
void np_screen_mercado(lv_obj_t *parent);
void np_screen_casa(lv_obj_t *parent);
void np_screen_alarmes(lv_obj_t *parent);
void np_screen_clima(lv_obj_t *parent);
void np_screen_timer(lv_obj_t *parent);
void np_screen_config(lv_obj_t *parent);
void np_screen_notif(lv_obj_t *parent);

#endif /* NOVAPANEL_H */
