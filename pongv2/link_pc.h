/*
 * link_pc.h - "Cable de enlace" del simulador de PC (UDP). Ver link_pc.c.
 * Solo lo usa hal_pc.c; el juego usa hal_link_send()/hal_link_recv() de hal.h.
 */
#ifndef LINK_PC_H
#define LINK_PC_H

/* Envia de una vez los bytes acumulados en este cuadro (se llama en hal_frame) */
void link_pc_flush(void);
void link_pc_close(void);

#endif
