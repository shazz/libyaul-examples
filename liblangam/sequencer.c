/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

#include "sequencer.h"

TAILQ_HEAD(scenes, scene);

struct scene {
        const char *name;
        uint32_t duration;
        void (*init)(void);
        void (*update)(uint32_t);
        void (*draw)(void);
        void (*exit)(void);
        bool initialized;

        TAILQ_ENTRY(scene) entries;
};

static struct scenes scenes;
static struct scene * current_scene;

static bool isStarted = false;
static bool isInit = false;
static uint32_t sceneTimer = 0;

#ifdef DEBUG
static char * consbuf;
#endif  
 
/*
 * void sequencer_initialize(void)
 *
 * Initialize the seq engine
 */ 
void sequencer_initialize(void)
{
    if(isInit == false) {
        TAILQ_INIT(&scenes);
        isInit = true;
    }
        
#ifdef DEBUG   
    cons_init(CONS_DRIVER_VDP2);  
    consbuf = (char *)malloc(1024); 
#endif       
}

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
int sequencer_register(const char *name, uint32_t duration,
    void (*init)(void), void (*update)(uint32_t),
    void (*draw)(void), void (*exit)(void))
{
        struct scene *scene;

        TAILQ_FOREACH (scene, &scenes, entries) {
                if ((strcmp(scene->name, name)) == 0) {
                        /* Duplicate exists */
                        return -1;
                }
        }

        scene = (struct scene *)malloc(sizeof(struct scene));
        assert(scene != NULL);

        scene->name = name;
        scene->duration = duration;
        scene->init = init;
        scene->update = update;
        scene->draw = draw;
        scene->exit = exit;
        scene->initialized = false;

        TAILQ_INSERT_TAIL(&scenes, scene, entries);

        return 0;
}

/*
 * void sequencer_start()
 *
 * load the first scene if no scene loaded and starts
 */
void sequencer_start()
{
    if(isStarted == false) {
        if(current_scene == NULL) {
            current_scene = TAILQ_FIRST(&scenes);
            
            // init first scene
            if(current_scene->initialized == false) {       
                current_scene->init();
                current_scene->initialized = true;
            }               
        }
        isStarted = true;       
    }
}


/*
 * void sequencer_stop(void)
 *
 * Stop the sequencer
 */
void sequencer_stop(void)
{
    isStarted = false;
#ifdef DEBUG      
    free(consbuf);
#endif     
}


/*
 * bool sequencer_isStarted()
 *
 * return sequencer state
 */
bool sequencer_isStarted()
{
    return isStarted;
}

/*
 * void sequencer_update(uint32_t timer)
 *
 * update the seq engine and scene
 */
void sequencer_update(uint32_t timer)
{
    if(isStarted == false) return;
    
    if(current_scene != NULL)
    { 
        if((timer - sceneTimer) >= current_scene->duration)
        {          
            sequencer_load_next();
            sceneTimer = timer;
        }
        current_scene->update(timer);
        
#ifdef DEBUG
        (void)sprintf(  consbuf, "[01;2HFrame Counter : %08lu[02;2HScene : %s[03;2HDuration : %08lu[04;2HScene timer : %08lu[05;2HPosition : %08lu", 
                        timer, current_scene->name, current_scene->duration, sceneTimer, (timer - sceneTimer));   
        cons_buffer(consbuf);  
#endif             
        
    }
}

/*
 * void sequencer_stop(void)
 *
 * draw the current scene
 */
void sequencer_draw()
{
    current_scene->draw();
#ifdef DEBUG      
    cons_flush();
#endif     
}

/*
 * void sequencer_load(const char *)
 * parameters: scene name
 *
 * load the scene found by name (if exists...)
 */
void sequencer_load(const char *name)
{
        struct scene *scene;

        TAILQ_FOREACH (scene, &scenes, entries) {
                if ((strcmp(scene->name, name)) == 0) {
                        current_scene = scene;
                        return;
                }
        }
}

/*
 * void sequencer_load_next(void)
 *
 * load and init the next scene in queue
 */
void sequencer_load_next(void)
{
        struct scene *scene;

        // finit current scene
        if(current_scene->initialized == true) {          
            current_scene->exit();
            current_scene->initialized = false;
        }    
        
        // load and init next one
        scene = TAILQ_NEXT (current_scene, entries);
       	if (scene != NULL) {
			current_scene = scene;
            current_scene->init();
        }
        else{
            sequencer_stop();
        }
}

/*
 * void sequencer_exit(void)
 *
 * Exit the sequencer
 */
void sequencer_exit(void)
{
    // bye bye
}








