/*
 * link.h - Protocolo del cable de enlace (GB Link) para el modo 1 VS 1.
 *
 * Fisicamente es un UART (8N1, 115200 baudios) por los pines 2 (SO) y 3 (SI)
 * del puerto de enlace. El cable cruza SO <-> SI y une las tierras.
 *
 * Cada paquete tiene la forma:
 *
 *     0x5A | tipo | datos (largo fijo segun el tipo) | suma
 *
 *   - 0x5A: byte de sincronia (marca el inicio).
 *   - tipo: 'H' (hola), 'I' (botones del esclavo), 'S' (estado del maestro).
 *   - suma: suma de 8 bits de tipo + datos. Si no cuadra, el paquete se
 *     descarta y el receptor busca el siguiente 0x5A.
 *
 * Asi, si se pierde o se corrompe un byte (por ejemplo al conectar el cable
 * o por los mensajes de arranque del BIOS), solo se pierde ese paquete.
 */
#ifndef LINK_H
#define LINK_H

#include <stdint.h>

#define LINK_SYNC      0x5A

#define LINK_HOLA      'H'   /* busco pareja: datos = id aleatorio (2 bytes)        */
#define LINK_BOTONES   'I'   /* esclavo -> maestro: datos = secuencia, botones       */
#define LINK_ESTADO    'S'   /* maestro -> esclavo: datos = todo lo que se dibuja    */

#define LINK_LEN_HOLA     2
#define LINK_LEN_BOTONES  2
#define LINK_LEN_ESTADO   10
#define LINK_MAX_DATOS    10

void    link_reset(void);
/* Envia un paquete del tipo indicado (el largo sale del tipo) */
void    link_send(uint8_t tipo, const uint8_t *datos);
/* Lee los bytes que hayan llegado. Si se completa un paquete valido lo copia
 * en 'datos' y devuelve su tipo; si no, devuelve 0. Llamarlo en un while
 * hasta que devuelva 0 para vaciar todo lo recibido. */
uint8_t link_poll(uint8_t *datos);

#endif
