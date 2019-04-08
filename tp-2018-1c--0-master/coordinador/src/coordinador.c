#include "coordinador.h"

int main(void) {

	signal(SIGINT, signal_handler);
	signal(SIGUSR1, signal_handler);
	g_tablaDeInstancias = crearListaInstancias();
	g_diccionarioConexiones = crearDiccionarioConexiones();
	g_diccionarioClaves = crearDiccionarioClaves();
	g_configuracion = malloc(sizeof(t_configuraciones));
	g_respuesta = true;

	g_config = config_create(PATH_CONFIG);
	armarConfigCoordinador(g_configuracion, g_config);

	g_logger = log_create("coordinador.log", "coordinador", true,
			LOG_LEVEL_TRACE);

	g_loggerDebug = log_create("coordinador.log", "coordinador",
	false, LOG_LEVEL_TRACE);

	sem_init(&g_mutexLog, 0, 1);
	sem_init(&g_mutex_tablas, 0, 1);
	sem_init(&g_mutex_respuesta_set, 0, 0);
	sem_init(&g_mutex_respuesta_store, 0, 0);
	sem_init(&g_peticion, 0, 1);

	iniciarServidor(g_configuracion->puertoConexion);

	return 0;
}

int iniciarServidor(char* puerto) {

	struct sockaddr_storage their_addr;
	struct addrinfo hints, *res;
	int status, sockfd;
	socklen_t addr_size;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((status = getaddrinfo(NULL, puerto, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	agregarConexion(g_diccionarioConexiones, "coordinador", sockfd);

	int yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	bind(sockfd, res->ai_addr, res->ai_addrlen);

	//TODO cerrar o free adrrinfo

	if (-1 == listen(sockfd, 10))
		perror("listen");
	printf("esperando conexiones en puerto %s\n", puerto);
	while (1) {
		addr_size = sizeof(their_addr);
		int* cliente_fd = malloc(sizeof(int));
		*cliente_fd = accept(sockfd, (struct sockaddr*) &their_addr,
				&addr_size);

		pthread_t pid;
		pthread_create(&pid, NULL, procesarPeticion, cliente_fd);

	}
}

void* procesarPeticion(void* cliente_fd) {

	while (1) {
		char* buffer = calloc(1000, sizeof(char));
		int recvError;
		memset(buffer, '$', 1000);

		if ((recvError = recv(*(int*) cliente_fd, buffer, 1000, 0)) <= 0) {
			procesarClienteDesconectado(*(int*) cliente_fd);
			free(buffer);
			pthread_exit(0);
			return 0;
		}

		int desplazamiento = 0;
		while (buffer[desplazamiento + 1] != '$') {
			int tamanio = 0;
			memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
			desplazamiento += sizeof(int);
			void* bufferPaquete = malloc(tamanio);
			memcpy(bufferPaquete, buffer + desplazamiento, tamanio);
			desplazamiento += tamanio;
			t_paquete* unPaquete = crearPaquete(bufferPaquete);
			procesarPaquete(unPaquete, *(int*) cliente_fd);
		}
		free(buffer);
	}
}

void procesarPaquete(t_paquete* paquete, int cliente_fd) {

	switch (paquete->codigoOperacion) {

	case HANDSHAKE:
		procesarHandshake(paquete, cliente_fd);
		break;

	case ENVIAR_NOMBRE_INSTANCIA:
		procesarNombreInstancia(paquete, cliente_fd);
		break;

	case ENVIAR_NOMBRE_ESI:
		procesarNombreESI(paquete, cliente_fd);
		break;

	case SET:
		usleep(g_configuracion->retardo * 1000);
		bloquearPeticion(&g_peticion);
		procesarSET(paquete, cliente_fd);
		break;

	case GET:
		usleep(g_configuracion->retardo * 1000);
		bloquearPeticion(&g_peticion);
		procesarGET(paquete, cliente_fd);
		break;

	case STORE:
		usleep(g_configuracion->retardo * 1000);
		bloquearPeticion(&g_peticion);
		procesarSTORE(paquete, cliente_fd);
		break;

	case RESPUESTA_SOLICITUD:
		procesarRespuesta(paquete, cliente_fd);
		break;

	case ENVIAR_CLAVE_ELIMINADA:
		procesarClaveEliminada(paquete, cliente_fd);
		break;

	case COMPACTAR:
		procesarCompactar(paquete, cliente_fd);
		break;

	case SOLICITAR_STATUS:
		procesarStatus(paquete, cliente_fd);
		break;

	case SOLICITAR_VALOR:
		procesarValor(paquete, cliente_fd);
		break;

	default:
		printf("codigo no reconocido: %d\n",paquete->codigoOperacion);
		break;
	}

	destruirPaquete(paquete);

}

void procesarHandshake(t_paquete* paquete, int cliente_fd) {

	switch (recibirHandshake(paquete)) {
	case PLANIFICADOR:
		logSeguro("info", g_mutexLog, "se conecto el planificador: %i",
				cliente_fd);
		agregarConexion(g_diccionarioConexiones, "planificador", cliente_fd);
		break;

	case ESI:
		logSeguro("debug", g_mutexLog, "se conecto un esi: %i", cliente_fd);
		break;

	case INSTANCIA:
		logSeguro("debug", g_mutexLog, "se conecto la instancia: %i",
				cliente_fd);
		break;

	}
}

void procesarRespuesta(t_paquete* paquete, int cliente_fd) {

	int respuesta = recibirRespuesta(paquete);
	t_conexion* conexionCliente = buscarConexion(g_diccionarioConexiones, NULL,
			cliente_fd);
	t_instancia* instanciaCliente = buscarInstancia(g_tablaDeInstancias, false,
			conexionCliente->nombre, 0, NULL);
	bool esPlanificadorElCliente = (strcmp(conexionCliente->nombre,
			"planificador") == 0);

	switch (respuesta) {

	case ERROR_ESPACIO_INSUFICIENTE:

		logSeguro("error", g_mutexLog, "error espacio insuficiente %s",
				instanciaCliente->nombre);

		desbloquearInstancia(instanciaCliente);

		bloquearTodasLasInstancias(g_tablaDeInstancias);

		logSeguro("debug", g_mutexLog, "bloqueadas todas instancias");

		compactarTodasLasInstancias(g_tablaDeInstancias,
				g_diccionarioConexiones);

		break;

	case ERROR_TAMANIO_CLAVE:
		logSeguro("error", g_mutexLog, "%s mando error de tamanio de clave",
				conexionCliente->nombre);

		break;

	case SET_OK:
		if (esPlanificadorElCliente) {
			sem_post(&g_mutex_respuesta_set);
			g_respuesta = true;
			logSeguro("debug", g_mutexLog, "%s acepto el SET",
					conexionCliente->nombre);
		} else {
			t_sentencia* TrabajoInstanciaProcesada = conseguirTrabajoActual(
					instanciaCliente);
			logSeguro("debug", g_mutexLog, "le llego el SET %s a %s",
					TrabajoInstanciaProcesada->clave, conexionCliente->nombre);

			agregarClaveDeInstancia(instanciaCliente,
					TrabajoInstanciaProcesada->clave);
			desbloquearInstancia(instanciaCliente);
//			free(TrabajoInstanciaProcesada->clave);
//			free(TrabajoInstanciaProcesada->valor);
//			//free(TrabajoInstanciaProcesada);
			desbloquearPeticion(&g_peticion);
		}

		break;

	case SET_DEFINITIVO_OK:
		;
		logSeguro("debug", g_mutexLog, "%s acepto el SET DEFINITIVO",
				instanciaCliente->nombre);
		desbloquearTodasLasInstancias(g_tablaDeInstancias);
		desbloquearPeticion(&g_peticion);
		break;

	case SET_DEFINITIVO_ERROR:
		;
		logSeguro("error", g_mutexLog, "%s no acepto el SET DEFINITIVO",
				instanciaCliente->nombre);
		desbloquearTodasLasInstancias(g_tablaDeInstancias);
		desbloquearPeticion(&g_peticion);
		break;

	case SET_ERROR:
		if (esPlanificadorElCliente) {
			sem_post(&g_mutex_respuesta_set);
			g_respuesta = false;
			logSeguro("warning", g_mutexLog, "%s no acepto el SET",
					conexionCliente->nombre);
		} else {
			logSeguro("error", g_mutexLog, "hubo un error con el SET de la %s",
					conexionCliente->nombre);
			desbloquearInstancia(instanciaCliente);
			sem_post(&g_peticion);
		}

		break;

	case STORE_OK:
		if (esPlanificadorElCliente) {
			sem_post(&g_mutex_respuesta_store);
			g_respuesta = true;
			logSeguro("debug", g_mutexLog, "%s acepto el STORE",
					conexionCliente->nombre);
		} else {
			logSeguro("debug", g_mutexLog, "le llego el STORE a %s",
					conexionCliente->nombre);
			desbloquearInstancia(instanciaCliente);
			desbloquearPeticion(&g_peticion);
			;

		}
		break;

	case STORE_ERROR:
		if (esPlanificadorElCliente) {
			sem_post(&g_mutex_respuesta_store);
			g_respuesta = false;
			logSeguro("warning", g_mutexLog, "%s no acepto el STORE",
					conexionCliente->nombre);
		} else {
			logSeguro("error", g_mutexLog,
					"hubo un error con el STORE de la %s",
					conexionCliente->nombre);
			desbloquearInstancia(instanciaCliente);
			desbloquearPeticion(&g_peticion);
			;
		}
		break;
	}
}

void procesarGET(t_paquete* paquete, int cliente_fd) {

	char* clave = recibirGet(paquete);
	agregarClaveAlSistema(g_diccionarioClaves, clave);
	t_conexion* conexionDelPlanificador = buscarConexion(
			g_diccionarioConexiones, "planificador", 0);
	enviarGet(conexionDelPlanificador->socket, clave);
	logSeguro("info", g_mutexLog, "enviar GET al planificador: %i, clave: %s\n",
			conexionDelPlanificador->socket, clave);
	free(clave);
	desbloquearPeticion(&g_peticion);

}

void procesarSET(t_paquete* paquete, int cliente_fd) {

	t_claveValor* sentencia = recibirSet(paquete);
	t_conexion* conexionDelPlanificador = buscarConexion(
			g_diccionarioConexiones, "planificador", 0);
	g_tiempoPorEjecucion = g_tiempoPorEjecucion + 1;

	if (!existeClaveEnSistema(g_diccionarioClaves, sentencia->clave)) {
		logSeguro("warning", g_mutexLog, "SET - la clave %s no existe.",
				sentencia->clave);
		enviarRespuesta(conexionDelPlanificador->socket,
				ERROR_CLAVE_NO_IDENTIFICADA);
	} else {

		logSeguro("debug", g_mutexLog,
				"preguntar por SET %s %s al planificador", sentencia->clave,
				sentencia->valor);
		enviarSet(conexionDelPlanificador->socket, sentencia->clave,
				sentencia->valor);
		sem_wait(&g_mutex_respuesta_set); //espera respuesta set

		t_instancia* instanciaElegida = buscarInstancia(g_tablaDeInstancias,
		false, NULL, 0, sentencia->clave);
		if (instanciaElegida == NULL) {
			instanciaElegida = PlanificarInstancia(
					g_configuracion->algoritmoDist, sentencia->clave,
					g_tablaDeInstancias);
		}

		if (g_respuesta == true) {

			if (instanciaElegida == NULL) {
				logSeguro("error", g_mutexLog,
						"no se pudo planificar una instancia para la clave: %s",
						sentencia->clave);
				enviarRespuesta(conexionDelPlanificador->socket,
						ERROR_CLAVE_INACCESIBLE);
				return;
			}

			t_conexion* conexionDeInstancia = buscarConexion(
					g_diccionarioConexiones, instanciaElegida->nombre, 0);

			instanciaElegida->ultimaModificacion = g_tiempoPorEjecucion;

			agregarTrabajoActual(instanciaElegida, sentencia->clave,
					sentencia->valor);
			bloquearInstancia(instanciaElegida);
			logSeguro("info", g_mutexLog, "ENVIAR SET %s %s a %s",
					sentencia->clave, sentencia->valor,
					instanciaElegida->nombre);
			enviarSet(conexionDeInstancia->socket, sentencia->clave,
					sentencia->valor);

		}

		free(sentencia->clave);
		free(sentencia->valor);
		free(sentencia);
	}
}

void procesarSTORE(t_paquete* paquete, int cliente_fd) {

	char* clave = recibirStore(paquete);
	t_conexion* conexionDelPlanificador = buscarConexion(
			g_diccionarioConexiones, "planificador", 0);

	logSeguro("debug", g_mutexLog, "preguntar por STORE %s al planificador",
			clave);
	enviarStore(conexionDelPlanificador->socket, clave);
	g_tiempoPorEjecucion = g_tiempoPorEjecucion + 1;
	sem_wait(&g_mutex_respuesta_store); //espera respuesta store del planificador

	if (g_respuesta == false) {
		return;
	}

	t_instancia* instanciaElegida = buscarInstancia(g_tablaDeInstancias, false,
	NULL, 0, clave);

	if (instanciaElegida == NULL) { //si no esta busca en las desconectadas
		instanciaElegida = buscarInstancia(g_tablaDeInstancias, true, NULL, 0,
				clave);
	}
	if (!existeClaveEnSistema(g_diccionarioClaves, clave)) {
		logSeguro("error", g_mutexLog, "STORE - la clave %s no existe.", clave);
		enviarRespuesta(conexionDelPlanificador->socket,
				ERROR_CLAVE_NO_IDENTIFICADA);
		return;
	}
	if (instanciaElegida == NULL) {
		logSeguro("warning", g_mutexLog,
				"la clave %s no se encuentra en ninguna instancia.", clave); //no hacer nada
		return;
	}

	if (instanciaElegida->disponible == false) {
		logSeguro("error", g_mutexLog,
				"la clave %s se encuentra en la %s pero esta desconectada.",
				clave, instanciaElegida->nombre);
		enviarRespuesta(conexionDelPlanificador->socket,
				ERROR_CLAVE_INACCESIBLE);
		return;
	}

	t_conexion* conexionDeInstancia = buscarConexion(g_diccionarioConexiones,
			instanciaElegida->nombre, 0);

	bloquearInstancia(instanciaElegida);
	enviarStore(conexionDeInstancia->socket, clave);
	logSeguro("info", g_mutexLog, "ENVIAR STORE %s a %s\n", clave,
			instanciaElegida->nombre);

	free(clave);
}

void procesarNombreInstancia(t_paquete* paquete, int cliente_fd) {

	char* nombre = recibirNombreInstancia(paquete);
	t_instancia* instanciaNueva = buscarInstancia(g_tablaDeInstancias, true,
			nombre, 0, NULL);

	if (instanciaNueva == NULL) {
		instanciaNueva = crearInstancia(nombre,
				g_configuracion->cantidadEntradas,
				g_configuracion->tamanioEntradas);
		agregarInstancia(g_tablaDeInstancias, instanciaNueva);
	}

	instanciaNueva->disponible = true;

	agregarConexion(g_diccionarioConexiones, instanciaNueva->nombre,
			cliente_fd);
	distribuirKeys(g_tablaDeInstancias);
	enviarInfoInstancia(cliente_fd, g_configuracion->cantidadEntradas,
			g_configuracion->tamanioEntradas, instanciaNueva->claves);
	logSeguro("info", g_mutexLog, "se conecto instancia: %s", nombre);

	free(nombre);
}

void procesarNombreESI(t_paquete* paquete, int cliente_fd) {

	char* nombreESI = recibirNombreEsi(paquete);

	agregarConexion(g_diccionarioConexiones, nombreESI, cliente_fd);
	logSeguro("info", g_mutexLog, "se conecto esi: %s", nombreESI);

	free(nombreESI);
}

void procesarClaveEliminada(t_paquete* paquete, int cliente_fd) {

	char* clave = recibirClaveEliminada(paquete);
	t_conexion* conexionDeInstancia = buscarConexion(g_diccionarioConexiones,
	NULL, cliente_fd);
	t_instancia* instanciaElegida = buscarInstancia(g_tablaDeInstancias,
	false, conexionDeInstancia->nombre, 0, NULL);
	eliminiarClaveDeInstancia(instanciaElegida, clave);
	logSeguro("info", g_mutexLog, "se elimino la clave %s de %s", clave,
			instanciaElegida->nombre);

	free(clave);

}

void procesarCompactar(t_paquete* paquete, int cliente_fd) {

	t_conexion* conexionCliente = buscarConexion(g_diccionarioConexiones, NULL,
			cliente_fd);
	t_instancia* instanciaCliente = buscarInstancia(g_tablaDeInstancias, false,
			conexionCliente->nombre, 0, NULL);

	logSeguro("debug", g_mutexLog, "%s termino compactacion",
			instanciaCliente->nombre);
	instanciasCompactadas--;

	if (instanciasCompactadas == 0) {
		logSeguro("debug", g_mutexLog, "ENVIAR SET DEFINITIVO a %s",
				instanciaCliente->nombre);
		t_instancia* instanciaConErrorSet = traerUltimaInstanciaUsada(
				g_tablaDeInstancias);
		t_sentencia* trabajoInstanciaErrorSet = conseguirTrabajoActual(
				instanciaConErrorSet);

		enviarSetDefinitivo(conexionCliente->socket,
				trabajoInstanciaErrorSet->clave,
				trabajoInstanciaErrorSet->valor);

		//free(trabajoInstanciaErrorSet->clave);
		//free(trabajoInstanciaErrorSet->valor);
		//free(trabajoInstanciaErrorSet);
		desbloquearPeticion(&g_peticion);
	}
	desbloquearInstancia(instanciaCliente);

}

void procesarStatus(t_paquete* paquete, int cliente_fd) {

	char *clave = recibirSolicitudStatus(paquete);

	char *nombreInstanciaActual;

	t_instancia *instanciaActual = buscarInstancia(g_tablaDeInstancias, true,
	NULL, 0, clave);

	if (instanciaActual == NULL)
		nombreInstanciaActual = "";
	else
		nombreInstanciaActual = instanciaActual->nombre;

	if (nombreInstanciaActual == NULL) {
		t_instancia *instanciaPosible = PlanificarInstancia(
				g_configuracion->algoritmoDist, clave, g_tablaDeInstancias);
		if(instanciaPosible == NULL)
			enviarRespuestaStatus(cliente_fd, "", "", "");
		else
			enviarRespuestaStatus(cliente_fd, "", "", instanciaPosible->nombre);
		return;
	}

	t_conexion* conexionInstancia = buscarConexion(g_diccionarioConexiones,
			nombreInstanciaActual, 0);
	enviarSolicitudValor(conexionInstancia->socket, clave);
}

void procesarValor(t_paquete * paquete, int cliente_fd) {

	char * valor = recibirSolicitudValor(paquete);

	char *nombreInstanciaPosible;

	t_conexion* conexionInstancia = buscarConexion(g_diccionarioConexiones,NULL,cliente_fd);
	t_instancia* instanciaActual = buscarInstancia(g_tablaDeInstancias,true,conexionInstancia->nombre,0,NULL);

	t_instancia *instanciaPosible = PlanificarInstancia(
			g_configuracion->algoritmoDist, valor, g_tablaDeInstancias);

	if (instanciaPosible == NULL)
		nombreInstanciaPosible = "";
	else
		nombreInstanciaPosible = instanciaPosible->nombre;

	logSeguro("debug", g_mutexLog, "status %s %s", instanciaActual->nombre,
			nombreInstanciaPosible);

	enviarRespuestaStatus(cliente_fd, valor, instanciaActual->nombre,
			nombreInstanciaPosible);

	free(valor);
}

void logSeguro(char* nivelLog, sem_t mutexLog, char* format, ...) {

	va_list ap;
	va_start(ap, format);
	char* mensaje = string_from_vformat(format, ap);
	sem_wait(&mutexLog);

	if (string_equals_ignore_case("trace", nivelLog)) {
		if (g_configuracion->logDebug)
			log_trace(g_logger, mensaje);
		else
			log_trace(g_loggerDebug, mensaje);
	}

	if (string_equals_ignore_case("debug", nivelLog)) {
		if (g_configuracion->logDebug)
			log_debug(g_logger, mensaje);
		else
			log_debug(g_loggerDebug, mensaje);
	}

	if (string_equals_ignore_case("info", nivelLog)) {
		log_info(g_logger, mensaje);
	}

	if (string_equals_ignore_case("warning", nivelLog)) {
		log_warning(g_logger, mensaje);
	}

	if (string_equals_ignore_case("error", nivelLog)) {
		log_error(g_logger, mensaje);
	}

	sem_post(&mutexLog);
	free(mensaje);
}

void signal_handler(int signum) {

	if (signum == SIGINT) {
		logSeguro("error", g_mutexLog, "Cerrando conexiones\n");
		cerrarTodasLasConexiones(g_diccionarioConexiones);

		list_destroy_and_destroy_elements(g_tablaDeInstancias,
				(void*) destruirInstancia);

		sem_destroy(&g_mutexLog);
		sem_destroy(&g_mutex_tablas);
		sem_destroy(&g_mutex_respuesta_set);
		sem_destroy(&g_mutex_respuesta_store);
		config_destroy(g_config);
		exit(0);
	}
	if (signum == SIGUSR1) {
		config_destroy(g_config);
		g_config = config_create(PATH_CONFIG);
		armarConfigCoordinador(g_configuracion, g_config);
		printf("algortimo planificaion actual: %s\n\n",
				g_configuracion->algoritmoDist);
		printf("INSTANCIAS----------------------\n");
		mostrarTablaInstancia(g_tablaDeInstancias);
		printf("CONEXIONES----------------------\n");
		mostrarDiccionario(g_diccionarioConexiones);
	}
}

void* procesarClienteDesconectado(int cliente_fd) {

	t_conexion* clienteDesconectado = buscarConexion(g_diccionarioConexiones,
	NULL, cliente_fd);
	if (clienteDesconectado == NULL) {
		return 0;
	}
	if (string_equals_ignore_case(clienteDesconectado->nombre,
			"planificador")) {
		logSeguro("error", g_mutexLog,
				"se desconecto %s\n\n\t\t --------ESTADO INSEGURO-------\n",
				clienteDesconectado->nombre);
		raise(SIGINT);
	} else {
		logSeguro("warning", g_mutexLog, "se desconecto %s\n",
				clienteDesconectado->nombre);
		t_instancia * instanciaDesconectada = buscarInstancia(
				g_tablaDeInstancias, false, clienteDesconectado->nombre, 0,
				NULL);
		if (instanciaDesconectada != NULL) { //entonces se desconecto un esi
			instanciaDesconectada->disponible = false;
			int sval;
			sem_getvalue(&instanciaDesconectada->instanciaMutex, &sval);
			if (sval < 1)
				sem_post(&instanciaDesconectada->instanciaMutex);
			distribuirKeys(g_tablaDeInstancias);
		}
		sacarConexion(g_diccionarioConexiones, clienteDesconectado);
	}
	return 0;
}

void compactarTodasLasInstancias(t_list* tablaDeInstancias,
		t_list* diccionarioConexiones) {
	instanciasCompactadas = 0;
	for (int i = 0; i < list_size(tablaDeInstancias); i++) {
		t_instancia* instanciaElegida = list_get(tablaDeInstancias, i);
		if (instanciaElegida->disponible) {
			t_conexion* instanciaConexion = buscarConexion(
					diccionarioConexiones, instanciaElegida->nombre, 0);
			logSeguro("debug", g_mutexLog, "enviar compactacion a %s",
					instanciaElegida->nombre);
			instanciasCompactadas++;
			enviarCompactacion(instanciaConexion->socket);

		}
	}
}

void armarConfigCoordinador(t_configuraciones* g_configuracion,
		t_config* archivoConfig) {

	g_configuracion->puertoConexion = config_get_string_value(archivoConfig,
			"PUERTO");
	g_configuracion->algoritmoDist = config_get_string_value(archivoConfig,
			"ALGORITMO_DISTRIBUCION");
	g_configuracion->cantidadEntradas = config_get_int_value(archivoConfig,
			"CANTIDAD_ENTRADAS");
	g_configuracion->tamanioEntradas = config_get_int_value(archivoConfig,
			"TAMANIO_ENTRADA");
	g_configuracion->retardo = config_get_int_value(archivoConfig, "RETARDO");
	g_configuracion->logDebug = config_get_int_value(archivoConfig,
			"DEBUG_MODE");

}

t_instancia* PlanificarInstancia(char* algoritmoDePlanificacion, char* clave,
		t_list* tablaDeInstancias) {

	sem_wait(&g_mutex_tablas);
	t_instancia* instanciaElegida = NULL;

	if (string_equals_ignore_case(algoritmoDePlanificacion, "LSU"))
		instanciaElegida = traerInstanciaMasEspacioDisponible(
				tablaDeInstancias);

	if (string_equals_ignore_case(algoritmoDePlanificacion, "EL"))
		instanciaElegida = traerUltimaInstanciaUsada(tablaDeInstancias);

	if (string_equals_ignore_case(algoritmoDePlanificacion, "KE")) {
		int keyDeClave = (int) string_substring(clave, 0, 1);
		instanciaElegida = buscarInstancia(tablaDeInstancias, false, NULL,
				keyDeClave, NULL);
	}

	sem_post(&g_mutex_tablas);

	return instanciaElegida;

}
