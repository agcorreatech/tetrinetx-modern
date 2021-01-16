/*
  crack.h
  
  This contains my IP independent algorithm to decrypt the initial encrypted
  initstring sent by the client to the server. 3 days work ;)
  
  Addedum (15/9/98): The actual encryption algorithm has now been kindly
                     provided by BlueCoder, found in #tetrinet on EFNET,
                     who is making his own TetriNET clone which is compatible
                     with TetriNET.
                     If I do include it, it will be found very likely in
                     crack.c, with the name blu_decrypt, blu_encrypt.
                     As I'm not creating a client, I don't need to use the
                     exact algorithm, and so my own techniques are used here.
*/

/* Takes a decrypted string in hex pairs, removes salt, and converts it to ascii */
char *tet_dec2str(char *buf);

/* Takes an encrypted string, and overwrites it with the decrypted string */
/* This function "learns" how to decrypt the encryption on the fly, to allow */
/* IP independent decryption (Lets say that that was a side effect ;) */
int tet_decrypt(char *buf);

/* Takes an encrypted hex pair, and decryptes it to a decrypted hex pair, based */
/* on one of 10 "normalised" encryption techniques */
unsigned char tet_decryptchar(int enctype, unsigned char prev, unsigned char cur);
