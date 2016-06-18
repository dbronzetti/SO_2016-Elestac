/*
 * consola.c
 *
 */

#include "consola.h"

int main() {
	char inputTeclado[250];
	char archivoAEjecutar[300];
	while (1) {
		printf(PROMPT);
		fgets(inputTeclado, sizeof(inputTeclado), stdin);
		char ** comando = string_split(inputTeclado, " ");
		switch (ReconocerComando(comando[0])) {
		case 1:
			printf("Comando Reconocido.\n");
			leerArchivoYGuardarEnCadena();
			fgets(inputTeclado, sizeof(inputTeclado), stdin);
			char ** comando = string_split(inputTeclado, " ");
			break;

		case 2:
			exit(-1);
		default:
			printf("Comando invalido, inténtelo nuevamente.\n");
		}
	}
	return 0;
}

int ReconocerComando(char * comando) {
	if (!strcmp("ejecutar\n", comando)) {
		return 1;
	}
	if (!strcmp("salir\n", comando)) {
		return 2;
	}
	return -1;
}

void leerArchivoYGuardarEnCadena() {
	char textoDeArchivo[300];
	FILE* archivo = NULL;
	printf("Ingrese archivo a ejecutar.\n");
	char nombreDelArchivo[300];
	scanf("%s", nombreDelArchivo);
	archivo = fopen(nombreDelArchivo, "r");
	if (archivo == NULL) {
		printf("Error al abrir el archivo.\n");
	} else {
		while (!feof(archivo)) {
			fgets(textoDeArchivo, 300, archivo);
			printf("%s\n", textoDeArchivo);
		}
	}
	fclose(archivo);
}
