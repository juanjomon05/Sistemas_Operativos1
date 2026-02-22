#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// ----------------------------------------
// Utilidades
// ----------------------------------------

// Capacidad teórica del pipe en xv6 (referencia del kernel).
#define PIPESIZE_REF 512

// Escribe exactamente n bytes, reintentando si es necesario.
static int write_exact(int fd, const void *buf, int n) {
  int total = 0;
  const char *p = (const char*)buf;
  while (total < n) {
    int r = write(fd, p + total, n - total);
    if (r < 0) return r;
    total += r;
  }
  return total;
}


// Rellena un buffer con un patrón repetitivo legible.
static void fill_pattern(char *buf, int n, char base) {
  for (int i = 0; i < n; i++) buf[i] = (char)(base + (i % 26));
}

// Imprime un separador visual.
static void banner(const char *title) {
  printf("\n================ %s ================\n", title);
}


// ----------------------------------------
// Ejercicio 1. Ping–pong con pipe y EOF
// ----------------------------------------

static void demo_ping_pong(void){

  banner("Ejercicio 1: ping-pong + EOF");

  // Usamos dos pipes para comunicación bidireccional:
  // p2c: padre -> hijo, c2p: hijo -> padre.
  int p2c[2];
  int c2p[2];
  if(pipe(p2c) < 0 || pipe(c2p) < 0){
    printf("pipe error\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("fork error\n");
    close(p2c[0]); close(p2c[1]);
    close(c2p[0]); close(c2p[1]);
    return;
  }

  if(pid == 0){
    char buf[16] = {0};

    // El hijo solo lee de p2c[0] y solo escribe en c2p[1].
    // Cerrar extremos no usados evita fugas y bloqueos inesperados.
    close(p2c[1]);
    close(c2p[0]);

    // Primer mensaje: padre envia "ping".
    int n = read(p2c[0], buf, sizeof(buf));
    printf("E1 hijo: recibi '%s' (%d bytes)\n", buf, n);

    // Segundo read sobre el mismo extremo:
    // como el padre ya cerro su escritura, read debe retornar 0 (EOF).
    int eof = read(p2c[0], buf, sizeof(buf));
    printf("E1 hijo: read() despues del cierre del padre = %d (EOF esperado=0)\n", eof);

    // Respuesta al padre por el canal inverso.
    write_exact(c2p[1], "pong", 5);
    close(c2p[1]);
    close(p2c[0]);
    exit(0);
  }

  // Padre: escribe en p2c[1] y lee en c2p[0].
  close(p2c[0]);
  close(c2p[1]);

  write_exact(p2c[1], "ping", 5);
  // Cerrar escritura para que el hijo observe EOF.
  close(p2c[1]);

  char buf[16] = {0};
  int n = read(c2p[0], buf, sizeof(buf));
  printf("E1 padre: recibi '%s' (%d bytes)\n", buf, n);

  // Igual que antes: cuando el hijo cierre c2p[1], este read retorna 0.
  int eof = read(c2p[0], buf, sizeof(buf));
  printf("E1 padre: read() despues del cierre del hijo = %d (EOF esperado=0)\n", eof);

  close(c2p[0]);
  wait(0);
}


// ----------------------------------------
// Ejercicio 2. Redirección con dup + exec (wc)
// ----------------------------------------

static void
demo_redirección(void)
{
  banner("Ejercicio 2: dup + exec con wc");

  // Un solo pipe: padre escribe datos, hijo los recibe por stdin.
  int p[2];
  if(pipe(p) < 0){
    printf("E2: pipe fallo\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("E2: fork fallo\n");
    close(p[0]);
    close(p[1]);
    return;
  }

  if(pid == 0){
    // Queremos que fd 0 (stdin) apunte al extremo de lectura del pipe.
    close(0);
    if(dup(p[0]) < 0){
      printf("E2 hijo: dup fallo\n");
      exit(1);
    }
    // Tras dup, ya no necesitamos estos fds originales.
    close(p[0]);
    close(p[1]);

    // wc leerá desde stdin (que ahora viene del pipe).
    char *argv[] = { "wc", 0 };
    exec("wc", argv);
    printf("E2 hijo: exec wc fallo\n");
    exit(1);
  }

  // Padre: solo escribe, por eso cierra el extremo de lectura.
  close(p[0]);

  // Enviamos varias líneas para que wc cuente líneas/palabras/bytes.
  write_exact(p[1], "linea uno\n", 10);
  write_exact(p[1], "linea dos\n", 10);
  write_exact(p[1], "linea tres\n", 11);

  // EOF para wc: sin este close, wc puede quedar esperando más entrada.
  close(p[1]);
  wait(0);
}



// ----------------------------------------
// Ejercicio 3. Bloqueo y sincronización: pipe vacío y lleno
// ----------------------------------------

static void
demo_bloqueo_vacio(void)
{
  banner("Ejercicio 3a: bloqueo con pipe vacio");

  int p[2];
  if(pipe(p) < 0){
    printf("E3a: pipe fallo\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("E3a: fork fallo\n");
    close(p[0]);
    close(p[1]);
    return;
  }

  if(pid == 0){
    char buf[32] = {0};
    uint t0, t1;

    // Hijo lector: intenta leer primero para forzar bloqueo por pipe vacío.
    close(p[1]);

    t0 = uptime();
    printf("E3a hijo: antes de read, ticks=%d\n", t0);

    // Aquí read duerme hasta que alguien escriba.
    int n = read(p[0], buf, sizeof(buf));

    t1 = uptime();
    printf("E3a hijo: despues de read, ticks=%d, bloqueo=%d ticks, n=%d, msg='%s'\n",
           t1, t1 - t0, n, buf);

    close(p[0]);
    exit(0);
  }

  close(p[0]);
  // Espera breve para dejar al hijo bloqueado dentro de read().
  pause(20);

  printf("E3a padre: escribiendo para desbloquear al hijo\n");
  write_exact(p[1], "dato-desbloqueo\n", 15);
  close(p[1]);
  wait(0);
}

static void
demo_bloqueo_lleno(void)
{
  banner("Ejercicio 3b: bloqueo con pipe lleno");
  printf("E3b: referencia teorica PIPESIZE=%d bytes\n", PIPESIZE_REF);

  int p[2];
  if(pipe(p) < 0){
    printf("E3b: pipe fallo\n");
    return;
  }

  int pid = fork();
  if(pid < 0){
    printf("E3b: fork fallo\n");
    close(p[0]);
    close(p[1]);
    return;
  }

  if(pid == 0){
    char buf[128];
    int total = 0;
    int n;

    // Hijo lector: duerme para dejar que el padre llene el pipe.
    close(p[1]);
    pause(20);

    printf("E3b hijo: comienzo a leer y liberar espacio del pipe\n");
    // Leer libera espacio; eso despierta al escritor si estaba bloqueado.
    while((n = read(p[0], buf, sizeof(buf))) > 0)
      total += n;

    printf("E3b hijo: EOF, total leido=%d bytes\n", total);
    close(p[0]);
    exit(0);
  }

  close(p[0]);

  // N > PIPESIZE fuerza escrituras parciales y posible bloqueo del escritor.
  const int N = 2000;
  char big[N];
  uint t0, t1;

  fill_pattern(big, N, 'a');

  t0 = uptime();
  printf("E3b padre: antes de write_exact(%d), ticks=%d\n", N, t0);

  int r = write_exact(p[1], big, N);

  t1 = uptime();
  // Si hubo bloqueo, la duración crece hasta que el hijo consume datos.
  printf("E3b padre: despues de write_exact, ticks=%d, duracion=%d ticks, retorno=%d\n",
         t1, t1 - t0, r);

  close(p[1]);
  wait(0);
}


// ----------------------------------------
// main(): ejecuta las demostraciones
// ----------------------------------------

int
main(void)
{
  demo_ping_pong();
  demo_redirección();
  demo_bloqueo_vacio();
  demo_bloqueo_lleno();

  printf("\n[OK] pipes2: ejercicios completados.\n");
  exit(0);
}
