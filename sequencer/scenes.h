/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SCENES_H
#define SCENES_H

void logo_init(void);
void logo_update(void);
void logo_draw(void);
void logo_exit(void);


void title_init(void);
void title_update(void);
void title_draw(void);
void title_exit(void);

#ifdef DEBUG
extern struct cons cons;
#endif

#endif /* !SCENES_H */