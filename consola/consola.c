#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#define PROMPT "anSISOP> "

char* leerArchivoYGuardarEnCadena(char*);
int compararCadenas(char*, char*);
int ReconocerComando(char ** );


int main() {
	char inputTeclado[250];
	char archivoAEjecutar[250];
	char* hash = "#!/usr/bin/ansisop";
	char* script;
	while (1) {
		printf(PROMPT);
		fgets(inputTeclado, sizeof(inputTeclado), stdin);
		char ** comando = string_split(inputTeclado, " ");
		switch (ReconocerComando(comando[0])) {
		case 1:
			printf("Comando Reconocido\n");
			printf("Ingrese archivo a ejecutar.\n");
			fgets(archivoAEjecutar, sizeof(archivoAEjecutar), stdin);
			script = leerArchivoYGuardarEnCadena(archivoAEjecutar);
			if (string_starts_with(script, hash))
				//if(compararCadenas(script, hash))
				printf("Script válido. \n");
			else
				printf("Script inválido. \n");
			break;
		default:
			printf("Comando invalido\n");
			exit(-1);
		}

	}
	return 0;
}

int ReconocerComando(char ** comando) {
	if (!strcmp("ejecutar", comando)) {
		return 1;
	}
	return -1;
}

char* leerArchivoYGuardarEnCadena(char* path) {
	//char* nombreArchivo = "prueba.txt";
	char* vacio = "";
	char textoExtraido[250];
	printf("%s", path);
	FILE *archivo = fopen("prueba.txt", "r");
	if (archivo == NULL) {
		printf("Error al abrir el archivo\n");
		return vacio;
	} else {
		while (!feof(archivo)) {
			fgets(textoExtraido, 250, archivo);
			//printf("%s\n", textoExtraido);
		}
		return textoExtraido;
	}
	return textoExtraido;
}

int compararCadenas(char* a, char* b) {
	char * c;
	char* hash = "#!/usr/bin/ansisop";
	c = strstr(a, b);
	if (!strcmp(c, hash))
		return 1;
	return -1;
}
