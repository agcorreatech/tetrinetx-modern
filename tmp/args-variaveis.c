#include <stdio.h>
#include <stdarg.h>

void imprimeNumeros(int,...);

int main(int argc,char **argv)
{
  printf("Argumentos Variaveis\n");
  imprimeNumeros(5,5,4,3,2,1);
  imprimeNumeros(4,5,4,3,2);
  imprimeNumeros(2,8,8);
  return 0; 
}

void imprimeNumeros(int quantidadeDeParametros,...)
{
  int numero;
  int contador=0;

  va_list argptr; // Declare uma variável do tipo va_list na função, o nome "argptr" pode ser trocado.
  va_start(argptr,quantidadeDeParametros); /* Inicie a função va_start colocando o nome da variável de tipo va_list declarada anteriormente e o nome do último parâmetro conhecido*/ 
  
  for(contador=1;contador<=quantidadeDeParametros;contador++) /* Estabeleça uma forma de controle, pois a função não saberá quantos parâmetros recebeu.*/
  {
    numero=va_arg(argptr,int);
    printf("%d ",numero);
   /* Cada vez que for acionada, a va_arg retornará o valor de um parâmetro variável, caso acabem os parâmetros variáveis,   ele retornará lixo de memória, necessitando de um controle. O va_arg tem como parâmetro a variável va_list declarada anteriormente e o tipo de dado do argumento variável.*/
  }
    va_end(argptr); // Sempre coloque a função va_end no final. A va_end recebe o nome da variável va_list declarada anteriormente.

printf("\n");

  return;
}
