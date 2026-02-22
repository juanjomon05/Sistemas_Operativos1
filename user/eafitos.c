#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_INPUT 100
#define MAX_ARGS 10

// Parser
// - Recibe una línea y la separa por espacios.
// - Convierte cada espacio en '\0' para terminar cada "palabra" en memoria.
void parse(char *input, char **args) {
  int i = 0;

  while (*input != 0) {

    // Salta espacios consecutivo
    // para separar tokens dentro del mismo buffer.
    while (*input == ' ')
      *input++ = 0;

    if (*input == 0)
      break;

    args[i++] = input;

    // Avanza hasta el final del token actual.
    while (*input != ' ' && *input != 0)
      input++;
  }

  // Marca fin de argv para exec.
  args[i] = 0;
}

// 1) se utiliza un fork, duplica el proceso actual (padre e hijo).
// 2) hijo -> exec: reemplaza su imagen por el programa solicitado.
// 3) padre -> wait: espera a que el hijo termine.
void execute_command(char **args) {
  int pid = fork();

  if (pid < 0) {
    // Error al crear proceso hijo.
    printf("fork falló\n");
    return;
  }

  if (pid == 0) {
    // Código del hijo:
    // exec no crea proceso nuevo, reutiliza este proceso hijo
    // y carga el programa indicado en args[0].
    exec(args[0], args);
    // Si exec retorna es porque hubo error
    printf("Error ejecutando comando\n");
    exit(1);
  } else {
    // Código del padre:
    // espera al hijo para hacer el proceso secuencial
    wait(0);
  }
}

int
main(void)
{
  char input[MAX_INPUT];
  char *args[MAX_ARGS];

  printf("=== EAFITos Shell v1 ===\n");

  while (1) {

    printf("EAFITos> ");

    // Limpia el buffer y lee una línea de entrada.
    memset(input, 0, sizeof(input));
    gets(input, MAX_INPUT);

    // Si la línea está vacía, vuelve a pedir comando.
    if (input[0] == 0)
      continue;

    // Convierte la línea en argv.
    parse(input, args);

    if (args[0] == 0)
      continue;

    // No usa exec porque se resuelve dentro del mismo proceso.
    if (strcmp(args[0], "salir") == 0) {
      printf("Saliendo...\n");
      exit(0);
    }

    execute_command(args);
  }

  exit(0);
}
