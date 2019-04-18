/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	01/03/2017
 */

/*************************************** Begin Student Code ***************************************/
 #include <stdint.h>

 //Declaramos las funciones propias que hemos implementado
 int listAll();

 /*
  * @brief 	Verifies if an i-node is used or not.
  * @return 	0 if the i-node is not used and -1 if it's used.
  */
 int iNodoUsed(uint64_t iNodoMap, int fileDescriptor);

 /*
  * @brief 	Find the file with the parameter name.
  * @return 	fd if the filename exist, and -1 in case of error.
  */
 int findByName(char *filename);

 /*
  * @brief 	Generates a Checksum for the file system
  * @return 	-1 if error, and 0 if success
  */
 int generateFSChecksum();

/*************************************** End Student Code ***************************************/
