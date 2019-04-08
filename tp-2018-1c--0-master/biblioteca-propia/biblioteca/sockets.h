#ifndef BIBLIOTECA_SOCKETS_H_
#define BIBLIOTECA_SOCKETS_H_

#include "estructuras.h"
#include <commons/log.h>
#include "paquetes.h"

/*------------------------------Clientes------------------------------*/
int 				conectarCliente					(const char * ip, int puerto, int cliente);
void 				gestionarSolicitudes			(int server_socket, void (*procesarPaquete)(void*, int*), t_log * logger);

/*------------------------------Servidor------------------------------*/
void	 			iniciarServer					(int puerto, void (*procesarPaquete)(void*, int*), t_log * logger);
int 				crearSocketServer				(char * puerto);
void 				gestionarNuevasConexiones		(int server_socket, fd_set * set_master, int * descriptor_mas_alto, t_log * logger);
void 				gestionarDatosCliente			(int client_socket, fd_set * set_master, void (*procesarPaquete)(void*, int*), t_log * logger);

#endif /* BIBLIOTECA_SOCKETS_H_ */
