

all: so-commons-library parsi readline biblioteca pruebas-ESI
	@tput setaf 2
	@echo "Terminado"
	@tput sgr0


so-commons-library:
	$(call mostrarTitulo,$@)
	git clone https://github.com/sisoputnfrba/so-commons-library ../so-commons-library
	cd ../so-commons-library; sudo make install
	@tput setaf 2
	@echo "Commons instaladas"
	@tput sgr0


parsi:
	$(call mostrarTitulo,$@)
	cd .. ; git clone https://github.com/sisoputnfrba/parsi
	cd ../parsi; sudo make install
	@tput setaf 2
	@echo "parsi  instalada"
	@tput sgr0


readline:
	$(call mostrarTitulo,$@)
	sudo apt-get install libreadline6 libreadline6-dev
	@tput setaf 2
	@echo "readline  instalada"
	@tput sgr0


biblioteca:
	$(call mostrarTitulo,$@)
	cd biblioteca-propia/Debug; make all
	mkdir /usr/include/biblioteca
	cp -u ./biblioteca-propia/Debug/libbiblioteca-propia.so /usr/lib/libbiblioteca-propia.so
	cp -u ./biblioteca-propia/biblioteca/*.h /usr/include/biblioteca
	@tput setaf 2
	@echo "Biblioteca Instalada"
	@tput sgr0


pruebas-ESI:
	$(call mostrarTitulo,$@)
	git clone https://github.com/sisoputnfrba/Pruebas-ESI.git
	@tput setaf 2
	@echo "Pruebas descargadas"
	@tput sgr0

	
clean:
	$(call mostrarTitulo,$@)
	rm -rf ../so-commons-library
	rm -rf /usr/include/biblioteca
	rm -rf /usr/lib/libbiblioteca-propia.so
	rm -rf ../parsi
	rm -rf ../Pruebas-ESI
	@tput setaf 2
	@echo "Desinstalado"
	@tput sgr0

