#include "globales.h"

static void vaciarListaBloqueados(t_infoBloqueo* nodo) {
	free(nodo->idESI);
	free(nodo->data->nombreESI);
	free(nodo->data);
	free(nodo);
}

static void vaciarBloqueados(t_list* nodo) {
	list_destroy_and_destroy_elements(nodo, (void*) vaciarListaBloqueados);
}

static void vaciarClaves(t_list* nodo) {
	list_destroy_and_destroy_elements(nodo, (void*) free);
}

static void vaciarListos(t_infoListos* nodo) {
	free(nodo->nombreESI);
	free(nodo);
}

extern void liberarTodo(void) {
	pthread_cancel(hiloAlgoritmos);
	pthread_cancel(hiloServidor);
	pthread_join(hiloServidor, NULL);
	pthread_join(hiloAlgoritmos, NULL);

	pthread_mutex_destroy(&mutexBloqueo);
	pthread_mutex_destroy(&mutexConsola);
	pthread_mutex_destroy(&mutexListo);
	pthread_mutex_destroy(&mutexLog);
	pthread_mutex_destroy(&modificacion);
	pthread_mutex_destroy(&mutexClavesTomadas);
	pthread_mutex_destroy(&mutexInstruccionConsola);

	sem_destroy(&ESIentrada);
	sem_destroy(&continua);

	log_destroy(g_logger);
	config_destroy(g_con);

	dictionary_destroy_and_destroy_elements(g_listos, (void*) vaciarListos);
	dictionary_destroy_and_destroy_elements(g_bloq, (void*) vaciarBloqueados);
	dictionary_destroy_and_destroy_elements(g_clavesTomadas,
			(void*) vaciarClaves);

	exit(0);
}

char* liberarESI(char* key) {
	char* nombre;

	void siEstaBloqueadaPorClaveEliminar(char* clave, t_list* listaBloqueados) {
		bool sonIguales(t_infoBloqueo* nodo) {
			return string_equals_ignore_case(nodo->idESI, key);
		}

		void liberarT_infoBloqueo(t_infoBloqueo* infoBloqueo) {
			nombre = strdup(infoBloqueo->data->nombreESI);
			free(infoBloqueo->data->nombreESI);
			free(infoBloqueo->data);
			free(infoBloqueo->idESI);
			free(infoBloqueo);
		}

		list_remove_and_destroy_by_condition(listaBloqueados,
				(void*) sonIguales, (void*) liberarT_infoBloqueo);

		if (list_is_empty(listaBloqueados)) {
			dictionary_remove_and_destroy(g_bloq, clave, (void*) list_destroy);
		}
	}

	if (dictionary_has_key(g_listos, key)) {
		pthread_mutex_lock(&mutexListo);
		nombre = strdup(((t_infoListos*)dictionary_get(g_listos, key))->nombreESI);
		pthread_mutex_unlock(&mutexListo);
		free(((t_infoListos*) dictionary_get(g_listos, key))->nombreESI);
		free(dictionary_remove(g_listos, key));
	} else {
		dictionary_iterator(g_bloq, (void*) siEstaBloqueadaPorClaveEliminar);
	}

	liberarClaves(nombre);
	return nombre;
}

void desbloquearESI(char* clave) {
	pthread_mutex_lock(&mutexBloqueo);
	t_list* lista = dictionary_remove(g_bloq, clave);
	t_infoBloqueo* nodo = list_remove(lista, 0);
	pthread_mutex_lock(&mutexListo);
	dictionary_put(g_listos, nodo->idESI, nodo->data);
	pthread_mutex_unlock(&mutexListo);
	if(list_is_empty(lista))
		list_destroy(lista);
	else
		dictionary_put(g_bloq, clave, lista);
	pthread_mutex_unlock(&mutexBloqueo);
	sem_post(&ESIentrada);
	pthread_mutex_lock(&modificacion);
	g_huboModificacion = 1;
	pthread_mutex_unlock(&modificacion);
}

void liberarClaves(char* clave) {
	if (dictionary_has_key(g_clavesTomadas, clave))
		list_destroy_and_destroy_elements(
				dictionary_remove(g_clavesTomadas, clave), (void*) free);
}
