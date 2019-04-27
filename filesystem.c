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
char preDir[MAX_NUMBER_FILES][MAX_NAME_LENGHT]; // array para guardar los nombres de los directorios predecesor del fichero/directorio
int suDir[MAX_NUMBER_FILES]; // array para guardar la cantidad de ficheros o directoios contenido en un directorio
int suDirMaster; // variable para guardar la cantidad de ficheros o directorio contenido en el directorio master

int aux_level;	// variable auxiliar para guardar temporalmente el nivel del fichero o directorio
char aux_preDir[32];	// variable auxiliar para guardar temporalmente el directorio predecesor
char aux_fileName[32];	// variable auxiliar para guardar temporalmente nombre del fichero
// estados para los ficheros
#define CLOSED 0
#define OPENED 1

// estado para el tipo de los inodos
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
	sb.magicNum = 2019; // un número que inventamos para identificar nuestro sistema de ficheros

	// recorrer todo el array de inodes para restablecer el nombre y los estados
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
	if (bwrite(DEVICE_IMAGE, 0, ((char *)&(sb))) == -1){
		printf("error bwrite\n");
		return -1;
	}

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
	int error = checkFile(path);
	if(error == -2){
		printf("error checkFile\n");
		return -1;
	}else if(error == -1){
		printf("error same name file\n");
		return -1;
	}

	// ahora procedemos a encontrar el id para el fichero a crear
	uint64_t pos = 1;	// mascara a 1
	uint64_t inodeMapCpy = sb.inodeMap;	// copia de inodeMap
	uint64_t position = 1;	// La primera posición de el mapa de inodes es 2^0, o sea, 1

	// recorre el mapa de inodes
	int i;
	for(i = 0; i < MAX_NUMBER_FILES && ((pos & inodeMapCpy) != 0); i++) {

		// el objetivo es encontrar 'i' que tenga valor 0 en el mapa de inodes
		// en la tabla de AND si devuelve 1, esa posición está ocupada, entonces pasamos a la siguiente posición.
		// y para ello multiplicamos por 2 la posición y desplazamos una posición el mapa.
		position *= 2;
		inodeMapCpy = inodeMapCpy >> 1;
	}

	// comprobar la cantidad de ficheros/directorios que se han creado
	if(strcmp(aux_preDir,"-")==0){	// en caso de directorio raiz
		if(suDirMaster<10){
			suDirMaster++;
		}else{
			printf("Out of range maximum file on the directory master\n");
			return -2;
		}
	}else{
		// sacar el id del inodo del directiorio predecesor
		int z;
		for(z=0; z<MAX_NUMBER_FILES; z++){
			if(strcmp(aux_preDir, preDir[z]))
				break;
		}
		if(suDir[z]<10){

			suDir[z]++;
		}else{
			printf("Out of range maximum file on the directory %s\n",iNodeNames[z]);
			return -2;
		}
	}



	// cuando encontramos la posición la almacenamos en el array de nombres de ficheros y en superbloque
	strcpy(iNodeNames[i], aux_fileName);
	sb.inodes[i].name = iNodeNames[i];
	sb.inodeMap += position;
	sb.inodes[i].directBlock = INIT_BLOCK+i+1;
	sb.inodes[i].type = FILE;
	levels[i] = aux_level;
	sb.inodes[i].level = aux_level;
	strcpy(preDir[i],aux_preDir);
	strcpy(sb.inodes[i].preDir,aux_preDir);

	printf("*** File named %s created with fd %i in level %i with predecesor Directoy %s\n ", aux_fileName, i, aux_level, aux_preDir);
	// si va todo bien devuelve 0
	return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *path)
{
	if(checkPath(path)!=0)	return -1;
	// sacar el nombre el fichero quitando su ruta
	char *pch;
	char line[132];  // where we will put a copy of the input
	char *fileName;
	strcpy(line, path);
	pch = strtok(line,"/"); // find the first double quote
	while(pch != NULL){
		fileName = pch;
		pch = strtok (NULL, "/");
	}


	// restar a 1 al array para guardar cantidades de ficheros
	if(strcmp(aux_preDir,"-")==0){	// en caso de directorio raiz
			suDirMaster--;
	}else{
		// sacar el id del inodo del directiorio predecesor
		int z;
		for(z=0; z<MAX_NUMBER_FILES&&(strcmp(aux_preDir, preDir[z])!=0); z++);
		suDir[z]--;
	}


	// conseguir el id que correponde al inodo con el nombre del fichero que hemos sacado
	int i;
	uint64_t position = 1;
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		//Se busca el fichero y se devuelve su posición
		if(strcmp(sb.inodes[i].name, fileName) == 0) {
			//Cuando encuentra el fichero, le resta la posicion en la que esta para dejarlo libre y borra el nombre
			sb.inodeMap -= position;
			strcpy(iNodeNames[i], "");
			sb.inodes[i].name = iNodeNames[i];
			sb.inodes[i].size = 0;
			sb.inodes[i].pointer = 0;
			fileState[i] = CLOSED;
			levels[i] = 0;
			sb.inodes[i].level = 0;
			strcpy(preDir[i],"");
			strcpy(sb.inodes[i].preDir,"");
			printf("*** File named %s with fd %i in level %i with predecesor Directoy %s is removed\n ", aux_fileName, i, aux_level, aux_preDir);
			return 0;
		}
		// si no encuentra el fichero pasa a la siguiente posicion
		position *= 2;
	}

	//Si no encuentra el fichero, devuelve error.
	printf("File not exist\n");
	return -1;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *path)
{
	if(checkPath(path)!=0)	return -2;
	// sacar el nombre el fichero quitando su ruta
	char *pch;
	char line[256];  // where we will put a copy of the input
	char *fileName;
	strcpy(line, path);
	pch = strtok(line,"/"); // find the first double quote
	while(pch != NULL){
		fileName = pch;
		pch = strtok (NULL, "/");
	}

	// conseguir el descriptor de fichero que correponde al inodo con el nombre del fichero que hemos sacado
	int fd = -1;
	int i;
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		//Se busca el fichero y se devuelve su posición
		if(strcmp(sb.inodes[i].name, fileName) == 0) {
			fd = i;
			break;
		}
	}

	//Si no existe el fichero, devuelve error
	if (fd == -1){
		printf("File not exist\n");
		return -1;
	}

	// Si el fichero ya está abierto se genera un error
	if(fileState[fd] == OPENED){
		printf("File already opened\n");
		return -2;
	}

	//Cambia el atributo a OPENED
	fileState[fd] = OPENED;

	//Pone el puntero del fichero al principio. Si esta ejecución es errónea, se devuelve error
	if (lseekFile(fd, FS_SEEK_BEGIN, 0) != 0) return -1;
	// Si todo es correcto, se devuelve el descriptor del fichero abierto

	printf("*** File named %s with fd %i in level %i with predecesor Directoy %s is opened now\n ", aux_fileName, fd, aux_level, aux_preDir);

	return fd;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fd)
{
	// Si el superbloque está fuera del rango se devuelve un error
	if (fd < 0 || fd > MAX_NUMBER_FILES-1){
		printf("Invalid fd\n");
		return -1;
	}

	// Si el superbloque que se quiere cerrar ya está cerrado, se genera un error.
	if(fileState[fd] == CLOSED){
		printf("error: file not exist or already closed\n");
		return -1;
	}

	// Si el superbloque que se quiere cerrar está abierto se cierra.
	fileState[fd] = CLOSED;

	printf("*** File named %s with fd %i in level %i with predecesor Directoy %s is closed now\n ", iNodeNames[fd], fd, levels[fd], preDir[fd]);
	//Si la ejecución es correcta, se devuelve 0.
	return 0;

}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fd, void *buffer, int numBytes)
{

	// Si el superbloque está fuera del rango se devuelve un error
	if (fd < 0 || fd > MAX_NUMBER_FILES-1){
		printf("Invalid fd\n");
		return -1;
	}
	// El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero
	if (numBytes < 0 || numBytes > MAX_FILE_SIZE){
		printf("Invalid numBytes\n");
		return -1;
	}

	// El fichero tiene que estar abierto para poder leer
	if (fileState[fd] != OPENED){
		printf("File not opened\n");
		return -1;
	}

	// Si en el mapa de inodes el bit correspondiente al descriptor de fichero es 0, se devuelve error
	uint64_t pos = 1;
	int i;
	//Se encuentra la posición del fichero
	for(i = 0; i<fd; i++) {
		pos *= 2;
	}

	//Se hace un AND con la máscara que vale 1. Si devuelve 0, esta libre y devuelve 0.
	if ((pos & sb.inodeMap) == 0) {
		return -1;
	}

	char block[BLOCK_SIZE];
	if(bread(DEVICE_IMAGE, fd+INIT_BLOCK, block)==-1){
		printf("error bread\n");
		return -1;
	}

	// Se almacenan los bytes solicitados del fichero en el buffer
	memmove(buffer, block, numBytes);

	printf("*** File named %s read with fd %i in level %i with predecesor Directoy %s\n ", iNodeNames[fd], fd, levels[fd], preDir[fd]);
	//Devuelve el número de Bytes leídos.
	return numBytes;

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
		printf("Invalid fd\n");
		return -1;
	}

	// El número de bytes a leer no puede ser mayor que el tamaño máximo de un fichero
	if (numBytes < 0 || numBytes > MAX_FILE_SIZE){
		printf("Invalid numBytes\n");
		return -1;
	}
	// El fichero tiene que estar abierto para poder leer
	if (fileState[fd] != OPENED){
		printf("File not opened\n");
		return -1;
	}
	// Si en el mapa de inodes el bit correspondiente al descriptor de fichero es 0, se devuelve error
	uint64_t pos = 1;
	int i;
	//Se encuentra la posición del fichero
	for(i = 0; i<fd; i++) {
		pos *= 2;
	}

	//Se hace un AND con la máscara que vale 1. Si devuelve 0, esta libre y devuelve 0.
	if ((pos & sb.inodeMap) == 0) {
		return -1;
	}


	//Se escribe en un bloque el buffer que almacena lo que se desa escribir
	char block[BLOCK_SIZE];
	memmove(block, buffer, numBytes);

	//Se actualiza el tamaño del fichero
	sb.inodes[fd].size = sizeof(buffer);

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
	printf("*** File named %s writed with fd %i in level %i with predecesor Directoy %s\n ", iNodeNames[fd], fd, levels[fd], preDir[fd]);
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
	if (fd < 0 || fd > MAX_NUMBER_FILES-1){
		printf("Invalid fd\n");
		return -1;
	}

	// Comprobar si existe el fichero
	if (strcmp(sb.inodes[fd].name,"")==0){
		printf("File not exists\n");
		return -1;
	}

	//Si se desea cambiar el puntero fuera del archivo, devuelve error.
	if (whence == FS_SEEK_CUR &&
		(sb.inodes[fd].pointer + offset > MAX_NUMBER_FILES-1 || sb.inodes[fd].pointer + offset < 0)){
			printf("Pointer out of range\n");
			return -1;
		}

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

	printf("*** File with fd %i named %s's pointer is now %i\n ", fd, iNodeNames[fd], sb.inodes[fd].pointer);
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
	int error = checkDir(path);
	if(error==-2){
		printf("error checkDir\n");
		return -1;
	}

	if(error == -1){
		printf("There is a directory with same name\n");
		return -1;
	}


	// comprobar la cantidad de ficheros/directorios que se han creado
	if(strcmp(aux_preDir,"-")==0){	// en caso de directorio raiz
		if(suDirMaster<10){
			suDirMaster++;
		}else{
			printf("Out of range maximum file on the directory master\n");
			return -2;
		}
	}else{
		// sacar el id del inodo del directiorio predecesor
		int z;
		for(z=0; z<MAX_NUMBER_FILES; z++){
			if(strcmp(aux_preDir, preDir[z]))
				break;
		}
		if(suDir[z]<10){

			suDir[z]++;
		}else{
			printf("Out of range maximum file on the directory %s\n",iNodeNames[z]);
			return -2;
		}
	}


	// ahora procedemos a encontrar el id para el fichero a crear
	uint64_t pos = 1;	// mascara a 1
	uint64_t inodeMapCpy = sb.inodeMap;	// copia de inodeMap
	uint64_t position = 1;	// La primera posición de el mapa de inodes es 2^0, o sea, 1
	// recorre el mapa de inodes
	int i;
	for(i = 0; i < MAX_NUMBER_FILES && ((pos & inodeMapCpy) != 0); i++) {

		// el objetivo es encontrar 'i' que tenga valor 0 en el mapa de inodes
		// tabla de AND para iNodeMapCpy y pos
		// si el AND devuelve 1, esa posición está ocupada, entonces pasamos a la siguiente posición.
		// y para ello multiplicamos por 2 la posición y desplazamos una posición el mapa.
		position *= 2;
		inodeMapCpy = inodeMapCpy >> 1;
	}

	// cuando encontramos la posición la almacenamos en el array de nombres de ficheros y en superbloque
	strcpy(iNodeNames[i], aux_fileName);
	strcpy(sb.inodes[i].name,iNodeNames[i]);
	//printf("sb.name is %s\n",sb.inodes[i].name);
	sb.inodeMap += position;
	sb.inodes[i].directBlock = INIT_BLOCK+i+1;
	sb.inodes[i].type = DIR;
	levels[i] = aux_level;
	//printf("level is %i\n",levels[i]);
	sb.inodes[i].level = aux_level;
	strcpy(preDir[i],aux_preDir);
	strcpy(sb.inodes[i].preDir,aux_preDir);
	//printf("preDir is %s\n", preDir[i]);

	// si va todo bien devuelve 0
	printf("*** Directory named %s created with fd %i in level %i with predecesor Directoy %s\n ", aux_fileName, i, aux_level, aux_preDir);
	return 0;
	/*************************************** End Student Code ***************************************/
}

/*
 * @brief	Deletes a directory, provided it exists in the file system.
 * @return	0 if success, -1 if the directory does not exist, -2 in case of error..
 */
int rmDir(char *path)
{
	if(checkPath(path)!=0){
		printf("error checkPath\n");
		return -2;
	}
	// encontrar el nombre del directorio
	char *pch;
	char line[256];  // where we will put a copy of the input
	char *dirName;
	strcpy(line, path);
	pch = strtok(line,"/"); // find the first double quote
	while(pch != NULL){
		dirName = pch;
		pch = strtok (NULL, "/");
	}


	// restar a 1 al array para guardar cantidades de ficheros
	if(strcmp(aux_preDir,"-")==0){	// en caso de directorio raiz
			suDirMaster--;
	}else{
		// sacar el id del inodo del directiorio predecesor
		int z;
		for(z=0; z<MAX_NUMBER_FILES&&(strcmp(aux_preDir, preDir[z])!=0); z++);
		suDir[z]--;
	}


	// encontrar el id del directorio y eliminarlo del inodo correspondiente
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
			sb.inodes[i].level = 0;
			strcpy(preDir[i],"");
			strcpy(sb.inodes[i].preDir,"");
			printf("*** Directory named %s with fd %i in level %i with predecesor Directoy %s is removed\n ", aux_fileName, i, aux_level, aux_preDir);
			return 0;
		}
		position *= 2;
	}

	//Si no encuentra el fichero, devuelve error.
	printf("Directory no exists\n");
	return -1;
}

/*
 * @brief	Lists the content of a directory and stores the inodes and names in arrays.
 * @return	The number of items in the directory, -1 if the directory does not exist, -2 in case of error..
 */
int lsDir(char *path, int inodesDir[10], char namesDir[10][33])
{
	// comprobar la ruta
	if(strcmp(path,"/")!=0){
		if(checkPath(path)!=0){
			printf("error checkPath\n");
			return -2;
		}
	}

	// encontrar el nombre del directorio de la ruta
	char *dirName;
	if(strcmp(path,"/")==0){	// si es raiz le damos '-'
		dirName ="-";
	}else{	// si no es raiz sacamos el ultimo elemento de /
		char *pch;
		char line[256];

		strcpy(line, path);
		pch = strtok(line,"/");
		while(pch != NULL){
			dirName = pch;
			pch = strtok (NULL, "/");
		}
	}

	int i;
	if(strcmp(path,"/")!=0){
		// comprobar si existe el directorio
		int fd = -1;
		for(i = 0; i < MAX_NUMBER_FILES; i++) {
			//Se busca el fichero y se devuelve su posición
			if(strcmp(sb.inodes[i].name, dirName) == 0) {
				fd = i;
				break;
			}
		}

		//Si no existe el directorio, devuelve error
		if (fd == -1){
			printf("Directory not exist\n");
			return -1;
		}
	}




	// pasar los nombres de los directorios y id a las variables correspondientes
	int index = 0;
	for(i=0; i<MAX_NUMBER_FILES; i++){
		if(strcmp(preDir[i],dirName) == 0){
			inodesDir[index] = i;
			strcpy(namesDir[index],iNodeNames[i]);
			index++;
		}
	}
	// lo que sobran se les asignan -1
	for(; index<10; index++){
		inodesDir[index] = -1;
		strcpy(namesDir[index],"");
	}
	// si va todo bien devuelve 0
	return 0;
}



/*************************************** Auxiliary Functions ***************************************/
int checkFile(char* path){
	if(path[0] != '/'||path[strlen(path)-1]=='/'||(strlen(path))>132){
		printf("path incorrect format\n");
		return -2;
	}


  char line[132];
	strcpy(line, path);	// guardamos el path en line ya que lo vamos a modificar
	char *fileWay[4];	// array de 4 dimensiones para guardar los 4 niveles de ficheros/directorios
  char *pch;	// variable para guardar string entre '/'
  int maxLevel=0;	// variable para guardar el nivel donde se encuentra el fichero

	pch = strtok(line,"/"); // sacar el primer string después de '/'
	do{
    fileWay[maxLevel] = pch;	// array para guardar los string que va sacando
		pch = strtok (NULL, "/");	// sacar los string consecutivos hasta agotar los '/'
    maxLevel++;	// aumentar el nuvel
	}while(pch != NULL&&maxLevel<4);



	// comprobar si existe la ruta
	int i,j;
	if(maxLevel>=2){	// procedemos a comprobar la existencia de la ruta solo si hay hay mas de 2 niveles

		int routeExisted[maxLevel-1];	// array para almacenar la ruta
		for(j=0; j<=maxLevel-2; j++){
			for(i = 0; i < MAX_NUMBER_FILES; i++) {
				if((strcmp(sb.inodes[i].name, fileWay[j]) == 0)&&(sb.inodes[i].type==DIR)&&(levels[i]==(j))){
					//el directorio existe si encontramos un inodo con mismo nombre, de tipo directorio y en el mismo nivel
					routeExisted[j] = 1;
				}
			}
		}

		// si no existe el directorio da error
		for(i=0; i<maxLevel-1; i++){
			if(routeExisted[i]!=1){
				printf("Directory does not exist\n");
				return -2;
			}

		}
	}

	// Comprobar si el fichero a crear tiene mas de 32 caracteres de longitud
	if(strlen(fileWay[maxLevel-1])>MAX_NAME_LENGHT){
		printf("File name has more than 32 characters\n");
		return -2;
	}


	// Ahora comprobamos si existen ficheros con el mismo nombre
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		if((strcmp(sb.inodes[i].name, fileWay[maxLevel-1]) == 0)&&(levels[i]==(maxLevel-1))){
			//el fichero existe si encontramos un inodo con mismo nombre en el mismo nivel
			if((maxLevel-1)>0){	// si no estamos en el directorio raiz
				if(strcmp(preDir[i], fileWay[maxLevel-2]) == 0){	// comprobar si tienen el mismo directorio predecesor
					printf("File or directory with same name already exists\n");
					return -1;
				}
			}else{
				printf("File or directory with same name already exists\n");
				return -1;
			}

		}
	}
	// guardar las variables auxiliares globales para luego actualizarlas en la funcion mkDir
	aux_level = maxLevel-1;	// el nivel del directorio que queremos crear ahora
	if(maxLevel>=2){
		strcpy(aux_preDir,fileWay[maxLevel-2]);
	}else{
		strcpy(aux_preDir,"-");	// si es en directorio raiz le asignamos su predecesor '-'
	}
	strcpy(aux_fileName,fileWay[maxLevel-1]);	// el nombre es el ultimo string se se saca
	return 0;
}


int checkDir(char* path){
	if(path[0] != '/'||path[strlen(path)-1]=='/'||strlen(path)>99){
		printf("path incorrect format\n");
		return -2;
	}




	  char line[99];
		strcpy(line, path);	// guardamos el path en line ya que lo vamos a modificar
		char *fileWay[4];	// array de 4 dimensiones para guardar los 4 niveles de ficheros/directorios
	  char *pch;	// variable para guardar string entre '/'
	  int maxLevel=0;	// variable para guardar el nivel donde se encuentra el fichero

		pch = strtok(line,"/"); // sacar el primer string después de '/'
		do{
	    fileWay[maxLevel] = pch;	// array para guardar los string que va sacando
			pch = strtok (NULL, "/");	// sacar los string consecutivos hasta agotar los '/'
	    maxLevel++;	// aumentar el nuvel
		}while(pch != NULL&&maxLevel<4);

		// comprobar si existe la ruta
		int i,j;
		if(maxLevel>=2){	// procedemos a comprobar la existencia de la ruta solo si hay hay mas de 2 niveles

			int routeExisted[maxLevel-1];
			for(j=0; j<=maxLevel-2; j++){
				for(i = 0; i < MAX_NUMBER_FILES; i++) {
					if((strcmp(sb.inodes[i].name, fileWay[j]) == 0)&&(sb.inodes[i].type==DIR)&&(levels[i]==(j))){
						//el directorio existe si encontramos un inodo con mismo nombre, de tipo directorio y en el mismo nivel
						routeExisted[j] = 1;
					}
				}
			}

			// si no existe el directorio da error
			for(i=0; i<maxLevel-1; i++){
				if(routeExisted[i]!=1){
					printf("Directory does not exist\n");
					return -2;
				}

			}
		}


		// Comprobar si el directorio a crear tiene mas de 32 caracteres de longitud
		if(strlen(fileWay[maxLevel-1])>MAX_NAME_LENGHT){
			printf("Directory name has more than 32 characters\n");
			return -2;
		}

	// Comprobar si existe un fichero o directorio con el mismo nombre
	for(i = 0; i < MAX_NUMBER_FILES; i++) {
		if((strcmp(sb.inodes[i].name, fileWay[maxLevel-1]) == 0)&&(levels[i]==(maxLevel-1))){
			printf("There already exist file or directory with the same name\n");
			return -1;
		}
	}



	// guardar las variables auxiliares globales para luego actualizarlas en la funcion mkDir
	aux_level = maxLevel-1;	// el nivel del directorio que queremos crear ahora
	if(maxLevel>=2){
		strcpy(aux_preDir,fileWay[maxLevel-2]);
	}else{
		strcpy(aux_preDir, "-");	// si es en directorio raiz le asignamos su predecesor '-'
	}
	strcpy(aux_fileName, fileWay[maxLevel-1]);	// el nombre es el ultimo string se se saca
	return 0;
}

int checkPath(char *path){
	if(path[0] != '/'||path[strlen(path)-1]=='/'||(strlen(path))>99){
		printf("path incorrect format\n");
		return -2;
	}


	char line[132];
	strcpy(line, path);	// guardamos el path en line ya que lo vamos a modificar
	char *fileWay[4];	// array de 4 dimensiones para guardar los 4 niveles de ficheros/directorios
	char *pch;	// variable para guardar string entre '/'
	int maxLevel=0;	// variable para guardar el nivel donde se encuentra el fichero

	pch = strtok(line,"/"); // sacar el primer string después de '/'
	do{
		fileWay[maxLevel] = pch;	// array para guardar los string que va sacando
		pch = strtok (NULL, "/");	// sacar los string consecutivos hasta agotar los '/'
		maxLevel++;	// aumentar el nuvel
	}while(pch != NULL&&maxLevel<4);



	// comprobar si existe la ruta
	int i,j;
	if(maxLevel>=2){	// procedemos a comprobar la existencia de la ruta solo si hay hay mas de 2 niveles

		int routeExisted[maxLevel-1];	// array para almacenar la ruta
		for(j=0; j<=maxLevel-2; j++){
			for(i = 0; i < MAX_NUMBER_FILES; i++) {
				if((strcmp(sb.inodes[i].name, fileWay[j]) == 0)&&(sb.inodes[i].type==DIR)&&(levels[i]==(j))){
					//el directorio existe si encontramos un inodo con mismo nombre, de tipo directorio y en el mismo nivel
					routeExisted[j] = 1;
				}
			}
		}

		// si no existe el directorio da error
		for(i=0; i<maxLevel-1; i++){
			if(routeExisted[i]!=1){
				printf("Path does not exist\n");
				return -2;
			}

		}
	}

	aux_level = maxLevel-1;	// el nivel del directorio que queremos crear ahora
	if(maxLevel>=2){
		strcpy(aux_preDir, fileWay[maxLevel-2]);
	}else{
		strcpy(aux_preDir , "-");	// si es en directorio raiz le asignamos su predecesor '-'
	}
	strcpy(aux_fileName , fileWay[maxLevel-1]);	// el nombre es el ultimo string se se saca

	return 0;
}
