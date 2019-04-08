#include "instancia.h"

int main(void) {
	//Creo archivo de log
	logInstancia = log_create("log_Instancia.log", "instancia", true,
			LOG_LEVEL_TRACE);
	log_trace(logInstancia, "Inicio el proceso instancia \n");

	//Conecto instancia con coordinador
	conectarInstancia();

	//Manejo de señales
	signal(SIGINT, (void*) procesarSIGINT);

	//Quedo a la espera de solicitudes
	recibirSolicitudes = true;
	while (recibirSolicitudes) {
		gestionarSolicitudes(socketCoordinador, (void*) procesarPaquete,
				logInstancia);
	}

	//Corto el hilo de almacenamiento
	almacenar = false;
	intervaloDump = 0;
	log_warning(logInstancia, "Espero para hacer el ultimo dump \n");
	//pthread_cancel(threadAlmacenamientoContinuo);
	pthread_join(threadAlmacenamientoContinuo, NULL);

	//Termina esi
	log_trace(logInstancia, "Termino el proceso instancia \n");

	//Destruyo archivo de log
	log_destroy(logInstancia);

	//Libero memoria
	destruirTablaEntradas();
	destruirBitMap();
	destruirStorage();
	free(puntoMontaje);
	free(algoritmoReemplazo);
	pthread_mutex_destroy(&mutexDumpCompactacion);

	return EXIT_SUCCESS;
}

/*-------------------------Conexion-------------------------*/
void conectarInstancia() {
	//Leo la configuracion del esi
	t_config* configInstancia = leerConfiguracion();

	//Setteo las variables de configuracion
	char * coordinadorIP = config_get_string_value(configInstancia,
			"COORDINADOR_IP");
	int coordinadorPuerto = config_get_int_value(configInstancia,
			"COORDINADOR_PUERTO");
	algoritmoReemplazo = strdup(
			config_get_string_value(configInstancia, "ALGORITMO_REEMPLAZO"));
	puntoMontaje = strdup(
			config_get_string_value(configInstancia, "PUNTO_MONTAJE"));
	char * nombreInstancia = config_get_string_value(configInstancia,
			"NOMBRE_INSTANCIA");
	intervaloDump = config_get_int_value(configInstancia, "INTERVALO_DUMP");

	//Conecto al coordinador
	socketCoordinador = conectarCliente(coordinadorIP, coordinadorPuerto,
			INSTANCIA);

	if (socketCoordinador == -1) {
		log_error(logInstancia, "No esta conectado el coordinador");
		exit(EXIT_FAILURE);
	}

	enviarNombreInstancia(socketCoordinador, nombreInstancia);

	//Destruyo la configuracion
	config_destroy(configInstancia);
}

t_config* leerConfiguracion() {
	t_config* configInstancia = config_create(RUTA_CONFIG);
	if (!configInstancia) {
		log_error(logInstancia, "Error al abrir la configuracion \n");
		exit(EXIT_FAILURE);
	}
	return configInstancia;
}

/*-------------------------Procesamiento paquetes-------------------------*/
void procesarPaquete(t_paquete * unPaquete, int * client_socket) {
	switch (unPaquete->codigoOperacion) {
	case ENVIAR_INFO_INSTANCIA:
		procesarEnviarInfoInstancia(unPaquete);
		break;
	case SET:
		procesarSet(unPaquete, *client_socket);
		break;
	case SET_DEFINITIVO:
		procesarSetDefinitivo(unPaquete, *client_socket);
		break;
	case STORE:
		procesarStore(unPaquete, *client_socket);
		break;
	case COMPACTAR:
		procesarCompactacion(unPaquete, *client_socket);
		break;
	case SOLICITAR_VALOR:
		procesarSolicitudValor(unPaquete, *client_socket);
		break;
	case ENVIAR_ERROR:
		procesarError(unPaquete);
		break;
	default:
		break;
	}
	destruirPaquete(unPaquete);
}

void procesarEnviarInfoInstancia(t_paquete * unPaquete) {
	t_infoInstancia * info = recibirInfoInstancia(unPaquete);

	//Setteo tam de entrada y cantidad
	cantEntradas = info->cantEntradas;
	tamanioEntrada = info->tamanioEntrada;

	log_trace(logInstancia,
			"La cantidad de entradas del Storage es: %d y el tamanio es: %d \n",
			cantEntradas, tamanioEntrada);

	//Creo el espacio de almacenamiento
	crearStorage();

	//Creo bitMap del storage
	crearBitMap();

	//Creo tabla de entradas
	crearTablaEntradas();

	//Verifico que no tenga archivos anteriores
	recuperarInformacionDeInstancia(info->listaClaves);

	//Creo el hilo para hacer el dump
	crearAlmacenamientoContinuo();

	//Libero memoria
	list_destroy_and_destroy_elements(info->listaClaves, free);
	free(info);

}

void procesarSet(t_paquete * unPaquete, int client_socket) {
	t_claveValor * claveValor = recibirSet(unPaquete);

	log_trace(logInstancia,
			"Me llego un SET con la clave: %s y con el valor: %s",
			claveValor->clave, (char*) claveValor->valor);

	t_tabla_entradas * entrada = buscarEntrada(claveValor->clave);

	if (entrada != NULL) {
		eliminarClave(claveValor->clave);
		log_warning(logInstancia,
				"La clave existe por lo que la elimino y guardo el valor nuevo \n");
	}

	int entradasNecesarias = entradasNecesariaParaUnTamanio(
			string_length(claveValor->valor));

	int entradasLibres = cantidadIndexLibres();

	int respuesta;

	if (entradasNecesarias <= entradasLibres) {
		respuesta = agregarClaveValor(claveValor->clave, claveValor->valor);
		if (respuesta == CANTIDAD_INDEX_LIBRES_INEXISTENTES) {
			enviarRespuesta(client_socket, ERROR_ESPACIO_INSUFICIENTE);
		} else {
			enviarRespuesta(client_socket, SET_OK);
			void * valor = buscarValorSegunClave(claveValor->clave);
			log_trace(logInstancia, "El valor de la clave guardada es: %s",
					(char *) valor);
			free(valor);
			aumentarTiempoReferenciadoMenosAClave(claveValor->clave);
		}
	} else {
		log_warning(logInstancia, "Ejecuto algoritmo de reemplazo: %s",
				algoritmoReemplazo);

		if (string_equals_ignore_case(algoritmoReemplazo, "CIRC")) {
			algoritmoReemplazoCircular(claveValor->clave, claveValor->valor);
		}

		if (string_equals_ignore_case(algoritmoReemplazo, "LRU")) {
			algoritmoReemplazoLeastRecentlyUsed(claveValor->clave,
					claveValor->valor);
		}

		if (string_equals_ignore_case(algoritmoReemplazo, "BSU")) {
			algoritmoReemplazoBiggestSpaceUsed(claveValor->clave,
					claveValor->valor);
		}

		respuesta = agregarClaveValor(claveValor->clave, claveValor->valor);

		if (respuesta == CANTIDAD_INDEX_LIBRES_INEXISTENTES) {
			enviarRespuesta(client_socket, ERROR_ESPACIO_INSUFICIENTE);
			log_error(logInstancia, "Error espacio insuficiente");
			aumentarTiempoReferenciadoMenosAClave(claveValor->clave);
		} else {
			enviarRespuesta(client_socket, SET_OK);
			void * valor = buscarValorSegunClave(claveValor->clave);
			log_trace(logInstancia, "El valor de la clave guardada es: %s",
					(char *) valor);
			free(valor);
			aumentarTiempoReferenciadoMenosAClave(claveValor->clave);

		}

	}

	mostrarBitmap();
	mostrarTablaEntradas();

	free(claveValor->clave);
	free(claveValor->valor);
	free(claveValor);

}

void procesarSetDefinitivo(t_paquete * unPaquete, int client_socket) {
	t_claveValor * claveValor = recibirSetDefinitivo(unPaquete);

	log_trace(logInstancia,
			"Me llego un SET_DEFINITIVO con la clave: %s y con el valor: %s",
			claveValor->clave, (char*) claveValor->valor);

	int respuesta = agregarClaveValor(claveValor->clave, claveValor->valor);

	aumentarTiempoReferenciadoMenosAClave(claveValor->clave);

	if (respuesta == CANTIDAD_INDEX_LIBRES_INEXISTENTES) {
		enviarRespuesta(client_socket, SET_DEFINITIVO_ERROR);
		log_error(logInstancia, "Error espacio insuficiente");
	} else {
		enviarRespuesta(client_socket, SET_DEFINITIVO_OK);
		void * valor = buscarValorSegunClave(claveValor->clave);
		log_trace(logInstancia, "El valor de la clave guardada es: %s",
				(char *) valor);
		free(valor);

	}

	free(claveValor->clave);
	free(claveValor->valor);
	free(claveValor);
}

void procesarStore(t_paquete * unPaquete, int client_socket) {
	char * clave = recibirStore(unPaquete);

	log_trace(logInstancia, "Me llego un STORE con la clave: %s", clave);

	t_tabla_entradas * entradaBuscada = buscarEntrada(clave);

	aumentarTiempoReferenciadoMenosAClave(clave);

	entradaBuscada->tiempoReferenciado = 0;

	almacenarEnMemoriaSecundaria(entradaBuscada);

	void * valor = buscarValorSegunClave(clave);

	log_trace(logInstancia, "El valor de la clave que guarde en memoria es: %s",
			(char *) valor);

	enviarRespuesta(client_socket, STORE_OK);

	free(valor);
	free(clave);
}

void procesarCompactacion(t_paquete * unPaquete, int client_socket) {
	log_trace(logInstancia, "Me llego pedido de compactacion");
	compactar();
	log_trace(logInstancia, "compactado");
	enviarCompactacion(client_socket);
	log_trace(logInstancia, "enviado aviso de termine compactacion");
	mostrarBitmap();
	log_trace(logInstancia, "mostre bitmap");
}

void procesarSolicitudValor(t_paquete * unPaquete, int client_socket) {
	char * clave = recibirSolicitudValor(unPaquete);

	log_trace(logInstancia, "Me llego solicitud de valor de la clave: %s",
			clave);

	t_tabla_entradas * respuesta = buscarEntrada(clave);

	if (respuesta == NULL) {
		log_warning(logInstancia, "La clave no existe");
		enviarRespSolicitudValor(client_socket, false, NULL);
		free(clave);
		return;
	}

	char * valorRespuesta = buscarValorSegunClave(respuesta->clave);

	if (!string_equals_ignore_case(valorRespuesta, "")) {
		log_trace(logInstancia, "El valor pedido es: %s", valorRespuesta);
		enviarRespSolicitudValor(client_socket, true, valorRespuesta);
	} else {
		log_warning(logInstancia, "El valor pedido es nulo");
		enviarRespSolicitudValor(client_socket, true, NULL);
	}

	free(respuesta);
	free(clave);
	free(valorRespuesta);
}

void procesarError(t_paquete * unPaquete) {
	log_error(logInstancia,
			"Me llego un error y dejo de recibir solicitudes \n");
	recibirSolicitudes = false;
}

/*-------------------------Tabla de entradas-------------------------*/
void crearTablaEntradas(void) {
	tablaEntradas = list_create();

	log_trace(logInstancia, "Creo la tabla de entradas \n");
}

void destruirTablaEntradas(void) {
	void eliminarEntrada(t_tabla_entradas * entrada) {
		free(entrada);
	}

	list_destroy_and_destroy_elements(tablaEntradas, (void *) eliminarEntrada);
}

t_tabla_entradas * buscarEntrada(char * clave) {
	bool esEntradaBuscada(t_tabla_entradas * entrada) {
		return string_equals_ignore_case(entrada->clave, clave);
	}

	t_tabla_entradas * registroEntrada = list_find(tablaEntradas,
			(void*) esEntradaBuscada);

	return registroEntrada;
}

void eliminarClave(char * clave) {
	pthread_mutex_lock(&mutexDumpCompactacion);

	bool esEntradaBuscada(t_tabla_entradas * entrada) {
		return string_equals_ignore_case(entrada->clave, clave);
	}

	t_tabla_entradas * entradaBuscada = list_remove_by_condition(tablaEntradas,
			(void*) esEntradaBuscada);

	if (entradaBuscada != NULL) {
		if (entradaBuscada->tamanio != 0) {
			int i;

			int cantidadEntradasABorar = entradasNecesariaParaUnTamanio(
					entradaBuscada->tamanio);

			for (i = 0; i < cantidadEntradasABorar; i++)
				liberarIndex(entradaBuscada->indexComienzo + i);
		}

		enviarClaveEliminada(socketCoordinador, entradaBuscada->clave);

		free(entradaBuscada);
	}

	pthread_mutex_unlock(&mutexDumpCompactacion);
}

void mostrarTablaEntradas(void) {
	int i;

	printf("Clave			Tamanio			1°Entrada \n");
	printf("----------------------------------------------------------\n");

	for (i = 0; i < tablaEntradas->elements_count; i++) {
		t_tabla_entradas * entrada = list_get(tablaEntradas, i);
		printf("%s			%d			%d \n", entrada->clave, entrada->tamanio,
				entrada->indexComienzo);
	}
	printf("\n");
}

int agregarClaveValor(char * clave, void * valor) {

	pthread_mutex_lock(&mutexDumpCompactacion);

	int tamValor = string_length(valor);

	int index = -1;

	void * respuesta = guardarEnStorage(valor, &index);

	if (respuesta == NULL) {
		pthread_mutex_unlock(&mutexDumpCompactacion);

		return CANTIDAD_INDEX_LIBRES_INEXISTENTES;
	} else {
		t_tabla_entradas * registroEntrada = malloc(sizeof(t_tabla_entradas));

		strncpy(registroEntrada->clave, clave,
				sizeof(registroEntrada->clave) - 1);

		registroEntrada->entrada = respuesta;

		registroEntrada->tamanio = tamValor;

		registroEntrada->indexComienzo = index;

		registroEntrada->tiempoReferenciado = 0;

		list_add(tablaEntradas, registroEntrada);

		pthread_mutex_unlock(&mutexDumpCompactacion);

		return 0;
	}
}

void * buscarValorSegunClave(char * clave) {
	t_tabla_entradas * entrada = buscarEntrada(clave);

	if (entrada == NULL)
		return NULL;

	char * respuesta = malloc(entrada->tamanio + 1);

	memcpy(respuesta, entrada->entrada, entrada->tamanio);

	respuesta[entrada->tamanio] = '\0';

	return respuesta;
}

t_tabla_entradas * buscarEntradaSegunIndex(int index) {
	bool esEntradaBuscada(t_tabla_entradas * entrada) {
		return (entrada->indexComienzo == index);
	}

	t_tabla_entradas * registroEntrada = list_find(tablaEntradas,
			(void*) esEntradaBuscada);

	return registroEntrada;
}

void mostrarEntrada(char * clave) {
	t_tabla_entradas * entrada = buscarEntrada(clave);

	printf("Clave: %s \n", entrada->clave);

	char * respuesta = malloc(entrada->tamanio + 1);

	memcpy(respuesta, entrada->entrada, entrada->tamanio);

	respuesta[entrada->tamanio] = '\0';

	printf("Valor: %s \n", respuesta);

	printf("Tamanio: %d \n\n", entrada->tamanio);

	free(respuesta);
}

void aumentarTiempoReferenciado(t_tabla_entradas * entrada) {
	entrada->tiempoReferenciado++;
}

void aumentarTiempoReferenciadoATodos(t_list * tabla) {
	list_iterate(tabla, (void *) aumentarTiempoReferenciado);
}

void aumentarTiempoReferenciadoMenosAClave(char* clave) {
	bool entradasMenosUna(t_tabla_entradas * entrada) {
		return !string_equals_ignore_case(entrada->clave, clave);
	}

	log_trace(logInstancia, "clave: %s", clave);

	t_list * listaFiltrada = list_filter(tablaEntradas,
			(void *) entradasMenosUna);

	aumentarTiempoReferenciadoATodos(listaFiltrada);

	t_tabla_entradas * entradaBuscada = buscarEntrada(clave);

	if (entradaBuscada != NULL)
		entradaBuscada->tiempoReferenciado = 0;

	list_destroy(listaFiltrada);
}

/*-------------------------BitMap del Storage-------------------------*/
void crearBitMap(void) {
	bitMap = malloc(sizeof(bool) * cantEntradas);

	liberarBitMap();

	log_trace(logInstancia, "Creo el bitMap");
}

void destruirBitMap(void) {
	free(bitMap);
}

void liberarBitMap(void) {
	for (int i = 0; i < cantEntradas; i++)
		liberarIndex(i);
}

void llenarBitMap(void) {
	for (int i = 0; i < cantEntradas; i++)
		ocuparIndex(i);
}

void liberarIndex(int index) {
	if (index + 1 <= cantEntradas) {
		bitMap[index] = false;
	} else {
		log_error(logInstancia,
				"No se puede liberar el index %d ya que no existe \n", index);
	}
}

void ocuparIndex(int index) {
	if (index + 1 <= cantEntradas) {
		bitMap[index] = true;
	} else {
		log_error(logInstancia,
				"No se puede ocuapar el index %d ya que no existe \n", index);
	}
}

int buscarIndexLibre(void) {
	int index = 0;

	while (bitMap[index]) {
		index++;
	}

	if (index < 99)
		bitMap[index] = false;

	return index;
}

void mostrarBitmap(void) {
	printf("Index			Ocupado \n");
	printf("-------------------------------\n");
	int i;
	for (i = 0; i < cantEntradas; i++)
		printf("%d			%d \n", i, bitMap[i]);
	printf("\n");
}

int buscarCantidadIndexLibres(int cantidad) {
	bool loEncontre = false;
	int candidato;
	int contador;
	int i;

	for (i = 0; !loEncontre && i < cantEntradas; i++) {
		if (!bitMap[i]) {
			candidato = i;
			contador = 1;

			while (contador < cantidad && (i + 1) < cantEntradas
					&& !bitMap[i + 1]) {
				i++;
				contador++;
			}

			if (contador == cantidad)
				loEncontre = true;
		}
	}

	if (!loEncontre)
		candidato = CANTIDAD_INDEX_LIBRES_INEXISTENTES;

	return candidato;
}

int cantidadIndexLibres(void) {
	int i;
	int cantidad = 0;
	for (i = 0; i < cantEntradas; i++) {
		if (!bitMap[i])
			cantidad++;
	}
	return cantidad;
}

/*-------------------------Storage-------------------------*/
void crearStorage(void) {
	storage = malloc(cantEntradas * tamanioEntrada);
	log_trace(logInstancia, "Creo el Storage");
}

void destruirStorage(void) {
	free(storage);
}

void * guardarEnStorage(void * valor, int * index) {
	int tamValor = strlen(valor);

	int entradasNecesaria = entradasNecesariaParaUnTamanio(tamValor);

	*index = buscarCantidadIndexLibres(entradasNecesaria);

	if ((*index) != CANTIDAD_INDEX_LIBRES_INEXISTENTES) {

		return guardarEnStorageEnIndex(valor, *index);

	} else {
		return NULL;
	}
}

void * guardarEnStorageEnIndex(void * valor, int index) {
	int tamValor = strlen(valor);

	int entradasNecesaria = entradasNecesariaParaUnTamanio(tamValor);

	int i;

	for (i = 0; i < entradasNecesaria; i++)
		ocuparIndex(index + i);

	memcpy(storage + (index * tamanioEntrada), valor, tamValor);

	return storage + (index * tamanioEntrada);
}

void compactar(void) {
	pthread_mutex_lock(&mutexDumpCompactacion);

	int i;
	int primeraEntradaLibre;

	for (i = 0; i < cantEntradas; i++) {
		if (!bitMap[i]) {

			primeraEntradaLibre = i;

			for (; i < cantEntradas && !bitMap[i]; i++)
				;

			if (i == cantEntradas)
				break;

			int indexOcupado = i;

			t_tabla_entradas * entrada = buscarEntradaSegunIndex(i);

			int cantidadEntradas = entrada->tamanio / tamanioEntrada;

			if (entrada->tamanio % tamanioEntrada != 0)
				cantidadEntradas++;

			entrada->indexComienzo = primeraEntradaLibre;

			void * valor = buscarValorSegunClave(entrada->clave);

			entrada->entrada = guardarEnStorageEnIndex(valor,
					primeraEntradaLibre);

			free(valor);

			for (int i2 = 0; i2 < cantidadEntradas; i2++) {
				ocuparIndex(primeraEntradaLibre);
				liberarIndex(indexOcupado);
				primeraEntradaLibre++;
				indexOcupado++;
			}
		}

	}
	pthread_mutex_unlock(&mutexDumpCompactacion);
}

/*-------------------------Dump-------------------------*/
void dump(void) {

	pthread_mutex_lock(&mutexDumpCompactacion);

	mkdir(puntoMontaje, 0777);

	list_iterate(tablaEntradas, (void*) almacenarEnMemoriaSecundaria);

	pthread_mutex_unlock(&mutexDumpCompactacion);
}

void almacenamientoContinuo(void) {
	while (almacenar) {
		sleep(intervaloDump);
		dump();
	}
}

void crearAlmacenamientoContinuo(void) {
	pthread_mutex_init(&mutexDumpCompactacion, NULL);

	if (pthread_create(&threadAlmacenamientoContinuo, NULL,
			(void*) almacenamientoContinuo, NULL)) {
		perror("Error el crear el thread almacenamientoContinuo.");
		exit(EXIT_FAILURE);
	}

}

void recuperarInformacionDeInstancia(t_list * listaClaves) {
	t_list * listaArchivos = listarArchivosDeMismaCarpeta(puntoMontaje);

	almacenar = true;

	if (listaArchivos == NULL) {
		log_warning(logInstancia,
				"No tengo archivos para recuperar inormacion de instancia anterior \n");
		return;
	}

	if (listaArchivos->elements_count == 0) {
		log_warning(logInstancia,
				"No tengo archivos para recuperar inormacion de instancia anterior \n");
		list_destroy(listaArchivos);
		return;
	}

	bool esArchivoARecuperar(char * rutaArchivo) {

		char ** spliteado = string_split(rutaArchivo, "/");

		int i;
		for (i = 0; spliteado[i] != NULL; i++)
			;

		bool esClaveBuscada(char * clave) {
			return string_equals_ignore_case(spliteado[i - 1], clave);
		}

		bool laEncontre = list_any_satisfy(listaClaves,
				(void *) esClaveBuscada);

		for (i = 0; spliteado[i] != NULL; ++i) {
			free(spliteado[i]);
		}
		free(spliteado[i]);
		free(spliteado);

		return laEncontre;
	}

	t_list * archivosARecuperar = list_filter(listaArchivos,
			(void *) esArchivoARecuperar);

	void guardarArchivoEnEstructurasAdministrativas(char * rutaArchivo) {
		size_t tamArch;
		FILE * archivofd;

		printf("El archivo a recuperar es: %s \n", rutaArchivo);

		void * archivo = abrirArchivo(rutaArchivo, &tamArch, &archivofd);

		char ** spliteado = string_split(rutaArchivo, "/");

		int i;

		for (i = 0; spliteado[i] != NULL; i++)
			;

		if (tamArch != 0)
			agregarClaveValor(spliteado[i - 1], archivo);

		for (i = 0; spliteado[i] != NULL; ++i) {
			free(spliteado[i]);
		}
		free(spliteado[i]);
		free(spliteado);

		munmap(archivo, tamArch);
		fclose(archivofd);

	}

	list_iterate(archivosARecuperar,
			(void*) guardarArchivoEnEstructurasAdministrativas);

	log_trace(logInstancia,
			"Tengo archivos para recuperar inormacion de instancia anterior");

	list_destroy(archivosARecuperar);
	list_destroy_and_destroy_elements(listaArchivos, (void *) free);

	mostrarTablaEntradas();
}

void almacenarEnMemoriaSecundaria(t_tabla_entradas * registroEntrada) {
	if (registroEntrada->tamanio == 0)
		return;

	mkdir(puntoMontaje, 0777);

	char * rutaArchivo = string_new();
	string_append(&rutaArchivo, puntoMontaje);
	string_append(&rutaArchivo, "/");
	string_append(&rutaArchivo, registroEntrada->clave);

	FILE* file = fopen(rutaArchivo, "w+b");

	char * valor = buscarValorSegunClave(registroEntrada->clave);

	fwrite(valor, sizeof(char), registroEntrada->tamanio, file);

	fclose(file);

	free(rutaArchivo);
	free(valor);
}

/*-------------------------Algoritmos de reemplazo-------------------------*/
void algoritmoReemplazoCircular(char * clave, void * valor) {
	t_list * entradasAtomicas = ordenarEntradasAtomicasParaCircular();

	eliminarEntradasParaReemplazo(entradasAtomicas, valor);
}

void algoritmoReemplazoBiggestSpaceUsed(char * clave, void * valor) {
	t_list * entradasAtomicas = ordenarEntradasAtomicasParaBSU();

	eliminarEntradasParaReemplazo(entradasAtomicas, valor);
}

void algoritmoReemplazoLeastRecentlyUsed(char * clave, void * valor) {
	t_list * entradasAtomicas = ordenarEntradasAtomicasParaLRU();

	eliminarEntradasParaReemplazo(entradasAtomicas, valor);
}

t_list * ordenarEntradasAtomicasParaCircular(void) {
	t_list * entradasAtomicas = filtrarEntradasAtomicas();

	bool ordenarMenorMayorSegunIndex(t_tabla_entradas * entrada,
			t_tabla_entradas * entradaMenor) {
		return entrada->indexComienzo < entradaMenor->indexComienzo;
	}

	list_sort(entradasAtomicas, (void*) ordenarMenorMayorSegunIndex);

	bool mayoresAEntradaAReemplazar(t_tabla_entradas * registroTabla) {

		return (registroTabla->indexComienzo >= entradaAReemplazar);
	}

	t_list * mayoresAReemplazar = list_filter(entradasAtomicas,
			(void*) mayoresAEntradaAReemplazar);

	bool menoresAEntradaAReemplazar(t_tabla_entradas * registroTabla) {

		return (registroTabla->indexComienzo < entradaAReemplazar);
	}

	t_list * menoresAReemplazar = list_filter(entradasAtomicas,
			(void*) menoresAEntradaAReemplazar);

	list_add_all(mayoresAReemplazar, menoresAReemplazar);

	list_destroy(entradasAtomicas);
	list_destroy(menoresAReemplazar);

	return mayoresAReemplazar;

}

t_list * ordenarEntradasAtomicasParaBSU(void) {
	t_list * entradasAtomicas = filtrarEntradasAtomicas();

	bool ordenarMenorMayorSegunTamanio(t_tabla_entradas * entrada,
			t_tabla_entradas * entradaMayor) {

		if (entrada->tamanio == entradaMayor->tamanio) {
			t_list * listaDesempate = desempate(entrada, entradaMayor);
			t_tabla_entradas * primera = list_get(listaDesempate, 1);
			bool resultado = string_equals_ignore_case(primera->clave,
					entradaMayor->clave);
			list_destroy(listaDesempate);
			return resultado;
		}

		return entrada->tamanio > entradaMayor->tamanio;
	}

	list_sort(entradasAtomicas, (void*) ordenarMenorMayorSegunTamanio);

	return entradasAtomicas;
}

t_list * ordenarEntradasAtomicasParaLRU(void) {
	t_list * entradasAtomicas = filtrarEntradasAtomicas();

	bool ordenarMenorMayorSegunTiempoReferenciado(t_tabla_entradas * entrada,
			t_tabla_entradas * entradaMayor) {

		if (entrada->tiempoReferenciado == entradaMayor->tiempoReferenciado) {
			t_list * listaDesempate = desempate(entrada, entradaMayor);
			t_tabla_entradas * primera = list_get(listaDesempate, 1);
			bool resultado = string_equals_ignore_case(primera->clave,
					entradaMayor->clave);
			list_destroy(listaDesempate);
			return resultado;
		}

		return entrada->tiempoReferenciado > entradaMayor->tiempoReferenciado;
	}

	list_sort(entradasAtomicas,
			(void*) ordenarMenorMayorSegunTiempoReferenciado);

	return entradasAtomicas;
}

void eliminarEntradasParaReemplazo(t_list * entradasAtomicas, void * valor) {
	int entradasNecesarias = entradasNecesariaParaUnTamanio(
			string_length(valor));

	int i = 0;

	for (int a = cantidadIndexLibres();
			a < entradasNecesarias && a < entradasAtomicas->elements_count;
			a++, i++) {
		t_tabla_entradas * entrada = list_get(entradasAtomicas, i);
		eliminarClave(entrada->clave);
		entrada = list_get(entradasAtomicas, i + 1);
		if (entrada != NULL)
			entradaAReemplazar = entrada->indexComienzo;
	}

	list_destroy(entradasAtomicas);

}

t_list * desempate(t_tabla_entradas * entrada, t_tabla_entradas * entrada2) {
	t_list * entradasEmpatadas = list_create();

	list_add(entradasEmpatadas, entrada);
	list_add(entradasEmpatadas, entrada2);

	bool ordenarMenorMayorSegunIndex(t_tabla_entradas * entrada,
			t_tabla_entradas * entradaMenor) {
		return entrada->indexComienzo < entradaMenor->indexComienzo;
	}

	list_sort(entradasEmpatadas, (void*) ordenarMenorMayorSegunIndex);

	bool mayoresAEntradaAReemplazar(t_tabla_entradas * registroTabla) {

		return (registroTabla->indexComienzo >= entradaAReemplazar);
	}

	t_list * mayoresAReemplazar = list_filter(entradasEmpatadas,
			(void*) mayoresAEntradaAReemplazar);

	bool menoresAEntradaAReemplazar(t_tabla_entradas * registroTabla) {

		return (registroTabla->indexComienzo < entradaAReemplazar);
	}

	t_list * menoresAReemplazar = list_filter(entradasEmpatadas,
			(void*) menoresAEntradaAReemplazar);

	list_add_all(mayoresAReemplazar, menoresAReemplazar);

	list_destroy(entradasEmpatadas);
	list_destroy(menoresAReemplazar);

	return mayoresAReemplazar;
}

/*-------------------------Señales-------------------------*/
void procesarSIGINT(void) {
	log_error(logInstancia, "Me llego signal: SIGINT y mato el proceso\n");

	enviarAvisoDesconexion(socketCoordinador);

	//Corto el hilo de almacenamiento
	almacenar = false;
	intervaloDump = 0;
	log_warning(logInstancia, "Espero para hacer el ultimo dump \n");
	pthread_cancel(threadAlmacenamientoContinuo);
	pthread_join(threadAlmacenamientoContinuo, NULL);

	//Termina esi
	log_trace(logInstancia, "Termino el proceso instancia \n");

	//Destruyo archivo de log
	log_destroy(logInstancia);

	//Libero memoria
	destruirTablaEntradas();
	destruirBitMap();
	destruirStorage();
	free(puntoMontaje);
	free(algoritmoReemplazo);
	pthread_mutex_destroy(&mutexDumpCompactacion);

	exit(EXIT_FAILURE);

}

/*-------------------------Funciones auxiliares-------------------------*/
void * abrirArchivo(char * rutaArchivo, size_t * tamArc, FILE ** archivo) {
//Abro el archivo
	*archivo = fopen(rutaArchivo, "r");

	if (*archivo == NULL) {
		log_error(logInstancia, "%s: No existe el archivo", rutaArchivo);
		perror("Error al abrir el archivo");
		exit(EXIT_FAILURE);
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

t_list * listarArchivosDeMismaCarpeta(char * ruta) {

	DIR *dir;

	struct dirent *ent;

	dir = opendir(ruta);

	if (dir == NULL)
		return NULL;

	t_list * listaArchivos = list_create();

	while ((ent = readdir(dir)) != NULL) {
		if (ent->d_name[0] != '.') {
			char * archivo = string_new();
			string_append(&archivo, ruta);
			string_append(&archivo, "/");
			string_append(&archivo, (char*) ent->d_name);
			list_add(listaArchivos, archivo);
		}
	}

	closedir(dir);

	return listaArchivos;
}

int entradasNecesariaParaUnTamanio(int tamanio) {
	int entradasNecesaria = tamanio / tamanioEntrada;

	if (tamanio % tamanioEntrada != 0)
		entradasNecesaria++;

	return entradasNecesaria;
}

t_list * filtrarEntradasAtomicas(void) {
	bool entradaAtomica(t_tabla_entradas * registroTabla) {
		int entradasNecesarias = registroTabla->tamanio / tamanioEntrada;

		if (registroTabla->tamanio % tamanioEntrada != 0)
			entradasNecesarias++;

		return (entradasNecesarias == 1);
	}

	t_list * listaFiltrada = list_filter(tablaEntradas, (void*) entradaAtomica);

	bool ordenarMenorMayor(t_tabla_entradas * entrada,
			t_tabla_entradas * entradaMenor) {
		return entrada->indexComienzo < entradaMenor->indexComienzo;
	}

	list_sort(listaFiltrada, (void*) ordenarMenorMayor);

	return listaFiltrada;
}
