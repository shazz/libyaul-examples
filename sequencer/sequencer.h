/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SCENE_H
#define SCENE_H

#define DEBUG

/*
 * typical dummy usage
 * scene_init();
 *
 * scene_register("title", 0, 300, -1, title_init, title_update, title_draw, title_exit);
 * scene_register("tutorial", -1, -1, -1, tutorial_init, tutorial_update, tutorial_draw, tutorial_exit);
 * scene_register("game", -1, -1, -1, game_init, game_update, game_draw, game_exit);
 * scene_register("game-over", -1, -1, 180, game_over_init, game_over_update, game_over_draw, game_over_exit);
 *
 * scene_load("title");
 *
 * while (true) {
 *      vdp2_tvmd_vblank_in_wait();
 *      scene_update();
 *      vdp2_tvmd_vblank_out_wait();
 *      scene_draw();
 * }
 */

/*
 * void sequencer_initialize(void)
 *
 * Initialize the seq engine
 */
void sequencer_initialize(void);

/*
 * void sequencer_stop(void)
 *
 * Stop the sequencer
 */
void sequencer_stop(void);

/*
 * void sequencer_update(void)
 *
 * update the seq engine
 */
void sequencer_update(uint32_t timer);

/*
 * int scene_register(const char *name, uint32_t, uint32_t, uint32_t,void (*init)(void), void (*update)(void), void (*draw)(void), void (*exit)(void))
 * parameters:
 * - name: scene name, should be unique
 * - start_time: if not user driven, start time in VBL occurences since beginning. -1 to discard
 * - end_time: if not user driven, end time in VBL occurences since beginning.  -1 to discard
 * - duratrion: if not user driven, durartion time  in VBL occurences since previous scene.  -1 to discard
 * - init function: function ptr to the scene init
 * - update function: function ptr to the scene update
 * - draw function: function ptr to the scene draw
 * - exit function: function ptr to the scene exit
 * returns: -1 if duplicate
 *
 *
 * Register a new scene in the queue
 */
int sequencer_register(const char *, uint32_t, uint32_t, uint32_t, void (*)(void), void (*)(void), void (*)(void), void (*)(void));

/*
 * void sequencer_start()
 *
 * load the first scene if no scene loaded and starts
 */
void sequencer_start();

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
 * void scene_init(void)
 *
 * init a scene
 */
void scene_init(void);

/*
 * void scene_update(void)
 *
 * update a scene
 */
void scene_update(void);

/*
 * void scene_draw(void)
 *
 * draw a scene
 */
void scene_draw(void);

/*
 * void scene_exit(void)
 *
 * exit a scene
 */
void scene_exit(void);

#endif /* !SCENE_H */