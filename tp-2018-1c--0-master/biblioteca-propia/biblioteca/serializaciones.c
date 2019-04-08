#include "serializaciones.h"

/*-------------------------Serializacion-------------------------*/
void serializarNumero(t_paquete* unPaquete, int numero) {
	int tamNumero = sizeof(int);

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamNumero;
	unPaquete->buffer->data = malloc(tamNumero);

	memcpy(unPaquete->buffer->data, &numero, tamNumero);
}

void serializarMensaje(t_paquete* unPaquete, char * palabra) {
	int tamPalabra = strlen(palabra) + 1;

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->data = malloc(tamPalabra);
	unPaquete->buffer->size = tamPalabra;

	memcpy(unPaquete->buffer->data, palabra, tamPalabra);
}

void serializarHandshake(t_paquete * unPaquete, int emisor) {
	serializarNumero(unPaquete, emisor);
}

void serializarArchvivo(t_paquete * unPaquete, char * rutaArchivo) {
	size_t tamArch;

	FILE * archivofd;

	void * archivo = abrirArchivo(rutaArchivo, &tamArch, &archivofd);

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->data = malloc(tamArch);
	unPaquete->buffer->size = tamArch;

	memcpy(unPaquete->buffer->data, archivo, tamArch);

	munmap(archivo, tamArch);

	fclose(archivofd);
}

void serializarClave(t_paquete * unPaquete, char * clave) {
	serializarMensaje(unPaquete, clave);
}

void serializarClaveValor(t_paquete * unPaquete, char * clave, char * valor) {
	int tamClave = strlen(clave) + 1;
	int tamValor = strlen(valor) + 1;

	int tamTotal = tamValor + tamClave;

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamTotal;

	unPaquete->buffer->data = malloc(tamTotal);

	memcpy(unPaquete->buffer->data + desplazamiento, clave, tamClave);
	desplazamiento += tamClave;

	memcpy(unPaquete->buffer->data + desplazamiento, valor, tamValor);
	desplazamiento += tamValor;
}

void serializarRespuestaStatus(t_paquete* unPaquete, char * valor,
		char * nomInstanciaActual, char * nomIntanciaPosible) {
	int tamActual = strlen(nomInstanciaActual) + 1;
	int tamPosible = strlen(nomIntanciaPosible) + 1;
	int tamValor = strlen(valor) + 1;

	int tamTotal = tamValor + tamActual + tamPosible;

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamTotal;

	unPaquete->buffer->data = malloc(tamTotal);

	memcpy(unPaquete->buffer->data + desplazamiento, nomInstanciaActual,
			tamActual);
	desplazamiento += tamActual;

	memcpy(unPaquete->buffer->data + desplazamiento, nomIntanciaPosible,
			tamPosible);
	desplazamiento += tamPosible;

	memcpy(unPaquete->buffer->data + desplazamiento, valor, tamValor);
	desplazamiento += tamValor;
}

void serializarInfoInstancia(t_paquete * unPaquete, int cantEntradas,
		int tamanioEntrada, t_list * listaClaves) {
	int tamCantEntradas = sizeof(int);
	int tamTamanioEntrada = sizeof(int);
	int tamCantidadElementosLista = sizeof(int);

	int tamTotal = tamCantEntradas + tamTamanioEntrada
			+ tamCantidadElementosLista;

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->size = tamTotal;

	unPaquete->buffer->data = malloc(tamTotal);

	memcpy(unPaquete->buffer->data + desplazamiento, &cantEntradas,
			tamCantEntradas);
	desplazamiento += tamCantEntradas;

	memcpy(unPaquete->buffer->data + desplazamiento, &tamanioEntrada,
			tamTamanioEntrada);
	desplazamiento += tamTamanioEntrada;

	memcpy(unPaquete->buffer->data + desplazamiento,
			&listaClaves->elements_count, tamCantidadElementosLista);
	desplazamiento += tamCantidadElementosLista;

	void serializarListaClaves(char * clave) {
		int tamClave = strlen(clave) + 1;

		unPaquete->buffer->size += tamClave;

		unPaquete->buffer->data = realloc(unPaquete->buffer->data,
				unPaquete->buffer->size);

		memcpy(unPaquete->buffer->data + desplazamiento, clave, tamClave);
		desplazamiento += tamClave;

	}

	list_iterate(listaClaves, (void *) serializarListaClaves);

}

void serializarExistenciaClaveValor(t_paquete * unPaquete, bool claveExistente,
		void * valor) {

	int tamClaveExistente = sizeof(bool);

	int tamValor;

	if (valor != NULL) {
		tamValor = strlen(valor) + 1;
	} else {
		tamValor = 0;
	}

	int tamTotal = tamClaveExistente + tamValor;

	int desplazamiento = 0;

	unPaquete->buffer = malloc(sizeof(t_stream));
	unPaquete->buffer->data = malloc(tamTotal);
	unPaquete->buffer->size = tamTotal;

	memcpy(unPaquete->buffer->data + desplazamiento, &claveExistente,
			tamClaveExistente);
	desplazamiento += tamClaveExistente;

	memcpy(unPaquete->buffer->data + desplazamiento, valor, tamValor);
}

/*-------------------------Deserializacion-------------------------*/
int deserializarNumero(t_stream* buffer) {
	return *(int*) (buffer->data);
}

char * deserializarMensaje(t_stream * buffer) {
	char * palabra = strdup(buffer->data);

	return palabra;
}

int deserializarHandshake(t_stream * buffer) {
	return deserializarNumero(buffer);
}

void * deserializarArchivo(t_stream * buffer) {
	void * archivo = malloc(buffer->size);

	memcpy(archivo, buffer->data, buffer->size);

	return archivo;
}

char * deserializarClave(t_stream * buffer) {
	return deserializarMensaje(buffer);
}

t_claveValor* deserializarClaveValor(t_stream * buffer) {
	int desplazamiento = 0;
	t_claveValor* ret = malloc(sizeof(t_claveValor));

	ret->clave = strdup(buffer->data + desplazamiento);
	desplazamiento += string_length(ret->clave);

	ret->valor = strdup(buffer->data + desplazamiento + 1);
	desplazamiento += string_length(ret->valor);

	return ret;
}

t_respuestaStatus* deserializarRespuestaStatus(t_stream * buffer) {
	int desplazamiento = 0;
	t_respuestaStatus* ret = malloc(sizeof(t_respuestaStatus));

	ret->nomInstanciaActual = strdup(buffer->data + desplazamiento);
	desplazamiento += string_length(ret->nomInstanciaActual);

	ret->nomIntanciaPosible = strdup(buffer->data + desplazamiento);
	desplazamiento += string_length(ret->nomIntanciaPosible);

	ret->valor = strdup(buffer->data + desplazamiento);
	desplazamiento += string_length(ret->valor);

	return ret;
}

t_infoInstancia * deserializarInfoInstancia(t_stream * buffer) {
	t_infoInstancia * info = malloc(sizeof(t_infoInstancia));

	info->listaClaves = list_create();

	int tamNumero = sizeof(int);

	int cantElementosLista;

	int desplazamiento = 0;

	memcpy(&info->cantEntradas, buffer->data + desplazamiento, tamNumero);
	desplazamiento += tamNumero;

	memcpy(&info->tamanioEntrada, buffer->data + desplazamiento, tamNumero);
	desplazamiento += tamNumero;

	memcpy(&cantElementosLista, buffer->data + desplazamiento, tamNumero);
	desplazamiento += tamNumero;

	int i;
	for (i = 0; i < cantElementosLista; ++i) {
		char * clave  = strdup(buffer->data + desplazamiento);
		desplazamiento += strlen(clave) + 1;

		list_add(info->listaClaves, clave);
	}

	return info;
}

t_respuestaValor * deserializarExistenciaClaveValor(t_stream * buffer) {
	t_respuestaValor * respuesta = malloc(sizeof(t_infoInstancia));

	int tamExistenciaClave = sizeof(bool);

	int desplazamiento = 0;

	memcpy(&respuesta->existenciaClave, buffer->data + desplazamiento,
			tamExistenciaClave);
	desplazamiento += tamExistenciaClave;

	int tamValor = buffer->size - tamExistenciaClave;

	if (tamValor != 0) {
		respuesta->valor = malloc(buffer->size - tamExistenciaClave);

		memcpy(respuesta->valor, buffer->data + desplazamiento,
				buffer->size - tamExistenciaClave);
	} else {
		respuesta->valor = NULL;
	}

	return respuesta;
}

/*-------------------------Funciones auxiliares-------------------------*/
void * abrirArchivo(char * rutaArchivo, size_t * tamArc, FILE ** archivo) {
	//Abro el archivo
	*archivo = fopen(rutaArchivo, "r");

	if (*archivo == NULL) {
		printf("%s: No existe el archivo o el directorio", rutaArchivo);
		return NULL;
	}

	//Copio informacion del archivo
	struct stat statArch;

	stat(rutaArchivo, &statArch);

	//Tamaño del archivo que voy a leer
	*tamArc = statArch.st_size;

	//Leo el total del archivo y lo asigno al buffer
	int fd = fileno(*archivo);
	void * dataArchivo = mmap(0, *tamArc, PROT_READ, MAP_SHARED, fd, 0);

	return dataArchivo;
}
