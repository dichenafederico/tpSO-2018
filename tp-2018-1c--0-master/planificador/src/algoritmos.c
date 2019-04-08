#include "algoritmos.h"

static double calcularProximaRafaga(double estimadoAnterior,
		double realAnterior, double arg) {
	return estimadoAnterior * g_alfa + realAnterior * (1 - g_alfa);
}

static double calcularRR(double estimadoAnterior, double realAnterior,
		double tEnEspera) {
	return 1 + (tEnEspera/calcularProximaRafaga(estimadoAnterior, realAnterior, 0));
}

static int esMenor(int comp1, int comp2) {
	return comp1 < comp2;
}

static int esMayor(int comp1, int comp2) {
	return comp1 > comp2;
}

static char* asignarID(int *val) {
	char* auxKey = NULL;

	do {
		free(auxKey);
		auxKey = string_itoa(*val);
		(*val)++;
	} while (!dictionary_has_key(g_listos, auxKey) && *val <= g_keyMaxima);
	return auxKey;
}

static void bloquear(t_infoListos* bloq, int nuevoReal, char* key) {
	bloq->estAnterior = calcularProximaRafaga(bloq->estAnterior,
			bloq->realAnterior, 0);
	bloq->realAnterior = nuevoReal;
	t_infoBloqueo* insert = malloc(sizeof(t_infoBloqueo));
	insert->idESI = strdup(key);
	insert->data = bloq;
	pthread_mutex_lock(&mutexBloqueo);
	if (dictionary_has_key(g_bloq, g_claveGET)) {
		list_add(dictionary_get(g_bloq, g_claveGET), insert);
	} else {
		t_list* aux = list_create();
		list_add(aux, insert);
		dictionary_put(g_bloq, g_claveGET, aux);
	}
	pthread_mutex_unlock(&mutexBloqueo);
	pthread_mutex_lock(&mutexLog);
	log_trace(g_logger, "Se ha bloqueado %s bajo la clave %s",
			insert->data->nombreESI, g_claveGET);
	pthread_mutex_unlock(&mutexLog);
	free(g_claveGET);
	g_claveGET = NULL;
}

static void liberarSalida(void* arg) {
	/*free(g_claveGET);
	 free(g_idESIactual);*/
}

static char* calcularSiguiente(double (*calculadorProx)(double, double, double),
		int (*ponderacion)(int, int)) {
	t_infoListos *actual;
	double unValor;
	char* auxKey = NULL;
	char* key;
	int i = 0;
	auxKey = asignarID(&i);
	actual = dictionary_get(g_listos, auxKey);
	unValor = calculadorProx(actual->estAnterior, actual->realAnterior,
			actual->tEnEspera);
	key = strdup(auxKey);
	for (; i <= g_keyMaxima; i++) {
		free(auxKey);
		auxKey = asignarID(&i);
		if (dictionary_has_key(g_listos, auxKey)) {
			actual = dictionary_get(g_listos, auxKey);
			double prox = calculadorProx(actual->estAnterior,
					actual->realAnterior, actual->tEnEspera);
			if (ponderacion(unValor, prox)) {
				unValor = prox;
				free(key);
				key = strdup(auxKey);
			}
		}
	}
	log_trace(g_logger, "Se ejecuta %s",
			((t_infoListos*) (dictionary_get(g_listos, key)))->nombreESI);
	log_warning(g_logger, "estimacion: %.2f", unValor);
	free(auxKey);
	return key;
}

static void envejecer(char* key, t_infoListos* data) {
	data->tEnEspera += g_tEjecucion;
}

extern void planificarSinDesalojo(char* algoritmo) {
	int cont;
	t_infoListos *aEjecutar;
	char* key = NULL;
	pthread_cleanup_push((void*) liberarSalida, NULL)
				;
				while (1) {

					g_enEjecucion = NULL;
					cont = 0;
					g_huboError = 0;
					sem_wait(&ESIentrada);
					pthread_mutex_lock(&mutexConsola);
					pthread_mutex_unlock(&mutexConsola);
					pthread_mutex_lock(&mutexListo);
					if (strcmp(algoritmo, "SJF-SD") == 0)
						key = calcularSiguiente((void*) calcularProximaRafaga,
								(void*) esMayor);
					if (strcmp(algoritmo, "HRRN") == 0)
						key = calcularSiguiente((void*) calcularRR,
								(void*) esMenor);

					g_idESIactual = key;
					aEjecutar = dictionary_remove(g_listos, key);
					g_enEjecucion = aEjecutar;
					g_nombreESIactual = aEjecutar->nombreESI;
					pthread_mutex_unlock(&mutexListo);
					g_socketEnEjecucion = aEjecutar->socketESI;
					while (!g_termino && !g_bloqueo && !g_huboError) {
						pthread_mutex_lock(&mutexConsola);
						enviarSolicitudEjecucion(g_socketEnEjecucion);
						pthread_mutex_unlock(&mutexConsola);
						cont++;
						sem_wait(&continua);
					}
					if (g_bloqueo) {
						g_bloqueo = 0;
						bloquear(aEjecutar, cont, key);
						enviarBloqueoESI(g_socketEnEjecucion);
					}
					if (g_termino || g_huboError) {
						if (!g_huboError)
							enviarRespuesta(g_socketEnEjecucion, CONTINUA_ESI);
						log_trace(g_logger, "%s ha terminado su ejecucion",
								aEjecutar->nombreESI);
						sem_wait(&continua);
						free(aEjecutar->nombreESI);
						free(aEjecutar);
						g_termino = 0;
					}
					if (strcmp(algoritmo, "HRRN") == 0) {
						g_tEjecucion = cont;
						pthread_mutex_lock(&mutexListo);
						dictionary_iterator(g_listos, (void*) envejecer);
						pthread_mutex_unlock(&mutexListo);
					}
					free(key);
					g_idESIactual = NULL;
				}
				pthread_cleanup_pop(1);
}

extern void planificarConDesalojo(void) {
	pthread_cleanup_push((void*)liberarSalida, NULL)
				;
				int cont;
				t_infoListos *aEjecutar = NULL;
				char* key = NULL;
				sem_wait(&ESIentrada);
				g_huboModificacion = 0;
				while (1) {
					cont = 0;
					g_huboError = 0;
					pthread_mutex_lock(&mutexConsola);
					pthread_mutex_unlock(&mutexConsola);
					pthread_mutex_lock(&mutexListo);
					key = calcularSiguiente((void*) calcularProximaRafaga,
							(void*) esMayor);
					g_idESIactual = key;
					aEjecutar = dictionary_remove(g_listos, key);
					g_nombreESIactual = aEjecutar->nombreESI;
					pthread_mutex_unlock(&mutexListo);
					g_socketEnEjecucion = aEjecutar->socketESI;

					do {
						pthread_mutex_lock(&mutexConsola);
						enviarSolicitudEjecucion(g_socketEnEjecucion);
						pthread_mutex_unlock(&mutexConsola);
						cont++;
						sem_wait(&continua);
					} while (!g_termino && !g_bloqueo && !g_huboModificacion
							&& !g_huboError);
					if (g_bloqueo) {
						g_bloqueo = 0;
						bloquear(aEjecutar, cont, key);
						enviarBloqueoESI(g_socketEnEjecucion);
						aEjecutar = NULL;
					}
					if (g_termino || g_huboError) {
						if (!g_huboError)
							enviarRespuesta(g_socketEnEjecucion, CONTINUA_ESI);
						log_trace(g_logger, "%s ha terminado su ejecucion",
								aEjecutar->nombreESI);
						sem_wait(&continua);
						free(aEjecutar->nombreESI);
						free(aEjecutar);
						g_termino = 0;
						aEjecutar = NULL;
					}
					if (aEjecutar != NULL) {
						aEjecutar->estAnterior = calcularProximaRafaga(
								aEjecutar->estAnterior, aEjecutar->realAnterior,
								0);
						aEjecutar->realAnterior = cont;
						pthread_mutex_lock(&mutexListo);
						dictionary_put(g_listos, key, aEjecutar);
						pthread_mutex_unlock(&mutexListo);
						sem_post(&ESIentrada);
						log_trace(g_logger, "Se desaloja %s para replanificar",
								aEjecutar->nombreESI);
						pthread_mutex_lock(&modificacion);
						g_huboModificacion = 0;
						pthread_mutex_unlock(&modificacion);
					}
					free(key);
					g_idESIactual = NULL;
					sem_wait(&ESIentrada);
				}
				pthread_cleanup_pop(1);
}
