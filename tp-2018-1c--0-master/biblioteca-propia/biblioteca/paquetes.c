#include "paquetes.h"

/*------------------------------Paquetes------------------------------*/
void enviarPaquetes(int socketfd, t_paquete * unPaquete) {

	int desplazamiento = 0;

	int tamPaquete = sizeof(int);
	int tamCodOp = sizeof(int);
	int tamSize = sizeof(int);
	int tamData = unPaquete->buffer->size;

	int tamTotal = tamCodOp + tamSize + tamData;

	void * buffer = malloc(tamPaquete + tamTotal);

	//Tamaño del paquete
	memcpy(buffer + desplazamiento, &tamTotal, tamPaquete);
	desplazamiento += tamPaquete;

	//Codigo de operacion
	memcpy(buffer + desplazamiento, &unPaquete->codigoOperacion, tamCodOp);
	desplazamiento += tamCodOp;

	//Buffer -- size
	memcpy(buffer + desplazamiento, &unPaquete->buffer->size, tamSize);
	desplazamiento += tamSize;

	//Buffer -- data
	memcpy(buffer + desplazamiento, unPaquete->buffer->data, tamData);

	//Envio el paquete y compruebo errores
	int resultado = send(socketfd, buffer, tamPaquete + tamTotal, MSG_NOSIGNAL);

	if (resultado == -1)
		perror("send");

	//Libero memoria
	destruirPaquete(unPaquete);
	free(buffer);
}

int recibirTamPaquete(int client_socket) {
	int tamBuffer;

	//Creo el buffer
	void * buffer = malloc(sizeof(int));

	//Recibo el buffer
	int recvd = recv(client_socket, buffer, sizeof(int), MSG_WAITALL);

	//Verifico error o conexión cerrada por el cliente
	if (recvd <= 0) {
		tamBuffer = recvd;
		if (recvd == -1) {
			perror("recv");
		}

		//Cierro el socket
		close(client_socket);

		//Libero memoria
		free(buffer);

		return -1;
	} else {

		//Copio el buffer en una variable asi despues lo libero
		memcpy(&tamBuffer, buffer, sizeof(int));
	}

	//Libero memoria
	free(buffer);

	return tamBuffer;
}

t_paquete * recibirPaquete(int client_socket, int tamPaquete) {
	//Reservo memoria para el buffer
	void * buffer = malloc(tamPaquete);
	//memset(buffer, '\0', tamPaquete); // Lo limpio al buffer para que no tenga basura

	//Recibo el buffer
	int recvd = recv(client_socket, buffer, tamPaquete, MSG_WAITALL);

	//Verifico error o conexión cerrada por el cliente
	if (recvd <= 0) {
		if (recvd == -1) {
			perror("recv");
		}
		printf("El socket %d ha producido un error "
				"y ha sido desconectado.\n", client_socket);

		//Cierro el socket
		close(client_socket);

	}

	//Armo el paquete a partir del buffer
	t_paquete * unPaquete = crearPaquete(buffer);

	return unPaquete;
}

t_paquete * crearPaquete(void * buffer) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	int desplazamiento = 0;

	int tamCodOp = sizeof(int);
	int tamSize = sizeof(size_t);

	//Codigo de operacion
	memcpy(&unPaquete->codigoOperacion, buffer + desplazamiento, tamCodOp);
	desplazamiento += tamCodOp;

	//Buffer -- size
	unPaquete->buffer = malloc(sizeof(t_stream));
	memcpy(&unPaquete->buffer->size, buffer + desplazamiento, tamSize);
	desplazamiento += tamSize;

	//Buffer -- data
	int tamData = unPaquete->buffer->size;
	unPaquete->buffer->data = malloc(tamData);
	memcpy(unPaquete->buffer->data, buffer + desplazamiento, tamData);

	//Libero memoria
	free(buffer);

	return unPaquete;
}

t_paquete * crearPaqueteError(int client_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));
	unPaquete->codigoOperacion = ENVIAR_ERROR;
	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = sizeof(int);
	unPaquete->buffer->data = malloc(unPaquete->buffer->size);
	memcpy(unPaquete->buffer->data, &client_socket, unPaquete->buffer->size);
	return unPaquete;
}

void destruirPaquete(t_paquete * unPaquete) {
	free(unPaquete->buffer->data);
	free(unPaquete->buffer);
	free(unPaquete);
}

void mostrarPaquete(t_paquete * unPaquete) {
	printf("Muestro el paquete: \n");
	printf("Codigo de operacion: %d \n", unPaquete->codigoOperacion);
	printf("Tamanio del buffer: %d \n", unPaquete->buffer->size);
	printf("Buffer: %s \n", (char*) unPaquete->buffer->data);
}

/*-------------------------Enviar-------------------------*/
void enviarHandshake(int server_socket, int emisor) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = HANDSHAKE;

	serializarHandshake(unPaquete, emisor);

	enviarPaquetes(server_socket, unPaquete);

}

void enviarMensaje(int server_socket, char * mensaje) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_MENSAJE;

	serializarMensaje(unPaquete, mensaje);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarArchivo(int server_socket, char * rutaArchivo) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_ARCHIVO;

	serializarArchvivo(unPaquete, rutaArchivo);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarSolicitudEjecucion(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SOLICITUD_EJECUCION;

	serializarNumero(unPaquete, 0);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarEjecucionTerminada(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = TERMINO_ESI;

	serializarNumero(unPaquete, 0);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarNombreEsi(int server_socket, char * nombre) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_NOMBRE_ESI;

	serializarMensaje(unPaquete, nombre);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarNombreInstancia(int server_socket, char * nombre) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_NOMBRE_INSTANCIA;

	serializarMensaje(unPaquete, nombre);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarGet(int server_socket, char * clave) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = GET;

	serializarClave(unPaquete, clave);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarSet(int server_socket, char * clave, char * valor) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SET;

	serializarClaveValor(unPaquete, clave, valor);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarSetDefinitivo(int server_socket, char * clave, char * valor) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SET_DEFINITIVO;

	serializarClaveValor(unPaquete, clave, valor);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarStore(int server_socket, char * clave) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = STORE;

	serializarClave(unPaquete, clave);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarRespuesta(int server_socket, int codRespuesta) {
	t_paquete* unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = RESPUESTA_SOLICITUD;

	serializarNumero(unPaquete, codRespuesta);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarSolicitudStatus(int server_socket, char * clave) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SOLICITAR_STATUS;


	serializarMensaje(unPaquete, clave);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarRespuestaStatus(int server_socket, char* valor,
		char * nomInstanciaActual, char * nomIntanciaPosible) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = RESPUESTA_STATUS;

	serializarRespuestaStatus(unPaquete, valor, nomInstanciaActual,
			nomIntanciaPosible);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarInfoInstancia(int server_socket, int cantEntradas,
		int tamanioEntrada, t_list * listaClaves) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_INFO_INSTANCIA;

	serializarInfoInstancia(unPaquete, cantEntradas, tamanioEntrada, listaClaves);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarCompactacion(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = COMPACTAR;

	serializarNumero(unPaquete, 0);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarSolicitudValor(int server_socket, char * clave){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = SOLICITAR_VALOR;

	serializarMensaje(unPaquete, clave);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarRespSolicitudValor(int server_socket, bool claveExistente, char * valor){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = RESPUESTA_SOLICITAR_VALOR;

	serializarExistenciaClaveValor(unPaquete, claveExistente, valor);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarClaveEliminada(int server_socket, char * clave){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_CLAVE_ELIMINADA;

	serializarMensaje(unPaquete, clave);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarAvisoDesconexion(int server_socket){
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_AVISO_DESCONEXION;

	serializarNumero(unPaquete, 0);

	enviarPaquetes(server_socket, unPaquete);
}

void enviarBloqueoESI(int server_socket) {
	t_paquete * unPaquete = malloc(sizeof(t_paquete));

	unPaquete->codigoOperacion = ENVIAR_BLOQUEO_ESI;

	serializarNumero(unPaquete, 0);

	enviarPaquetes(server_socket, unPaquete);
}

/*-------------------------Recibir-------------------------*/
int recibirHandshake(t_paquete * unPaquete) {
	return deserializarHandshake(unPaquete->buffer);
}

char * recibirMensaje(t_paquete * unPaquete) {
	return deserializarMensaje(unPaquete->buffer);
}

void* recibirArchivo(t_paquete * unPaquete) {
	return deserializarArchivo(unPaquete->buffer);
}

char * recibirNombreEsi(t_paquete * unPaquete) {
	return deserializarMensaje(unPaquete->buffer);
}

char * recibirNombreInstancia(t_paquete * unPaquete) {
	return deserializarMensaje(unPaquete->buffer);
}

t_claveValor* recibirSet(t_paquete* unPaquete) {
	return deserializarClaveValor(unPaquete->buffer);
}

t_claveValor* recibirSetDefinitivo(t_paquete* unPaquete) {
	return deserializarClaveValor(unPaquete->buffer);
}

char * recibirGet(t_paquete* unPaquete) {
	return deserializarMensaje(unPaquete->buffer);
}

char * recibirStore(t_paquete* unPaquete) {
	return deserializarMensaje(unPaquete->buffer);
}

int recibirRespuesta(t_paquete* unPaquete) {
	return deserializarNumero(unPaquete->buffer);
}

char * recibirSolicitudStatus(t_paquete * unPaquete){
	return deserializarMensaje(unPaquete->buffer);
}

t_respuestaStatus * recibirRespuestaStatus(t_paquete* unPaquete) {
	return deserializarRespuestaStatus(unPaquete->buffer);
}

t_infoInstancia * recibirInfoInstancia(t_paquete * unPaquete) {
	return deserializarInfoInstancia(unPaquete->buffer);
}

char * recibirSolicitudValor(t_paquete * unPaquete){
	return deserializarMensaje(unPaquete->buffer);
}

t_respuestaValor * recibirRespSolicitudValor(t_paquete * unPaquete){
	return deserializarExistenciaClaveValor(unPaquete->buffer);
}

char * recibirClaveEliminada(t_paquete * unPaquete){
	return deserializarMensaje(unPaquete->buffer);
}
