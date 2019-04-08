#ifndef BIBLIOTECA_SERIALIZACION_H_
#define BIBLIOTECA_SERIALIZACION_H_

#include "estructuras.h"
#include <commons/string.h>
#include <commons/collections/list.h>

/*----------------------------------------Serializacion----------------------------------------*/
void	serializarNumero				(t_paquete * unPaquete, int numero);
void	serializarMensaje				(t_paquete * unPaquete, char * mensaje);
void	serializarHandshake				(t_paquete * unPaquete, int emisor);
void	serializarArchvivo				(t_paquete * unPaquete, char * rutaArchivo);
void 	serializarClave					(t_paquete * unPaquete, char * clave);
void 	serializarClaveValor			(t_paquete * unPaquete, char * clave, char * valor);
void	serializarRespuestaStatus		(t_paquete* unPaquete, char * valor, char * nomInstanciaActual, char * nomIntanciaPosible);
void 	serializarInfoInstancia			(t_paquete * unPaquete, int cantEntradas, int tamanioEntrada, t_list * listaClaves);
void	serializarExistenciaClaveValor	(t_paquete * unPaquete, bool claveExistente, void * valor);

/*----------------------------------------Deserializacion----------------------------------------*/
int 				deserializarNumero					(t_stream * buffer);
char * 				deserializarMensaje					(t_stream * buffer);
int 				deserializarHandshake				(t_stream * buffer);
void * 				deserializarArchivo					(t_stream * buffer);
char * 				deserializarClave					(t_stream * buffer);
t_claveValor* 		deserializarClaveValor				(t_stream * buffer);
t_respuestaStatus* 	deserializarRespuestaStatus			(t_stream * buffer);
t_infoInstancia * 	deserializarInfoInstancia			(t_stream * buffer);
t_respuestaValor * 	deserializarExistenciaClaveValor	(t_stream * buffer);
bool 				deserializarBool					(t_stream* buffer);

/*----------------------------------------Funciones auxiliares----------------------------------------*/
void *	abrirArchivo	(char * rutaArchivo, size_t * tamArc, FILE ** archivo);

#endif /* BIBLIOTECA_SERIALIZACION_H_ */
