/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SEQUENCER_H
#define SEQUENCER_H

//#define DEBUG

/*
 * typical dummy usage
 * sequencer_initialize();
 * 
 * sequencer_register("title", 300, title_init, title_update, title_draw, title_exit);
 * sequencer_register("tutorial", 600, tutorial_init, tutorial_update, tutorial_draw, tutorial_exit);
 * sequencer_register("game", 9999999, game_init, game_update, game_draw, game_exit);
 * sequencer_register("game-over", 180, game_over_init, game_over_update, game_over_draw, game_over_exit);
 * 
 * sequencer_start();
 *
 * while (sequencer_isStarted()) {
 *      vdp2_tvmd_vblank_in_wait();
 *      sequencer_update(g_frame_counter);
 *      sequencer_draw();
 *      vdp2_tvmd_vblank_out_wait();
 * }
 * sequencer_exit();
 */

/*
 * void sequencer_initialize(void)
 *
 * Initialize the seq engine
 */
void sequencer_initialize(void);

/*
 * int scene_register(const char *name, uint32_t,void (*init)(void), void (*update)(void), void (*draw)(void), void (*exit)(void))
 * parameters:
 * - name: scene name, should be unique
 * - duration: if not user driven, durartion time  in VBL occurences since previous scene.
 * - init function: function ptr to the scene init
 * - update function: function ptr to the scene update
 * - draw function: function ptr to the scene draw
 * - exit function: function ptr to the scene exit
 * returns: -1 if duplicate
 *
 *
 * Register a new scene in the queue
 */
int sequencer_register(const char *, uint32_t, void (*)(void), void (*)(uint32_t), void (*)(void), void (*)(void));

/*
 * void sequencer_start()
 *
 * load the first scene if no scene loaded and starts
 */
void sequencer_start();

/*
 * void sequencer_stop(void)
 *
 * Stop the sequencer
 */
void sequencer_stop(void);

/*
 * bool sequencer_isStarted()
 *
 * return sequencer state
 */
bool sequencer_isStarted();

/*
 * void sequencer_load(const char *)
 * parameters: scene name
 *
 * load the scene found by name (if exists...)
 */
void sequencer_load(const char *);

/*
 * void sequencer_load_next(void)
 *
 * load and init the next scene in queue
 */
void sequencer_load_next(void);

/*
 * void sequencer_update(void)
 *
 * update the seq engine and scene
 */
void sequencer_update(uint32_t timer);

/*
 * void sequencer_stop(void)
 *
 * draw the current scene
 */
void sequencer_draw(void);

/*
 * void sequencer_exit(void)
 *
 * Exit the sequencer
 */
void sequencer_exit(void);

#endif /* !SEQUENCER_H */