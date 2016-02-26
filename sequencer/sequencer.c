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
        uint32_t start_time;
        uint32_t end_time;
        uint32_t duration;
        void (*init)(void);
        void (*update)(void);
        void (*draw)(void);
        void (*exit)(void);
        bool initialized;

        TAILQ_ENTRY(scene) entries;
};

static struct scenes scenes;
static struct scene * current_scene;

static bool isStarted = false;
static bool isInit = false;

#ifdef DEBUG
struct cons cons;
static char * consbuf;
int nbOfScenesShown = 1;
#endif  

/*
 * 
 * Sequencer functions
 * 
 */
 
void
sequencer_initialize(void)
{
    if(isInit == false) {
        TAILQ_INIT(&scenes);
        isInit = true;
    }
        
#ifdef DEBUG   
    /* CONS */ 
    cons_init(&cons, CONS_DRIVER_VDP2);  
    consbuf = (char *)malloc(1024); 
#endif       
}

void 
sequencer_update(uint32_t timer)
{
    if(isStarted == false) return;
    
    if(current_scene != NULL)
    {
#ifdef DEBUG
        (void)sprintf(consbuf, "[01;2HFrame Counter : %08lu", timer); 
        cons_write(&cons, consbuf);
        
        (void)sprintf(consbuf, "[02;2HScene shown : %08d", nbOfScenesShown); 
        cons_write(&cons, consbuf);        
        
        (void)sprintf(consbuf, "[03;2HScene : %s", current_scene->name); 
        cons_write(&cons, consbuf);    
        
        (void)sprintf(consbuf, "[04;2HStart : %08lu", current_scene->start_time); 
        cons_write(&cons, consbuf);      

        (void)sprintf(consbuf, "[05;2HEnd : %08lu", current_scene->end_time); 
        cons_write(&cons, consbuf);   
        
        (void)sprintf(consbuf, "[06;2HDuration : %08lu", current_scene->duration); 
        cons_write(&cons, consbuf);
#endif      

        if(timer < current_scene->start_time ) 
            return;
        
        if(timer > current_scene->end_time)
        {
#ifdef DEBUG            
            nbOfScenesShown++;
            cons_write(&cons, "[10;2HMoving to next scene"); 
#endif              
            sequencer_load_next();
            
        }
#ifdef DEBUG         
        else
        {
            cons_write(&cons, "[10;2HNot moving            "); 
        }    
#endif         
    }
}

void sequencer_stop(void)
{
    isStarted = false;
#ifdef DEBUG      
    free(consbuf);
#endif     
}


int
sequencer_register(const char *name, uint32_t start_time, uint32_t end_time, uint32_t duration,
    void (*init)(void), void (*update)(void),
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
        scene->start_time = start_time;
        scene->end_time = end_time;
        scene->duration = duration;
        scene->init = init;
        scene->update = update;
        scene->draw = draw;
        scene->exit = exit;
        scene->initialized = false;

        TAILQ_INSERT_TAIL(&scenes, scene, entries);
        
        //current_scene = TAILQ_FIRST(&scenes);

        return 0;
}

void
sequencer_load(const char *name)
{
        struct scene *scene;

        TAILQ_FOREACH (scene, &scenes, entries) {
                if ((strcmp(scene->name, name)) == 0) {
                        current_scene = scene;
                        return;
                }
        }
}

void
sequencer_start()
{
    if(isStarted == false) {
        if(current_scene == NULL) {
            current_scene = TAILQ_FIRST(&scenes);
        }
        isStarted = true;       
    }
}

bool sequencer_isStarted()
{
    return isStarted;
}

void
sequencer_load_next(void)
{
        struct scene *scene;

        scene = TAILQ_NEXT (current_scene, entries);
       	if (scene != NULL) {
			current_scene = scene;
			return;
        }
}

/*
 * 
 * Scene functions
 * 
 */
 
void
scene_init(void)
{
	if(current_scene->initialized == false) {       
		current_scene->init();
		current_scene->initialized = true;
	}

}

void
scene_update(void)
{
	current_scene->update();
}

void
scene_draw(void)
{
#ifdef DEBUG       
    cons_flush(&cons);    
#endif     
	current_scene->draw();
}

void
scene_exit(void)
{
	if(current_scene->initialized == true) {          
		current_scene->exit();
		current_scene->initialized = false;
	}
}



