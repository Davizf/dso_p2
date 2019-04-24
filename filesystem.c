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
#include <string.h>


// las estructuras iniciales
struct Superblock sb;
char iNodeNames[MAX_NUMBER_FILES][MAX_NAME_LENGHT];	// array para guardar los nombres de los ficheros/directorios
int fileState[MAX_NUMBER_FILES];	// array para guardar el estado de los ficheros: abierto o cerrado
int levels[MAX_FILE_SIZE]; // el nivel que se encuentra cada fichero: 4 niveles en total
char preDir[MAX_NUMBER_FILES][MAX_FILE_SIZE]; // array para guardar los nombres de los directorios predecesor del fichero/directorio

int aux_level;
char* aux_preDir;
char* aux_fileName;
// estados para los ficheros
#define CLOSED 0
#define OPENED 1

// tipo de inodo para INode.type
#define FILE 0
#define DIR 1
// el número del bloque inicial
#define INIT_BLOCK 1


/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{

	// comprobar la condición NF9
	if (deviceSize < 50*1024 || deviceSize > 10*1024*1024){
		printf("Error NF9\n");
		return -1;
	}

	// inicializar los valores iniciales
	sb.disk_size = deviceSize;
	sb.inodeMap = 0;
	sb.magicNum = 2019; // un número que inventamos para identificar el sistema de ficheros

	// recorrer todo el array de inodes para restablecer el nombre y el estado a CLOSED
	int i;
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		strcpy(iNodeNames[i], "");
		sb.inodes[i].name = iNodeNames[i];
		fileState[i] = CLOSED;
		levels[i] = 0;
		strcpy(preDir[i], "");
	}

	// Para terminar hacemos un unmount para guardar los metadatos en el disco
	if (unmountFS() == -1){
		printf("Error unmountFS\n");
		return -1;
	}

	// si va todo bien devuelve 0;
	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	// leer el bloque que contienen los metadatos en el disco
	if (bread(DEVICE_IMAGE, 0, ((char *)&(sb))) == -1){
		printf("error bread\n");
		return -1;
	}

	// si va todo bien devuelve 0;
	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	// escribir el bloque que contienen los metadatos al disco
	if (bwrite(DEVICE_IMAGE, 0, ((char *)&(sb))) == -1) return -1;

	// si va todo bien devuelve 0;
	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *path)
{
	// comprueba las condicionanes a la hora de crear el fichero
	if(checkFile(path) == -1){
		printf("error checkFile\n");
		return -1;
	}

	// ahora procedemos a encontrar el id para el fichero a crear
	uint64_t mask = 1;	// mascara a 1
	uint64_t inodeMapCpy = sb.inodeMap;	// copia de inodeMap
	uint64_t position = 1;	// La primera posición de el mapa de inodes es 2^0, o sea, 1


	// recorre el mapa de inodes
	int i;
	for(i = 0; i < MAX_NUMBER_FILES && ((mask & inodeMapCpy) != 0); i++) {

		// el objetivo es encontrar 'i' que tenga valor 0 en el mapa de inodes
		// tabla de AND para iNodeMapCpy y mas
		// si el AND devuelve 1, esa posición está ocupada, entonces pasamos a la siguiente posición.
		// y para ello multiplicamos por 2 la posición y desplazamos una posición el mapa.
		position *= 2;
		inodeMapCpy = inodeMapCpy >> 1;
	}

	// cuando encontramos la posición la almacenamos en el array de nombres de ficheros y en superbloque
	strcpy(iNodeNames[i], aux_fileName);
	sb.inodes[i].name = iNodeNames[i];
	sb.inodeMap += position;
	sb.inodes[i].directBlock = INIT_BLOCK+i+1;
	sb.inodes[i].type = FILE;
	levels[i] = aux_level;
	printf("predir is %s\n",aux_preDir);
	strcpy(preDir[i],aux_preDir);

	// si va todo bien devuelve 0
	return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *path)
{
	char *pch;
	char line[256];  // where we will put a copy of the input
	char *fileName;
	strcpy(line, path);
	pch = strtok(line,"/"); // find the first double quote
	while(pch != NULL){
		fileName = pch;
		pch = strtok (NULL, "/");
	}

	int i;

	uint64_t position = 1;
	//printf("fileName is %s\n",fileName);
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		//Se busca el fichero y se devuelve su posición
		//printf("sb name is %s\n",sb.inodes[i].name);
		if(strcmp(sb.inodes[i].name, fileName) == 0) {
			//Cuando encuentra el fichero, le resta la posicion en la que esta para dejarlo libre y borra el nombre
			sb.inodeMap -= position;
			strcpy(iNodeNames[i], "");
			sb.inodes[i].name = iNodeNames[i];
			fileState[i] = CLOSED;
			levels[i] = 0;
			strcpy(preDir[i],"");
			return 0;
		}
		position *= 2;
	}

	//Si no encuentra el fichero, devuelve error.
	return -1;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *path)
{
	/*************************************** Begin Student Code ***************************************/
	// Se invoca a la funcion findByName, que devuelve el descriptor a partir del nombre del fichero.

	char *pch;
	char line[256];  // where we will put a copy of the input
	char *fileName;
	strcpy(line, path);
	pch = strtok(line,"/"); // find the first double quote
	while(pch != NULL){
		fileName = pch;
		pch = strtok (NULL, "/");
	}

	int fd = -1;
	int i;

	//printf("fileName is %s\n",fileName);
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		//Se busca el fichero y se devuelve su posición
		//printf("sb name is %s\n",sb.inodes[i].name);
		if(strcmp(sb.inodes[i].name, fileName) == 0) {
			fd = i;
			break;
		}
	}

	//Si el descriptor es erróneo, devuelve error
	if (fd == -1) return -1;


	// Si el fichero ya está abierto se genera un error
	if(fileState[fd] == OPENED) return -2;

	//Cambia el atributo a OPENED
	fileState[fd] = OPENED;

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
int closeFile(int fd)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fd < 0 || fd > MAX_NUMBER_FILES-1) return -1;

	// Si el superbloque que se quiere cerrar ya está cerrado, se genera un error.
	if(fileState[fd] == CLOSED) return -1;

	// Si el superbloque que se quiere cerrar está abierto se cierra.
	fileState[fd] = CLOSED;

	//Si la ejecución es correcta, se devuelve 0.
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fd, void *buffer, int numBytes)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fd < 0 || fd > MAX_NUMBER_FILES-1) return -1;

	// El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero
	if (numBytes < 0 || numBytes > MAX_FILE_SIZE) return -1;

	// El fichero tiene que estar abierto para poder leer
	if (fileState[fd] != OPENED) return -1;

	// Si en el mapa de inodes el bit correspondiente al descriptor de fichero es 0, se devuelve error
	uint64_t mask = 1;
	int i;
	//Se encuentra la posición del fichero
	for(i = 0; i<fd; i++) {
		mask *= 2;
	}

	//Se hace un AND con la máscara que vale 1. Si devuelve 0, esta libre y devuelve 0.
	if ((mask & sb.inodeMap) == 0) {
		return -1;
	}



	char block[BLOCK_SIZE];
	bread(DEVICE_IMAGE, fd+INIT_BLOCK, block);

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
int writeFile(int fd, void *buffer, int numBytes)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fd < 0 || fd > MAX_NUMBER_FILES-1){
		printf("el superbloque está fuera del rango\n");
		return -1;
	}

	// El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero
	if (numBytes < 0 || numBytes > MAX_FILE_SIZE){
		printf("El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero\n");
		return -1;
	}
	// El fichero tiene que estar abierto para poder leer
	if (fileState[fd] != OPENED){
		printf("El fichero tiene que estar abierto para poder leer\n");
		return -1;
	}
	// Si en el mapa de inodes el bit correspondiente al descriptor de fichero es 0, se devuelve error
	uint64_t mask = 1;
	int i;
	//Se encuentra la posición del fichero
	for(i = 0; i<fd; i++) {
		mask *= 2;
	}

	//Se hace un AND con la máscara que vale 1. Si devuelve 0, esta libre y devuelve 0.
	if ((mask & sb.inodeMap) == 0) {
		return -1;
	}


	//Se escribe en un bloque el buffer que almacena lo que se desa escribir
	char block[BLOCK_SIZE];
	memmove(block, buffer, numBytes);

	//Se invoca a la función bwrite para guardar en memoria el bloque a escribir. Si la ejecución es errónea devuelve error.
	if (bwrite(DEVICE_IMAGE, fd+INIT_BLOCK, block) == -1){
		printf("error bwrite\n");
		return -1;
	}

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
int lseekFile(int fd, long offset, int whence)
{
	/*************************************** Begin Student Code ***************************************/
	// Si el superbloque está fuera del rango se devuelve un error
	if (fd < 0 || fd > MAX_NUMBER_FILES-1) return -1;

	//Si se desea cambiar el puntero fuera del archivo, devuelve error.
	if (whence == FS_SEEK_CUR &&
		(sb.inodes[fd].pointer + offset > MAX_NUMBER_FILES-1 || sb.inodes[fd].pointer + offset < 0)) return -1;

	//Se comprueba el flag de whence
	switch(whence){
		//Si se desea cambiar el puntero desde la posición anterior, se añade el número de bytes que se desee moveer
		case FS_SEEK_CUR:
			sb.inodes[fd].pointer = offset;
			break;
		//Si se desea cambiar a la posición inicial, se pone a 0.
		case FS_SEEK_BEGIN:
			sb.inodes[fd].pointer = 0;
			break;
		//Si se desea cambiar a la posición inicial, se pone al tamaño del bloque-1.
		case FS_SEEK_END:
			sb.inodes[fd].pointer = MAX_FILE_SIZE-1;
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

	// comprueba las condicionanes a la hora de crear el fichero
	if(checkDir(path) == -1){
		printf("error mkDir\n");
		return -1;
	}

	// ahora procedemos a encontrar el id para el fichero a crear
	uint64_t mask = 1;	// mascara a 1
	uint64_t inodeMapCpy = sb.inodeMap;	// copia de inodeMap
	uint64_t position = 1;	// La primera posición de el mapa de inodes es 2^0, o sea, 1


	// recorre el mapa de inodes
	int i;
	for(i = 0; i < MAX_NUMBER_FILES && ((mask & inodeMapCpy) != 0); i++) {

		// el objetivo es encontrar 'i' que tenga valor 0 en el mapa de inodes
		// tabla de AND para iNodeMapCpy y mas
		// si el AND devuelve 1, esa posición está ocupada, entonces pasamos a la siguiente posición.
		// y para ello multiplicamos por 2 la posición y desplazamos una posición el mapa.
		position *= 2;
		inodeMapCpy = inodeMapCpy >> 1;
	}

	// cuando encontramos la posición la almacenamos en el array de nombres de ficheros y en superbloque
	strcpy(iNodeNames[i], aux_fileName);
	sb.inodes[i].name = iNodeNames[i];
	sb.inodeMap += position;
	sb.inodes[i].directBlock = INIT_BLOCK+i+1;
	sb.inodes[i].type = DIR;
	levels[i] = aux_level;
	strcpy(preDir[i],aux_preDir);

	// si va todo bien devuelve 0

	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Deletes a directory, provided it exists in the file system.
 * @return	0 if success, -1 if the directory does not exist, -2 in case of error..
 */
int rmDir(char *path)
{
	char *pch;
	char line[256];  // where we will put a copy of the input
	char *dirName;
	strcpy(line, path);
	pch = strtok(line,"/"); // find the first double quote
	while(pch != NULL){
		dirName = pch;
		pch = strtok (NULL, "/");
	}

	int i;

	uint64_t position = 1;
	//printf("fileName is %s\n",fileName);
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		//Se busca el fichero y se devuelve su posición
		//printf("sb name is %s\n",sb.inodes[i].name);
		if(strcmp(sb.inodes[i].name, dirName) == 0) {
			//Cuando encuentra el fichero, le resta la posicion en la que esta para dejarlo libre y borra el nombre
			sb.inodeMap -= position;
			strcpy(iNodeNames[i], "");
			sb.inodes[i].name = iNodeNames[i];
			levels[i] = 0;
			strcpy(preDir[i],"");
			return 0;
		}
		position *= 2;
	}

	//Si no encuentra el fichero, devuelve error.
	return -1;
}

/*
 * @brief	Lists the content of a directory and stores the inodes and names in arrays.
 * @return	The number of items in the directory, -1 if the directory does not exist, -2 in case of error..
 */
int lsDir(char *path, int inodesDir[10], char namesDir[10][33])
{
	char *dirName;
	if(strcmp(path,"/")==0){
		dirName ="/";
	}else{
		char *pch;
		char line[256];

		strcpy(line, path);
		pch = strtok(line,"/");
		while(pch != NULL){
			dirName = pch;
			pch = strtok (NULL, "/");
		}
	}


	printf("dirName is %s\n",dirName);
	int i;
	int index = 0;
	for(i=0; i<MAX_NUMBER_FILES; i++){
		if(strcmp(preDir[i],dirName) == 0){
			inodesDir[index] = i;
			strcpy(namesDir[index],iNodeNames[i]);
			index++;
		}
	}
	for(; index<10; index++){
		inodesDir[index] = -1;
	}

	return 0;
}



/*************************************** Auxiliary Functions ***************************************/
int checkFile(char* path){

	if(path[0] != '/'||path[strlen(path)-1]=='/') return -1;

  char line[256];  // where we will put a copy of the input

	strcpy(line, path);
	char *fileWay[4];
  char *pch;

  int maxLevel=0;



	pch = strtok(line,"/"); // find the first double quote
	do{
    fileWay[maxLevel] = pch;
		pch = strtok (NULL, "/");
    maxLevel++;
	}while(pch != NULL&&maxLevel<4);



	int i,j;
	int directoryExist = 0;
	if(maxLevel>=2){
		for(i = 0; i < MAX_NUMBER_FILES; i++) {
			for(j=0; j<=maxLevel-2; j++){
				if((strcmp(sb.inodes[i].name, fileWay[j]) == 0)&&(sb.inodes[i].type==DIR)&&(levels[i]==(j))){
					directoryExist = 1;
				}
			}
		}
	}

	if(directoryExist == 0&&maxLevel>=2){
		printf("Directory does not exist\n");
		return -1;
	}

	for(i = 0; i < MAX_NUMBER_FILES; i++) {

		//printf("inode[%i].name is %s and fileWay[%i] is %s\n",i,sb.inodes[i].name, maxLevel-1,fileWay[maxLevel-1]);
		if((strcmp(sb.inodes[i].name, fileWay[maxLevel-1]) == 0)&&(levels[i]==(maxLevel-1))){
			//printf("preDir is %s and fileWay is %s\n",preDir[i], fileWay[maxLevel-2]);
			if((maxLevel-1)>0){
				if(strcmp(preDir[i], fileWay[maxLevel-2]) == 0){
					printf("File already exists\n");
					return -1;
				}
			}else{
				printf("File already exists\n");
				return -1;
			}

		}
	}
	aux_level = maxLevel-1;
	if(maxLevel>=2){
		aux_preDir = fileWay[maxLevel-2];
	}else{
		aux_preDir = "/";
	}
	aux_fileName = fileWay[maxLevel-1];
	return 0;
}


int checkDir(char* path){
	if(path[0] != '/'||path[strlen(path)-1]=='/') return -1;

  char line[256];  // where we will put a copy of the input

	strcpy(line, path);
	char *fileWay[4];
  char *pch;

  int maxLevel=0;



	pch = strtok(line,"/"); // find the first double quote
	do{
    fileWay[maxLevel] = pch;
		pch = strtok (NULL, "/");
    maxLevel++;
	}while(pch != NULL&&maxLevel<4);



	int i,j;
	int directoryExist = 1;
	if(maxLevel>=2){
		directoryExist = 0;
		for(i = 0; i < MAX_NUMBER_FILES; i++) {
			for(j=0; j<=maxLevel-2; j++){
				if((strcmp(sb.inodes[i].name, fileWay[j]) == 0)&&(sb.inodes[i].type==DIR)&&(levels[i]==(j))){
					directoryExist = 1;
				}
			}
		}
	}

	if(directoryExist == 0){
		printf("Previous directory does not exist\n");
		return -1;
	}

	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		if((strcmp(sb.inodes[i].name, fileWay[maxLevel-1]) == 0)&&(levels[i]==(maxLevel-1))){
			printf("There already exist file or directory with the same name\n");
			return -1;
		}
	}




	aux_level = maxLevel-1;
	if(maxLevel>=2){
		aux_preDir = fileWay[maxLevel-2];
	}else{
		aux_preDir = "/";
	}
	aux_fileName = fileWay[maxLevel-1];
	return 0;
}
