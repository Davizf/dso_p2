/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	01/03/2017
 */


int checkFile(char* path);  // Comprobar las condiciones para crear el fichero

int checkDir(char* path); // Comprobar las condiciones para crear el directorio

int checkPath(char* path);  // Comprobar si la ruta es válida
