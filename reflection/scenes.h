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

void additive_init(void);
void additive_update(uint32_t timer);
void additive_draw(void);
void additive_exit(void);

#endif /* !SCENES_H */