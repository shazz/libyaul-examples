/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SCENES_H
#define SCENES_H

void intro_init(void);
void intro_update(uint32_t timer);
void intro_draw(void);
void intro_exit(void);

void reflection_init(void);
void reflection_update(uint32_t timer);
void reflection_draw(void);
void reflection_exit(void);

#ifdef DEBUG
extern struct cons cons;
#endif

#endif /* !SCENES_H */