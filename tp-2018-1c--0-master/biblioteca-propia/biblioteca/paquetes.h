#ifndef SRC_PROCESAMIENTOPAQUETES_H_
#define SRC_PROCESAMIENTOPAQUETES_H_

#include "estructuras.h"
#include "serializaciones.h"

/*-----------------------------------Paquetes-----------------------------------*/
void	 						enviarPaquetes				(int socketfd, t_paquete * unPaquete);
int 							recibirTamPaquete			(int client_socket);
t_paquete * 					recibirPaquete				(int client_socket, int tamPaquete);
t_paquete * 					crearPaquete				(void * buffer);
t_paquete *						crearPaqueteError			(int client_socket);
void 							destruirPaquete				(t_paquete * unPaquete);
void 							mostrarPaquete				(t_paquete * unPaquete);

/*-----------------------------------Enviar paquetes-----------------------------------*/
void 							enviarHandshake				(int server_socket, int emisor);
void 							enviarMensaje				(int server_socket, char * mensaje);
void 							enviarArchivo				(int server_socket, char * rutaArchivo);
void 							enviarSolicitudEjecucion	(int server_socket);
void 							enviarNombreEsi				(int server_socket, char * nombre);
void 							enviarEjecucionTerminada	(int server_socket);
void 							enviarNombreInstancia		(int server_socket, char * nombre);
void 							enviarGet					(int server_socket, char * clave);
void 							enviarSet					(int server_socket, char * clave, char * valor);
void 							enviarSetDefinitivo			(int server_socket, char * clave, char * valor);
void 							enviarStore					(int server_socket, char * clave);
void 							enviarSolicitudStatus		(int server_socket, char * clave);
void 							enviarRespuesta				(int server_socket, int codRespuesta);
void 							enviarRespuestaStatus		(int server_socket, char* valor, char * nomInstanciaActual, char * nomIntanciaPosible);
void 							enviarInfoInstancia			(int server_socket, int cantEntradas, int tamanioEntrada, t_list * listaClaves);
void 							enviarCompactacion			(int server_socket);
void 							enviarSolicitudValor		(int server_socket, char * clave);
void 							enviarRespSolicitudValor	(int server_socket, bool claveExistente, char * valor);
void							enviarClaveEliminada		(int server_socket, char * clave);
void							enviarAvisoDesconexion		(int server_socket);
void 							enviarBloqueoESI			(int server_socket);

/*-----------------------------------Recibir paquetes-----------------------------------*/
int								recibirHandshake			(t_paquete * unPaquete);
char * 					  		recibirMensaje				(t_paquete * unPaquete);
void * 					  		recibirArchivo				(t_paquete * unPaquete);
char * 					  		recibirNombreEsi			(t_paquete * unPaquete);
char * 					  		recibirNombreInstancia		(t_paquete * unPaquete);
t_claveValor *					recibirSet					(t_paquete * unPaquete);
t_claveValor *					recibirSetDefinitivo		(t_paquete * unPaquete);
char* 					  		recibirGet					(t_paquete * unPaquete);
char* 					  		recibirStore				(t_paquete * unPaquete);
int 							recibirRespuesta			(t_paquete* unPaquete);
char *							recibirSolicitudStatus		(t_paquete * unPaquete);
t_respuestaStatus* 				recibirRespuestaStatus		(t_paquete * unPaquete);
t_infoInstancia * 				recibirInfoInstancia		(t_paquete * unPaquete);
char *							recibirSolicitudValor		(t_paquete * unPaquete);
t_respuestaValor *				recibirRespSolicitudValor	(t_paquete * unPaquete);
char *							recibirClaveEliminada		(t_paquete * unPaquete);

#endif /* SRC_PROCESAMIENTOPAQUETES_H_ */
