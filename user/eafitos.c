#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAX_INPUT 100
#define MAX_ARGS 10

// FUNCIONES
// se ejecutan en el mismo proceso de la shell (no necesitan exec).
void listar();
void leer(char *filename);
void tiempo();
void calc(char *a, char *op, char *b);
void ayuda();

// MAIN
int
main(void)
{
  // args, terminado en NULL
  char input[MAX_INPUT];
  char *args[MAX_ARGS];

  while(1){
    // Prompt de la shell interactiva.
    printf("EAFITos> ");

    // lee desde el teclado con stdin
    gets(input, MAX_INPUT);

    // Eliminar '\n'
    for(int k = 0; input[k] != 0; k++){
      if(input[k] == '\n'){
        input[k] = 0;
        break;
      }
    }

    if(input[0] == 0)
      continue;

    // Parser
    // Convierte "comando arg1 arg2" en:
    // args[0] -> "comando"
    // y reemplaza espacios por '\0' para separar tokens.
    int argc = 0;
    int inword = 0;

    for(int j = 0; input[j] != 0; j++){
      if(input[j] != ' ' && inword == 0){
        args[argc++] = &input[j];
        inword = 1;
      }
      else if(input[j] == ' '){
        input[j] = 0;
        inword = 0;
      }
    }

    args[argc] = 0;

    // COMANDOS INTERNOS
    // se resuelven dentro de la shell sin crear otro programa.

    if(strcmp(args[0], "listar") == 0){
      listar();
    }

    else if(strcmp(args[0], "leer") == 0){
      if(args[1] != 0)
        leer(args[1]);
      else
        printf("Uso: leer <archivo>\n");
    }

    else if(strcmp(args[0], "tiempo") == 0){
      tiempo();
    }

    else if(strcmp(args[0], "calc") == 0){
      if(args[1] && args[2] && args[3])
        calc(args[1], args[2], args[3]);
      else
        printf("Uso: calc <n1> <op> <n2>\n");
    }

    else if(strcmp(args[0], "ayuda") == 0){
      ayuda();
    }

    else if(strcmp(args[0], "salir") == 0){
      printf("Saliendo de EAFITos...\n");
      exit(0);
    }

    // COMANDO EXTERNO
    else{
      // fork crea un proceso hijo duplicando la shell actual
      if(fork() == 0){
        // En el hijo:
        // Si exec tiene exito no retorna.
        exec(args[0], args);
        // Si retorna, hubo error por ejemplo un comando no definido
        printf("Comando no reconocido\n");
        exit(1);
      }
      // En el padre:
      // wait sincroniza
      wait(0);
    }
  }

  exit(0);
}

// IMPLEMENTACIONES

void
listar()
{
  // Se ejecuta "ls" como proceso hijo 
  if(fork() == 0){
    char *args[] = {"ls", 0};
    exec("ls", args);
    exit(0);
  }
  // El padre espera al hijo antes de mostrar el próximo mensaje
  wait(0);
}

void
leer(char *filename)
{
  int fd;
  char buf[128];
  int n;

  fd = open(filename, O_RDONLY);
  if(fd < 0){
    printf("No se pudo abrir el archivo\n");
    return;
  }

  // Lectura por bloques:
  // read retorna cuántos bytes reales leyó en cada iteración.
  while((n = read(fd, buf, sizeof(buf))) > 0){
    write(1, buf, n);
  }

  close(fd);
}

void
tiempo()
{
  // Un tick es una unidad de tiempo del temporizador del kernel 
  //(Todavia no encuentro la forma de poner la hora normal)
  printf("Ticks desde inicio del sistema: %d\n", uptime());
}

void
calc(char *a, char *op, char *b)
{
  // Conversión de texto a entero (entrada de usuario a dato numérico)
  int x = atoi(a);
  int y = atoi(b);

  if(strcmp(op, "+") == 0)
    printf("%d\n", x + y);
  else if(strcmp(op, "-") == 0)
    printf("%d\n", x - y);
  else if(strcmp(op, "*") == 0)
    printf("%d\n", x * y);
  else if(strcmp(op, "/") == 0){
    if(y == 0)
      printf("Error: división por cero\n");
    else
      printf("%d\n", x / y);
  }
  else
    printf("Operador no válido\n");
}

void
ayuda()
{
  // Ayuda mínima integrada en la shell.
  printf("\n=== EAFITos Shell ===\n");
  printf("Comandos disponibles:\n");
  printf("listar\n");
  printf("leer <archivo>\n");
  printf("tiempo\n");
  printf("calc <n1> <op> <n2>\n");
  printf("ayuda\n");
  printf("salir\n");
  printf("=====================\n\n");
}
