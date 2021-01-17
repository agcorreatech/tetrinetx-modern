#include <stdio.h>
#include <stdarg.h>

void stringVariavel(char *format,...);

int main(int argc,char **argv)
{

  int numero=10; char *nome="Alex";

  printf("String Variavel\n");
  stringVariavel("Numero: %d\n",numero);
  stringVariavel("String: %s\n",nome);
  stringVariavel("Numero: %d - String: %s\n",numero,nome);
  return 0; 
}

void stringVariavel(char *format,...)
{

  va_list va; // Declare uma variável do tipo va_list na função, o nome "argptr" pode ser trocado.
  static char SBUF2[1050];

  va_start(va,format); /* Inicie a função va_start colocando o nome da variável de tipo va_list declarada anteriormente e o nome do último parâmetro conhecido*/ 
  
  vsprintf(SBUF2,format,va); // Retorna todos os parâmetros que estiverem dentro de va
  printf("%s", SBUF2);

  va_end(va); // Sempre coloque a função va_end no final. A va_end recebe o nome da variável va_list declarada anteriormente.

  return;
}
