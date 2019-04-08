

run: tablasAdministrativas prueba 
	./prueba

prueba: tablasAdministrativas
	gcc prueba.c -o prueba -lcommons -lbiblioteca-propia -lpthread tablasAdministrativas

tablasAdministrativas: 
	gcc -c ../src/tablasAdministrativas.c -o tablasAdministrativas


clean:
	-rm prueba
	-rm tablasAdministrativas
