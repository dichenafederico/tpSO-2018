#include "planificador.h"

int g_intervaloReconexion;
char* g_claveBusqueda;
t_infoBloqueo* aBorrar;

int main(void) {

	signal(SIGINT, (void*) atenderCtrlC);
	g_con = config_create(RUTA_CONFIGURACION_PLANIF);
	g_logger = log_create("log.log", "Planificador", 1, LOG_LEVEL_TRACE);
	char* ip = config_get_string_value(g_con, "COORDINADOR_IP");
	int puertoCoordinador = config_get_int_value(g_con, "COORDINADOR_PUERTO");
	g_socketCoordinador = conectarCliente(ip, puertoCoordinador, PLANIFICADOR);

	if (g_socketCoordinador == -1) {
		log_error(g_logger,
				"El coordinador no esta conectado. Revise y vuelva a ejecutar");
		config_destroy(g_con);
		log_destroy(g_logger);
		return EXIT_FAILURE;
	}

	g_listos = dictionary_create();
	g_bloq = dictionary_create();
	g_clavesTomadas = dictionary_create();

	int puertoLocal = config_get_int_value(g_con, "PUERTO");

	g_keyMaxima = 0;

	g_est = config_get_double_value(g_con, "ESTIMACION_INICIAL");

	asignarBloquedas(config_get_array_value(g_con, "CLAVES_BLOQUEADAS"));

	char* algoritmo = config_get_string_value(g_con, "ALGORITMO_PLANIFICACION");
	g_alfa = (config_get_double_value(g_con, "ALFA") / 100);

	pthread_mutex_init(&mutexBloqueo, NULL);
	pthread_mutex_init(&mutexConsola, NULL);
	pthread_mutex_init(&mutexListo, NULL);
	pthread_mutex_init(&modificacion, NULL);
	pthread_mutex_init(&mutexLog, NULL);
	pthread_mutex_init(&mutexClavesTomadas, NULL);
	pthread_mutex_init(&mutexInstruccionConsola, NULL);

	sem_init(&ESIentrada, 0, 0);
	sem_init(&continua, 0, 0);

	g_termino = 0;
	g_bloqueo = 0;
	g_huboError = 0;
	g_instruccionConsola = 0;

	pthread_create(&hiloAlgoritmos, NULL, (void*) planificar, algoritmo);
	pthread_create(&hiloCoordinador, NULL, (void*) atenderCoordinador, NULL);
	pthread_create(&hiloServidor, NULL, (void*) iniciarServidor,
			(void*) &puertoLocal);

	log_debug(g_logger, "inicio consola");
	iniciarConsola();

	pthread_cancel(hiloCoordinador);
	pthread_join(hiloCoordinador, NULL);

	liberarTodo();

	return EXIT_SUCCESS;
}

void asignarBloquedas(char** codigos) {
	int i = 0;
	for (; codigos[i] != NULL; i++) {
		t_list* ins = list_create();
		dictionary_put(g_bloq, codigos[i], ins);
		free(codigos[i]);
	}
	free(codigos);
}

void procesarPaqueteESIs(t_paquete* unPaquete, int* socketCliente) {
	t_infoListos *dat;
	char* keyAux, *nombreAux;
	switch (unPaquete->codigoOperacion) {
	case HANDSHAKE:
		recibirHandshakePlanif(unPaquete, socketCliente);
		break;
	case ENVIAR_NOMBRE_ESI:
		if (g_keyMaxima < *socketCliente)
			g_keyMaxima = *socketCliente;
		dat = malloc(sizeof(t_infoListos));
		dat->estAnterior = g_est;
		dat->realAnterior = 0;
		dat->socketESI = *socketCliente;
		dat->tEnEspera = 0;
		dat->nombreESI = recibirNombreEsi(unPaquete);
		keyAux = string_itoa(*socketCliente);
		pthread_mutex_lock(&mutexListo);
		dictionary_put(g_listos, keyAux, dat);
		pthread_mutex_unlock(&mutexListo);
		free(keyAux);
		pthread_mutex_lock(&modificacion);
		g_huboModificacion = 1;
		pthread_mutex_unlock(&modificacion);
		sem_post(&ESIentrada);
		pthread_mutex_lock(&mutexLog);
		log_info(g_logger, "Se conecto exitosamente el %s", dat->nombreESI);
		pthread_mutex_unlock(&mutexLog);
		break;
	case TERMINO_ESI:
		g_termino = 1;
		break;
	case ENVIAR_ERROR:
		if (!g_termino && !g_huboError) {
			keyAux = string_itoa(*socketCliente);
			nombreAux = liberarESI(keyAux);
			pthread_mutex_lock(&mutexLog);
			log_warning(g_logger, "%s se ha desconectado sin haber ejecutado",
					nombreAux);
			pthread_mutex_unlock(&mutexLog);
			free(keyAux);
			free(nombreAux);
		} else {
			if (dictionary_has_key(g_clavesTomadas, g_nombreESIactual)) {
				list_destroy_and_destroy_elements(
						dictionary_remove(g_clavesTomadas, g_nombreESIactual),
						(void*) free);
			}
			sem_post(&continua);
		}
		break;
	}
	destruirPaquete(unPaquete);
}

static int existeClave(char* clave) {
	int existe = 0;
	void buscarEnbloq(char* cl, void* arg) {
		if (!existe)
			existe = !strcmp(clave, cl);
	}
	void buscarEnClavesTomadas(char* c, t_list* lista) {
		if (!existe)
			existe = list_any_satisfy(lista, (void*) condicionDeTomada);
	}
	pthread_mutex_lock(&mutexBloqueo);
	dictionary_iterator(g_bloq, (void*) buscarEnbloq);
	pthread_mutex_unlock(&mutexBloqueo);
	if (!existe) {
		pthread_mutex_lock(&mutexClavesTomadas);
		dictionary_iterator(g_clavesTomadas, (void*) buscarEnClavesTomadas);
		pthread_mutex_unlock(&mutexClavesTomadas);
	}
	return existe;
}

void procesarPaqueteCoordinador(t_paquete* unPaquete, int* socketCliente) {
	t_list* aux;
	switch (unPaquete->codigoOperacion) {
	case SET:
		pthread_mutex_lock(&mutexInstruccionConsola);
		if (!g_instruccionConsola) {
			g_claveTomada = 0;
			t_claveValor* recv = recibirSet(unPaquete);
			g_claveGET = recv->clave;
			if (dictionary_has_key(g_clavesTomadas, g_nombreESIactual)) {
				pthread_mutex_lock(&mutexClavesTomadas);
				g_claveTomada = list_any_satisfy(
						dictionary_get(g_clavesTomadas, g_nombreESIactual),
						(void*) condicionDeTomada);
				pthread_mutex_unlock(&mutexClavesTomadas);
			}
			if (g_claveTomada)
				enviarRespuesta(g_socketCoordinador, SET_OK);
			else {
				enviarRespuesta(g_socketCoordinador, SET_ERROR);
				g_huboError = 1;
				enviarRespuesta(g_socketEnEjecucion, ABORTO_ESI);
				liberarClaves(g_nombreESIactual);
				pthread_mutex_lock(&mutexLog);
				if (existeClave(g_claveGET))
					log_error(g_logger,
							"%s se aborta por SET sobre clave no tomada",
							g_nombreESIactual);
				else
					log_error(g_logger,
							"%s se aborta por SET sobre clave inexistente",
							g_nombreESIactual);
				pthread_mutex_unlock(&mutexLog);
			}
			free(recv->clave);
			free(recv->valor);
			free(recv);
			g_claveGET = NULL;
			sem_post(&continua);
		} else
			g_instruccionConsola = 0;
		pthread_mutex_unlock(&mutexInstruccionConsola);
		break;
	case GET:
		pthread_mutex_lock(&mutexInstruccionConsola);
		if (!g_instruccionConsola) {
			g_claveTomada = 0;
			g_claveGET = recibirGet(unPaquete);
			if (!esiTieneClave()) {
				pthread_mutex_lock(&mutexClavesTomadas);
				dictionary_iterator(g_clavesTomadas, (void*) claveEstaTomada);
				if (g_claveTomada || dictionary_has_key(g_bloq, g_claveGET)) {
					g_bloqueo = 1;
				} else {
					if (dictionary_has_key(g_clavesTomadas,
							g_nombreESIactual)) {
						list_add(
								dictionary_get(g_clavesTomadas,
										g_nombreESIactual), g_claveGET);
					} else {
						aux = list_create();
						list_add(aux, g_claveGET);
						dictionary_put(g_clavesTomadas, g_nombreESIactual, aux);
					}
					pthread_mutex_lock(&mutexLog);
					log_trace(g_logger, "%s ha tomado la clave %s exitosamente",
							g_nombreESIactual, g_claveGET);
					pthread_mutex_unlock(&mutexLog);
				}
				pthread_mutex_unlock(&mutexClavesTomadas);
			} else {
				pthread_mutex_lock(&mutexLog);
				log_trace(g_logger, "%s ya tiene la clave %s tomada",
						g_nombreESIactual, g_claveGET);
				pthread_mutex_unlock(&mutexLog);
				free(g_claveGET);
				g_claveGET = NULL;
			}
			sem_post(&continua);
		} else
			g_instruccionConsola = 0;
		pthread_mutex_unlock(&mutexInstruccionConsola);
		break;
	case STORE:
		pthread_mutex_lock(&mutexInstruccionConsola);
		if (!g_instruccionConsola) {
			g_claveTomada = 0;
			g_claveGET = recibirStore(unPaquete);
			if (dictionary_has_key(g_clavesTomadas, g_nombreESIactual)) {
				pthread_mutex_lock(&mutexClavesTomadas);
				g_claveTomada = list_any_satisfy(
						dictionary_get(g_clavesTomadas, g_nombreESIactual),
						(void*) condicionDeTomada);
				pthread_mutex_unlock(&mutexClavesTomadas);
			}
			if (g_claveTomada) {
				if (dictionary_has_key(g_bloq, g_claveGET))
					desbloquearESI(g_claveGET);
				pthread_mutex_lock(&mutexBloqueo);
				aux = dictionary_get(g_clavesTomadas, g_nombreESIactual);
				list_remove_and_destroy_by_condition(aux,
						(void*) condicionDeTomada, (void*) free);
				pthread_mutex_unlock(&mutexBloqueo);
				pthread_mutex_lock(&mutexLog);
				log_trace(g_logger, "%s ha liberado la clave %s exitosamente",
						g_nombreESIactual, g_claveGET);
				pthread_mutex_unlock(&mutexLog);
				enviarRespuesta(g_socketCoordinador, STORE_OK);
			} else {
				g_huboError = 1;
				enviarRespuesta(g_socketEnEjecucion, ABORTO_ESI);
				enviarRespuesta(g_socketCoordinador, STORE_ERROR);
				liberarClaves(g_nombreESIactual);
				pthread_mutex_lock(&mutexLog);
				if (existeClave(g_claveGET))
					log_error(g_logger,
							"%s se aborta por STORE sobre clave no tomada",
							g_nombreESIactual);
				else
					log_error(g_logger,
							"%s se aborta por STORE sobre clave inexistente",
							g_nombreESIactual);
				pthread_mutex_unlock(&mutexLog);
			}
			free(g_claveGET);
			g_claveGET = NULL;
			sem_post(&continua);
		} else
			g_instruccionConsola = 0;
		pthread_mutex_unlock(&mutexInstruccionConsola);
		break;
	case RESPUESTA_SOLICITUD:
		pthread_mutex_lock(&mutexInstruccionConsola);
		if (!g_instruccionConsola) {
			pthread_mutex_lock(&mutexLog);
			switch (recibirRespuesta(unPaquete)) {
			case ERROR_TAMANIO_CLAVE:
				log_error(g_logger,
						"Se aborta %s por clave mayor a 40 caracteres",
						g_nombreESIactual);
				break;
			case ERROR_CLAVE_NO_IDENTIFICADA:
				log_error(g_logger,
						"Se aborta %s por una clave no identificada",
						g_nombreESIactual);
				break;
			case ERROR_CLAVE_INACCESIBLE:
				log_error(g_logger, "Se aborta %s por clave inaccesible",
						g_nombreESIactual);
				break;
			case ERROR_ESPACIO_INSUFICIENTE:
				log_error(g_logger, "Se aborta %s por espacio insuficiente",
						g_nombreESIactual);
				break;
			}
			pthread_mutex_unlock(&mutexLog);
			g_huboError = 1;
			enviarRespuesta(g_socketEnEjecucion, ABORTO_ESI);
			liberarClaves(g_nombreESIactual);
			sem_post(&continua);
		} else
			g_instruccionConsola = 0;
		pthread_mutex_unlock(&mutexInstruccionConsola);
		break;
	case RESPUESTA_STATUS:
		mostrarPorConsola(recibirRespuestaStatus(unPaquete));
		break;
	case ENVIAR_ERROR:
		pthread_mutex_lock(&mutexLog);
		log_error(g_logger,
				"El coordinador se ha desconectado. Se aborta el planificador y sus ESIs");
		pthread_mutex_unlock(&mutexLog);
		destruirPaquete(unPaquete);
		liberarTodo();
		exit(EXIT_FAILURE);
		break;
	}
	destruirPaquete(unPaquete);
}

void atenderCoordinador(void* arg) {
	while (1) {
		gestionarSolicitudes(g_socketCoordinador,
				(void*) procesarPaqueteCoordinador, g_logger);
	}
}

int condicionDeTomada(char* nodo) {
	return !strcmp(nodo, g_claveGET);
}

void claveEstaTomada(char* key, t_list* value) {
	if (!g_claveTomada && strcmp(g_nombreESIactual, key) != 0)
		g_claveTomada = list_any_satisfy(value, (void*) condicionDeTomada);
}

void recibirHandshakePlanif(t_paquete* unPaquete, int* socketCliente) {
	int tipoCliente;
	memcpy(&tipoCliente, unPaquete->buffer->data, sizeof(int));
	switch (tipoCliente) {
	case ESI:
		break;
	default:
		*socketCliente = -1;
	}
}

void iniciarServidor(void* puerto) {
	iniciarServer(*(int*) puerto, (void*) procesarPaqueteESIs, g_logger);
}

void planificar(char* algoritmo) {
		if (strcmp(algoritmo, "SJF-SD") == 0 || strcmp(algoritmo, "HRRN") == 0)
				planificarSinDesalojo(algoritmo);
			else
				planificarConDesalojo();
}

int esiTieneClave(void) {
	if (dictionary_has_key(g_clavesTomadas, g_nombreESIactual))
		return list_any_satisfy(
				dictionary_get(g_clavesTomadas, g_nombreESIactual),
				(void*) condicionDeTomada);
	return 0;
}

void atenderCtrlC(void) {
	log_error(g_logger, "Me llego signal: SIGINT y mato el proceso\n");
	pthread_cancel(hiloCoordinador);
	pthread_join(hiloCoordinador, NULL);
	liberarTodo();
	exit(EXIT_FAILURE);
}
