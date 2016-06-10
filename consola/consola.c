/*
 * consola.c
 *
 */

#include "consola.h"

int main() {
	char inputTeclado[250];
	char archivoAEjecutar[250];
//	char* hash = "#!/usr/bin/ansisop";
	char* codeScript;
	while (1) {
		printf(PROMPT);
		fgets(inputTeclado, sizeof(inputTeclado), stdin);
		char ** comando = string_split(inputTeclado, " ");
		switch (ReconocerComando(comando[0])) {
		case 1:
			printf("Comando Reconocido\n");

//			fgets(archivoAEjecutar, sizeof(archivoAEjecutar), stdin);
			codeScript = leerArchivoYGuardarEnCadena(archivoAEjecutar);
//			if (string_starts_with(script, hash))
//			if(compararCadenas(script, hash))
//				printf("Script válido. \n");
//			else
//				printf("Script inválido. \n");
			break;
		default:
			printf("Comando invalido\n");
			exit(-1);
		}

	}

	return 0;
}

int ReconocerComando(char ** comando) {
	if (strcmp("ejecutar", comando)) {
		return 1;
	}
	return -1;
}

char* leerArchivoYGuardarEnCadena(char* path) {
	//char* nombreArchivo = "prueba.txt";
	char* hash = "#!/usr/bin/ansisop";
	char* vacio = "";
	char* textoExtraido;
	printf("%s", path);
	FILE *archivo = NULL;
	archivo = fopen(path, "r+");

	printf("Ingrese archivo a ejecutar.\n");
	char* nombrearchivo;
	scanf("%s", nombrearchivo);
	char lectura[300];

	archivo = fopen(nombrearchivo, "r");
	if (archivo == NULL) {
		printf("Error al abrir el archivo\n");
		return vacio;
	} else {

		fscanf(archivo, "%[^\n]", &lectura);
		printf("Primera linea: %s\n", lectura);

		if (strcmp(lectura, hash)) {
			printf("Script inválido. \n");
		} else {
			printf("Script válido. \n");
			while (!feof(archivo)) {

				fgets(textoExtraido, 250, archivo);
				printf("%s\n", textoExtraido);
			}
		}
		fclose(archivo);
		return textoExtraido;
	}
}

/*

int compararCadenas(char* a, char* b) {
	char * c;
	char* hash = "#!/usr/bin/ansisop";
	c = strstr(a, b);
	if (!strcmp(c, hash)){
		return 1;
	}
	return 0;
}
*/

