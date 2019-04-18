/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	01/03/2017
 */

#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}



/*************************************** Begin Student Code ***************************************/
#include <stdint.h>

#define MAX_FILES 64
#define MAX_NAME_LENGHT 32
#define MAX_FILE_SIZE 2048
#define BLOCK_SIZE 2048
#define DATA_BLOCKS_START 2
#define OPENED 1
#define CLOSED 0
#define FSCLEAN 0
#define FSACTIVE 1

struct INodo;
struct Superblock;


typedef struct INodo {
  char *name; //Nombre del fichero
  uint8_t directBlock; //Bloque asociado
  uint8_t opened; //Indica si está abierto o no
  uint16_t bytesUsed; //Bytes usados por el fichero
  uint16_t pointer; //Puntero del fichero
  uint16_t checksum; //Utilizado para comprobar la integridad
} INodo;

typedef struct Superblock {
  long diskSize; //Tamaño del disco
  uint64_t INodoMap; //Mpa de indos
  struct INodo iNodos[MAX_FILES]; //Array donde se almacenan los i-nodos
  uint16_t checksum; //Utilizado para comprobar la integridad
} Superblock;

/*************************************** End Student Code ***************************************/