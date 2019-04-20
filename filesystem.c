/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	01/03/2017
 */

#include "include/filesystem.h" // Headers for the core functionality
#include "include/auxiliary.h"  // Headers for auxiliary functions
#include "include/metadata.h"   // Type and structure declaration of the file system


/*************************************** Begin Student Code ***************************************/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//Se inicializan las estructuras de memoria que se van a utilizar en la gestión del sistema de ficheros
struct Superblock sb;
char iNodoNames[MAX_NUMBER_FILES][MAX_NAME_LENGHT];

#define CLOSED 0
#define OPENED 1
#define DATA_BLOCKS_START 2
#define FSCLEAN 0
#define FSACTIVE 1

/*************************************** End Student Code ***************************************/


/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	/*************************************** Begin Student Code ***************************************/
	// Se comprueba que el tamaño del dispositivo está entre 50KiB y 100KiB, tal y como se especifa en los requisitos de arquitectura
	//En caso de que el tamaño del dispositivo no esté en el rango aceptable, devuelve error
	if (deviceSize < 51200 || deviceSize > 102400) return -1;

	// Se restaura el mapa de i-nodos y el número de bloques ocupados. Se guarda en los metadatos el tamaño del dispostivio.
	sb.diskSize = deviceSize;
	sb.INodoMap = 0;
	sb.magicNum = 0;

	//Se recorre el array de inodos, restableciendo los nombres y el magicNum de cada fichero y cerrando todos los archivos.
	// Se borran todos los nombres de la estructura que almacena los nombres de los ficheros.
	int i;
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		sb.iNodos[i].name = iNodoNames[i];
		strcpy(iNodoNames[i], "");
		sb.iNodos[i].opened = CLOSED;
		sb.iNodos[i].magicNum = 0;
	}

	//Por último, se desmonta el dispositivo. Si ocurre un problema desmontandolo, devuelve error.
	if (unmountFS() == -1) return -1;

	//Si la ejecución es correcta, devuelve un 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	/*************************************** Begin Student Code ***************************************/
	//Mediante la función bread, se traen al sistema de ficheros los dos bloques que contienen los metadatos.
	//En caso de error durante la ejecución de esta función, se devuelve error.
	if (bread(DEVICE_IMAGE, 0, ((char *)&(sb))) == -1) return -1;
	if (bread(DEVICE_IMAGE, 1, ((char *)&(iNodoNames))) == -1) return -1;
	//TODO: Comenta los nuevos cambios

	//Si la ejecución es correcta, devuelve un 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	/*************************************** Begin Student Code ***************************************/
	//Mediante la función bwrite, se guardan en el dispositivo los dos bloques que contienen los metadatos.
	//En caso de error durante la ejecución de esta función, se devuelve error.
	if (bwrite(DEVICE_IMAGE, 0, ((char *)&(sb))) == -1) return -1;
	if (bwrite(DEVICE_IMAGE, 1, ((char *)&(iNodoNames))) == -1) return -1;

	//Si la ejecución es correcta, devuelve un 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *path)
{
	/*************************************** Begin Student Code ***************************************/
	//Se recorre el array que almacena los nombres para comprobar si el nombre del fichero que se desea crear
	//Si ese nombre ya existe, se devuelve fallo.
	int i;
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		if(strcmp(sb.iNodos[i].name, path) == 0) return -1;
	}

	// Máscara con la ultima posición a 1.
	uint64_t mask = 0x0000000000000001;
	//Se crea una copia del mapa de inodos, ya que va a ser modificado
	uint64_t iNodoMapCpy = sb.INodoMap;
	//La primera posicion que tenemos es 2^0, es decir, 1
	uint64_t numToAdd = 1;


	// Recorre el mapa de inodos
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		//Cuando la funcion AND entre la mascara y el mapara devuelva un 0, significa que el mapa en esa posicion es 0,
		//ya que la máscara sabemos que tiene el valor 1. En ese casa, se para el bucle, ya que se tiene el valor deseado.
		if ((mask & iNodoMapCpy) == 0) {
			break;
		}

		printf("\n\nmask = %ju (0x%jx)\n",mask, mask);
		printf("iNodoMapCpy = %ju (0x%jx)\n",iNodoMapCpy,iNodoMapCpy);
		printf("numToAdd = %ju (0x%jx)\n\n",numToAdd,numToAdd);
		//Si el AND devuelve 1, esa posición está ocupada, por lo que pasamos a buscar en la siguiente posición. Para ello
		// multiplicamos por 2 la posición donde vamos a añadirlo y desplazamos una posición el mapa.
		numToAdd *= 2;
		iNodoMapCpy = iNodoMapCpy >> 1;
	}

	//Se añade en la posición libre el nombre del archivo que se va a añadir. Se añade al mapa la posicion que se ocupa.
	//En el array de inodos se guarda la posición del fichero.
	strcpy(iNodoNames[i], path);
	sb.INodoMap += numToAdd;
	sb.iNodos[i].directBlock = DATA_BLOCKS_START+i+1;

	//Si la ejecución es correcta, se devuelve un 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *path)
{
	/*************************************** Begin Student Code ***************************************/
	uint64_t numToAdd = 1;
	int i;
	//Se recorre el array que guarda los nombres para encontrar el fichero.
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		if(strcmp(sb.iNodos[i].name, path) == 0) {
			//Cuando encuentra el fichero, le resta la posicion en la que esta para dejarlo libre y borra el nombre
			sb.INodoMap -= numToAdd;
			strcpy(iNodoNames[i], "");
			return 0;
		}
		numToAdd *= 2;
	}
	//Si no encuentra el fichero, devuelve error.
	return -1;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *path)
{
	/*************************************** Begin Student Code ***************************************/
	// Se invoca a la funcion findByName, que devuelve el descriptor a partir del nombre del fichero.
	int fd = findByName(path);

	//Si el descriptor es erróneo, devuelve error
	if (fd == -1) return -1;

	// Si el fichero ya está abierto se genera un error
	if(sb.iNodos[fd].opened == OPENED) return -2;

	//Cambia el atributo a OPENED
	sb.iNodos[fd].opened = OPENED;

	//Pone el puntero del fichero al principio. Si esta ejecución es errónea, se devuelve error
	if (lseekFile(fd, FS_SEEK_BEGIN, 0) != 0) return -1;

	// Si todo es correcto, se devuelve el descriptor del fichero abierto
	return fd;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fileDescriptor < 0 || fileDescriptor > MAX_NUMBER_FILES-1) return -1;

	// Si el superbloque que se quiere cerrar ya está cerrado, se genera un error.
	if(sb.iNodos[fileDescriptor].opened == CLOSED) return -1;

	// Si el superbloque que se quiere cerrar está abierto se cierra.
	sb.iNodos[fileDescriptor].opened = CLOSED;

	//Si la ejecución es correcta, se devuelve 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fileDescriptor < 0 || fileDescriptor > MAX_NUMBER_FILES-1) return -1;

	// El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero
	if (numBytes < 0 || numBytes > MAX_FILE_SIZE) return -1;

	// El fichero tiene que estar abierto para poder leer
	if (sb.iNodos[fileDescriptor].opened != OPENED) return -1;

	// Si en el mapa de iNodos el bit correspondiente al descriptor de fichero es 0, se devuelve error
	if(iNodoUsed(sb.INodoMap, fileDescriptor) == 0) return -1;

	char block[BLOCK_SIZE];
	bread(DEVICE_IMAGE, fileDescriptor+DATA_BLOCKS_START, block);

	// Se almacenan los bytes solicitados del fichero en el buffer
	memmove(buffer, block, numBytes);

	//Devuelve el número de Bytes leídos.
	return numBytes;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fileDescriptor < 0 || fileDescriptor > MAX_NUMBER_FILES-1){
		printf("el superbloque está fuera del rango\n");
		return -1;
	}

	// El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero
	if (numBytes < 0 || numBytes > MAX_FILE_SIZE){
		printf("El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero\n");
		return -1;
	}
	// El fichero tiene que estar abierto para poder leer
	if (sb.iNodos[fileDescriptor].opened != OPENED){
		printf("El fichero tiene que estar abierto para poder leer\n");
		return -1;
	}
	// Si en el mapa de iNodos el bit correspondiente al descriptor de fichero es 0, se devuelve error
	if(iNodoUsed(sb.INodoMap, fileDescriptor) == 0){
		printf("en el mapa de iNodos el bit correspondiente al descriptor de fichero es 0\n");
		return -1;
	}

	//Se escribe en un bloque el buffer que almacena lo que se desa escribir
	char block[BLOCK_SIZE];
	memmove(block, buffer, numBytes);

	//Se invoca a la función bwrite para guardar en memoria el bloque a escribir. Si la ejecución es errónea devuelve error.
	if (bwrite(DEVICE_IMAGE, fileDescriptor, block) == -1){
		printf("error bwrite\n");
		return -1;
	}

///////
	char aux[BLOCK_SIZE];
	if (bread(DEVICE_IMAGE, fileDescriptor, aux) ==-1){
		printf("error bread\n");
		return -1;
	}

	//Se realiza el magicNum del nuevo bloque y se almacena.
	//sb.iNodos[fileDescriptor].magicNum = CRC16((unsigned char*)block, BLOCK_SIZE);

	//Se desmonta para sincronizar el dispositivo y el sistema de ficheros.
	if (unmountFS() == -1){
		printf("error unmountFS\n");
		return -1;
	}
	//Devuelve el número de Bytes escritos.
	return numBytes;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fileDescriptor < 0 || fileDescriptor > MAX_NUMBER_FILES-1) return -1;

	//Si se desea cambiar el puntero fuera del archivo, devuelve error.
	if (whence == FS_SEEK_CUR &&
		(sb.iNodos[fileDescriptor].pointer + offset > MAX_NUMBER_FILES-1 || sb.iNodos[fileDescriptor].pointer + offset < 0)) return -1;

	//Se comprueba el flag de whence
	switch(whence){
		//Si se desea cambiar el puntero desde la posición anterior, se añade el número de bytes que se desee moveer
		case FS_SEEK_CUR:
			sb.iNodos[fileDescriptor].pointer = offset;
			break;
		//Si se desea cambiar a la posición inicial, se pone a 0.
		case FS_SEEK_BEGIN:
			sb.iNodos[fileDescriptor].pointer = 0;
			break;
		//Si se desea cambiar a la posición inicial, se pone al tamaño del bloque-1.
		case FS_SEEK_END:
			sb.iNodos[fileDescriptor].pointer = MAX_FILE_SIZE-1;
			break;
	}
	//Si la ejecucíón es correcta, se devuelve 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Creates a new directory provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the directory already exists, -2 in case of error.
 */
int mkDir(char *path)
{
	/*************************************** Begin Student Code ***************************************/
	return -1;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Deletes a directory, provided it exists in the file system.
 * @return	0 if success, -1 if the directory does not exist, -2 in case of error..
 */
int rmDir(char *path)
{
	/*************************************** Begin Student Code ***************************************/
	return -1;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Lists the content of a directory and stores the inodes and names in arrays.
 * @return	The number of items in the directory, -1 if the directory does not exist, -2 in case of error..
 */
int lsDir(char *path, int inodesDir[10], char namesDir[10][33])
{
	/*************************************** Begin Student Code ***************************************/
	return -1;
	/*************************************** End Student Code ***************************************/
}









	/*************************************** Begin Student Code ***************************************/




/******************Función: checkFile*****************************
Se encarga revisar la integridad de un fichero*/
int checkFile(char *path)
{
	//Se obtiene el descriptor del fichero mediante la funcion findByName.
	int fd = findByName(path);
	char block[BLOCK_SIZE];
	//Se obtiene el fichero que se desea comprobar
	if(bread(DEVICE_IMAGE, fd+DATA_BLOCKS_START, block) == -1) return -2;

	// Se genera el magicNum del fichero que se ha obtenido. Se compara con el magicNum de ese fichero, si no coinciden está corrupto.
	//if (sb.iNodos[fd].magicNum != CRC16((unsigned char*)block, BLOCK_SIZE)) return -1;

	//Si la ejecucíón es correcta, se devuelve 0.
	return 0;
}

/********************FUNCIONES PROPIAS**************************/




/******************Función: iNodeUsed*****************************
Se encarga de comprobar si un inodo esta siendo usado.*/
int iNodoUsed(uint64_t iNodoMap, int fileDescriptor) {
	uint64_t mask = 1;
	int i;
	//Se encuentra la posición del fichero
	for(i = 0; i<fileDescriptor; i++) {
		mask *= 2;
	}

	//Se hace un AND con la máscara que vale 1. Si devuelve 0, esta libre y devuelve 0.
	if ((mask & iNodoMap) == 0) {
		return 0;
	}
//Si devuelve 1, esta ocupado y se devuelve -1.
	return -1;
}


/******************Función: findByName*****************************
Se encarga de devolver el descriptor de un fichero a partir del nombre.*/
int findByName(char *path) {
	int fd;
	for(fd = 0; fd < MAX_NUMBER_FILES; fd++) {
		//Se busca el fichero y se devuelve su posición
		if(strcmp(sb.iNodos[fd].name, path) == 0) {
			return fd;
		}
	}
	//Si no se encuentra el fichero, se devuelve -1.
	return -1;
}

/*************************************** End Student Code ***************************************/
