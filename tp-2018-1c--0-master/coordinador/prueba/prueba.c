#include "prueba.h"

int main(void) {
/*
	sem_init(&g_mutex_tablas,0,1);
	t_list* tablaDeInstancia = crearListaInstancias();
	pruebaInstancias(tablaDeInstancia);
	agregarClaveDeInstancia(list_get(tablaDeInstancia,0),"lucas");
	mostrarInstancia(list_get(tablaDeInstancia,0));
*/
	int i = 10;
	i = i/5 + (i%5 != 0);
	printf("%i\n",i);
	return 0;
}



void pruebaInstancias(t_list* tablaDeInstancias) {

	agregarInstancia(tablaDeInstancias, crearInstancia("instancia1", 12, 20));
	agregarInstancia(tablaDeInstancias, crearInstancia("instancia2", 13, 20));
	agregarInstancia(tablaDeInstancias, crearInstancia("instancia3", 14, 20));
	mostrarTablaInstancia(tablaDeInstancias);

}
