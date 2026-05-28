#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/fixed_points.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

#define THREAD_MAGIC 0xcd6abf4b
#define A 55

/*
 * NOVO (MLFQS): Média de carga do sistema (load_avg).
 * Armazenada em ponto fixo. Representa quantas threads em média
 * estão prontas ou em execução no último minuto.
 * Inicializada como 0 em thread_init().
 */
static fixed_t load_avg;

/* Lista de processos no estado THREAD_READY. */
static struct list ready_list;

/* Lista de todos os processos do sistema. */
static struct list all_list;

static struct thread *idle_thread;
static struct thread *initial_thread;
static struct lock tid_lock;

struct kernel_thread_frame 
  {
    void *eip;
    thread_func *function;
    void *aux;
  };

static long long idle_ticks;
static long long kernel_ticks;
static long long user_ticks;

#define TIME_SLICE 4
static unsigned thread_ticks;

bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);
static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

/* Declarações das funções MLFQS e de ordenação (implementadas abaixo) */
void mlfqs_update_priority(struct thread *t, void *aux);
void mlfqs_update_recent_cpu(struct thread *t, void *aux);
void mlfqs_update_load_avg(void);
void mlfqs_increment_recent_cpu(void);
bool sort_by_priority(const struct list_elem *a, const struct list_elem *b, void *aux);

/*
 * MODIFICADO (MLFQS): Inicialização do sistema de threads.
 *
 * Além das inicializações originais (filas, lock de tid, thread inicial),
 * agora inicializa load_avg = 0 em formato de ponto fixo,
 * necessário para os cálculos do MLFQS antes do primeiro segundo de execução.
 */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);

  load_avg = int_to_fp (0); /* NOVO: inicializa load_avg para o MLFQS */

  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
}

/* Inicia o escalonamento preemptivo habilitando interrupções. */
void
thread_start (void) 
{
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);
  intr_enable ();
  sema_down (&idle_started);
}

/* Chamado pelo handler de interrupção do timer a cada tick. */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();

  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  thread_ticks++;
  if (thread_ticks >= TIME_SLICE){
    intr_yield_on_return ();
  }
}

void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Cria uma nova thread do kernel com o nome, prioridade e função dados. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  thread_unblock (t);

  return tid;
}

/* Coloca a thread atual para dormir até thread_unblock() ser chamado. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/*
 * MODIFICADO (MLFQS / Escalonamento por prioridade): Desbloqueia a thread T.
 *
 * Na versão original, a thread era simplesmente inserida no final da
 * ready_list com list_push_back().
 *
 * Na nova versão, usa list_insert_ordered() com sort_by_priority,
 * garantindo que a ready_list fique sempre ordenada por prioridade
 * decrescente. Assim, o escalonador sempre pega a thread de maior
 * prioridade na frente da lista.
 */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);

  /* MODIFICADO: inserção ordenada por prioridade (maior primeiro) */
  list_insert_ordered(&ready_list, &t->elem, sort_by_priority, NULL);
  t->status = THREAD_READY;
  intr_set_level (old_level);
}

const char *
thread_name (void) 
{
  return thread_current ()->name;
}

struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);
  return t;
}

tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/*
 * MODIFICADO (Escalonamento por prioridade): Cede a CPU voluntariamente.
 *
 * Na versão original, usava list_push_back() para reinserir a thread na
 * ready_list.
 *
 * Agora usa list_insert_ordered() com sort_by_priority, mantendo a lista
 * ordenada por prioridade. Isso garante que, ao ceder a CPU, a thread
 * volte para a posição correta na fila conforme sua prioridade atual.
 */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    /* MODIFICADO: inserção ordenada por prioridade */
    list_insert_ordered(&ready_list, &cur->elem, sort_by_priority, NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Aplica a função func em todas as threads do sistema. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/*
 * MODIFICADO (MLFQS): Define a prioridade da thread atual.
 *
 * No modo MLFQS, a prioridade é gerenciada automaticamente pelo escalonador
 * (calculada com base em recent_cpu e nice), portanto chamadas manuais
 * a thread_set_priority() são ignoradas quando thread_mlfqs == true.
 */
void
thread_set_priority (int new_priority) 
{
  if (thread_mlfqs) {
    return; /* MLFQS: prioridade não pode ser definida manualmente */
  }
  thread_current ()->priority = new_priority;
}

/* Retorna a prioridade atual da thread em execução. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/*
 * MODIFICADO (MLFQS): Define o valor nice da thread atual.
 *
 * O valor nice influencia diretamente o cálculo de prioridade no MLFQS:
 * nice positivo = "gentil", cede mais CPU (menor prioridade);
 * nice negativo = "egoísta", pede mais CPU (maior prioridade).
 *
 * Após alterar o nice, recalcula imediatamente a prioridade da thread
 * e chama thread_yield() para que o escalonador possa preemptar se
 * necessário (outra thread pode ter prioridade maior agora).
 */
void
thread_set_nice (int nice) 
{
  thread_current ()->nice = nice;
  mlfqs_update_priority(thread_current(), NULL); /* recalcula prioridade */
  thread_yield(); /* pode precisar ceder a CPU */
}

/* Retorna o valor nice atual da thread em execução. */
int
thread_get_nice (void) 
{
  return thread_current ()->nice;
}

/*
 * NOVA (MLFQS): Recalcula a prioridade de uma thread.
 *
 * Fórmula do MLFQS:
 *   priority = PRI_MAX - (recent_cpu / 4) - (nice * 2)
 *
 * - PRI_MAX = 63: ponto de partida máximo.
 * - recent_cpu / 4: threads que usaram mais CPU recentemente
 *   recebem penalidade de prioridade.
 * - nice * 2: threads "gentis" (nice alto) têm prioridade reduzida.
 *
 * O resultado é limitado entre PRI_MIN (0) e PRI_MAX (63).
 * A idle_thread é ignorada (não participa do escalonamento normal).
 */
void mlfqs_update_priority(struct thread *t, void *aux UNUSED)
{
  if (t == idle_thread) {
    return;
  }

  int priority = PRI_MAX - fp_to_int_rounded(div_fp_int(t->recent_cpu, 4)) - (t->nice * 2);
  if (priority > PRI_MAX) priority = PRI_MAX;
  if (priority < PRI_MIN) priority = PRI_MIN;
  t->priority = priority;
}

/*
 * NOVA (MLFQS): Atualiza o recent_cpu de uma thread.
 *
 * Fórmula:
 *   recent_cpu = (2*load_avg) / (2*load_avg + 1) * recent_cpu + nice
 *
 * O coeficiente (2*load_avg)/(2*load_avg+1) é um fator de decaimento
 * exponencial: quanto maior a carga do sistema, mais devagar o
 * recent_cpu decai. Isso reflete quanto tempo de CPU a thread
 * consumiu "recentemente" (nas últimas dezenas de segundos).
 *
 * Chamada a cada segundo para todas as threads via thread_foreach().
 * A idle_thread é ignorada.
 */
void mlfqs_update_recent_cpu(struct thread *t, void *aux UNUSED)
{
  if (t == idle_thread) {
    return;
  }

  fixed_t temp = mult_fp_int(load_avg, 2);
  fixed_t coeficient = div_fp(temp, add_fp_int(temp, 1));

  t->recent_cpu = add_fp_int(mult_fp(coeficient, t->recent_cpu), t->nice);
}

/*
 * NOVA (MLFQS): Atualiza a média de carga do sistema (load_avg).
 *
 * Fórmula:
 *   load_avg = (59/60) * load_avg + (1/60) * ready_threads
 *
 * É uma média móvel exponencial do número de threads prontas ou
 * em execução (excluindo idle_thread). O coeficiente 59/60
 * faz com que o histórico recente tenha mais peso.
 *
 * Chamada uma vez por segundo (a cada TIMER_FREQ ticks) pelo timer_interrupt.
 * ready_threads inclui a thread atual em execução (se não for idle).
 */
void mlfqs_update_load_avg(void)
{
  int ready_threads = list_size(&ready_list);
  if (thread_current() != idle_thread) {
    ready_threads++; /* conta a thread em execução */
  }

  fixed_t coeficient_1 = div_fp_int(int_to_fp(59), 60); /* 59/60 */
  fixed_t coeficient_2 = div_fp_int(int_to_fp(1), 60);  /* 1/60  */
  
  load_avg = add_fp(  
      mult_fp(coeficient_1, load_avg),
      mult_fp(coeficient_2, int_to_fp(ready_threads))
    );
}

/*
 * NOVA (MLFQS): Incrementa o recent_cpu da thread em execução em 1.
 *
 * Chamada a cada tick pelo timer_interrupt.
 * Representa que a thread atual utilizou mais 1 tick de CPU.
 * A idle_thread não conta, pois ela não representa trabalho real.
 */
void mlfqs_increment_recent_cpu(void)
{
  if (thread_current() == idle_thread) {
    return;
  }
  thread_current()->recent_cpu = add_fp_int(thread_current()->recent_cpu, 1);
}

/*
 * MODIFICADO (MLFQS): Retorna 100 vezes o load_avg do sistema.
 *
 * Multiplica por 100 e arredonda para inteiro para expor o valor
 * em ponto fixo como um inteiro escalado (evita floats no kernel).
 */
int
thread_get_load_avg (void) 
{
  return fp_to_int_rounded(mult_fp_int(load_avg, 100));
}

/*
 * MODIFICADO (MLFQS): Retorna 100 vezes o recent_cpu da thread atual.
 *
 * Mesmo princípio do load_avg: escala por 100 para representar
 * decimais sem usar ponto flutuante.
 */
int
thread_get_recent_cpu (void) 
{
  fixed_t recent_cpu = thread_current ()->recent_cpu;
  return fp_to_int_rounded (mult_fp_int(recent_cpu, 100));
}

/* Thread idle: executada quando nenhuma outra thread está pronta. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      intr_disable ();
      thread_block ();
      asm volatile ("sti; hlt" : : : "memory");
    }
}

static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);
  intr_enable ();
  function (aux);
  thread_exit ();
}

struct thread *
running_thread (void) 
{
  uint32_t *esp;
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/*
 * MODIFICADO (MLFQS): Inicialização básica de uma struct thread.
 *
 * Além dos campos originais (status, nome, stack, prioridade, magic),
 * agora inicializa os campos do MLFQS:
 * - nice = 0: valor padrão neutro (sem preferência).
 * - recent_cpu = 0: thread recém-criada ainda não usou CPU.
 */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->magic = THREAD_MAGIC;
  t->nice = 0;                      /* NOVO: inicializado para MLFQS */
  t->recent_cpu = int_to_fp (0);    /* NOVO: inicializado para MLFQS */
  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
}

static void *
alloc_frame (struct thread *t, size_t size) 
{
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);
  t->stack -= size;
  return t->stack;
}

static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  cur->status = THREAD_RUNNING;
  thread_ticks = 0;

#ifdef USERPROG
  process_activate ();
#endif

  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/*
 * NOVA (Escalonamento por prioridade): Função de comparação para a ready_list.
 *
 * Usada como comparador em list_insert_ordered() dentro de thread_unblock()
 * e thread_yield(). Retorna true se a thread A tem prioridade maior que B,
 * garantindo que a ready_list fique ordenada do maior para o menor,
 * e o escalonador sempre execute a thread mais prioritária.
 */
bool
sort_by_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct thread *t_a = list_entry(a, struct thread, elem);
  struct thread *t_b = list_entry(b, struct thread, elem);
  
  return t_a->priority > t_b->priority;
}

uint32_t thread_stack_ofs = offsetof (struct thread, stack);
