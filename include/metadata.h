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


#include <stdint.h>

#define MAX_NUMBER_FILES 40 // NF1
#define MAX_NAME_LENGHT 32  // NF3
#define BLOCK_SIZE 2048 // NF6
#define MAX_FILE_SIZE 2048  // NF7

struct INodo;
struct Superblock;


typedef struct INode {
  uint8_t type; // 0 si es fichero y 1 si es directorio
  char *name; // nombre del fichero o directorio
  uint8_t directBlock; // número del bloque que al que pertenece
  uint16_t size; // tamaño del fichero en bytes
  uint16_t pointer; // puntero del fichero, si es directorio no lo tiene
} INode;

typedef struct Superblock {
  uint16_t magicNum; // número mágico que indentifica nuestro sistema de ficheros
  long disk_size; // el tamaño del disco
  uint64_t inodeMap; // mapa de bits
  struct INode inodes[MAX_NUMBER_FILES]; // array de inodos
} Superblock;
