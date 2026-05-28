#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* Veja [8254] para detalhes de hardware do chip timer 8254. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Número de ticks do timer desde que o SO foi iniciado. */
static int64_t ticks;

/*
 * NOVO (Alarm Clock): Lista global de threads dormindo,
 * ordenada pelo tick em que cada thread deve ser acordada.
 */
static struct list sleeping_list;

/* Número de loops por tick do timer.
   Inicializado por timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/*
 * MODIFICADO (Alarm Clock): Inicializa o timer.
 *
 * Além de configurar o chip PIT e registrar a interrupção de hardware,
 * agora também inicializa a sleeping_list, que é a lista encadeada
 * usada para guardar as threads que estão dormindo.
 */
void
timer_init (void) 
{
  list_init(&sleeping_list); /* NOVO: inicializa a lista de threads dormindo */
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

/*
 * NOVA (Alarm Clock): Função de comparação para inserção ordenada na sleeping_list.
 *
 * Compara duas threads pelo tick em que devem ser acordadas (wakeup_tick).
 * Se os ticks forem iguais, a thread com maior prioridade vem primeiro.
 * Isso garante que a lista fique sempre ordenada do menor para o maior
 * wakeup_tick, permitindo que o timer_interrupt acorde as threads
 * de forma eficiente sem percorrer a lista inteira.
 */
static bool
sort_by_wakeup_tick(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  struct thread *thread_a = list_entry(a, struct thread, sleep_elem);
  struct thread *thread_b = list_entry(b, struct thread, sleep_elem);
  if (thread_a->wakeup_tick == thread_b->wakeup_tick) {
    return thread_a->priority > thread_b->priority;
  }
  return thread_a->wakeup_tick < thread_b->wakeup_tick;
}

/* Calibra loops_per_tick, usado para implementar pequenos atrasos. */
void
timer_calibrate (void) 
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1)) 
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (loops_per_tick | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Retorna o número de ticks do timer desde que o SO foi iniciado. */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Retorna o número de ticks decorridos desde THEN. */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}

/*
 * MODIFICADO (Alarm Clock + MLFQS): Handler da interrupção do timer.
 *
 * Chamado a cada tick do hardware. Agora possui duas responsabilidades
 * adicionais além de incrementar o contador global de ticks:
 *
 * 1. [MLFQS] Atualiza as métricas do escalonador:
 *    - A cada tick: incrementa recent_cpu da thread atual em execução.
 *    - A cada segundo (TIMER_FREQ ticks): recalcula load_avg e recent_cpu
 *      de todas as threads.
 *    - A cada 4 ticks: recalcula a prioridade de todas as threads.
 *
 * 2. [Alarm Clock] Acorda threads cujo wakeup_tick chegou:
 *    - Percorre o início da sleeping_list (que está ordenada por wakeup_tick).
 *    - Se o tick atual >= wakeup_tick da thread da frente, ela é removida
 *      da lista e desbloqueada com thread_unblock().
 *    - Para assim que encontrar uma thread cujo tempo ainda não chegou,
 *      pois a lista é ordenada e não há razão de continuar.
 */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();
  
  /* [MLFQS] Atualizações periódicas das métricas do escalonador */
  if (thread_mlfqs) {
    mlfqs_increment_recent_cpu();              /* a cada tick */
    if (ticks % TIMER_FREQ == 0) {            /* a cada segundo */
      mlfqs_update_load_avg();
      thread_foreach(mlfqs_update_recent_cpu, NULL);
    }
    if (ticks % 4 == 0) {                     /* a cada 4 ticks */
      thread_foreach(mlfqs_update_priority, NULL);
    }
  }

  /* [Alarm Clock] Acorda threads cujo tempo de sono acabou */
  while (!list_empty(&sleeping_list)) {
    struct list_elem *e = list_front(&sleeping_list);
    struct thread *t = list_entry (e, struct thread, sleep_elem);

    if (ticks >= t->wakeup_tick) {
      list_pop_front(&sleeping_list);
      thread_unblock(t);
    } else {
      break; /* lista ordenada: se esta não acordou, as próximas também não */
    }
  }
}

/*
 * MODIFICADO (Alarm Clock): Coloca a thread atual para dormir por TICKS ticks.
 *
 * Implementação original usava busy-wait (loop ocupado consumindo CPU).
 * Nova implementação usa espera bloqueada:
 *
 * 1. Calcula o tick exato em que a thread deve ser acordada (wakeup_tick).
 * 2. Desabilita interrupções para evitar condição de corrida.
 * 3. Registra o wakeup_tick na struct da thread atual.
 * 4. Insere a thread na sleeping_list de forma ordenada por wakeup_tick.
 * 5. Chama thread_block() para bloquear a thread (cede a CPU).
 * 6. Ao ser desbloqueada pelo timer_interrupt, reabilita as interrupções
 *    e retorna normalmente.
 *
 * Desta forma, nenhum ciclo de CPU é desperdiçado enquanto a thread dorme.
 */
void
timer_sleep (int64_t ticks) 
{
  if (ticks <= 0) {
    return; /* nada a fazer para valores inválidos */
  }

  int64_t start = timer_ticks ();
  int64_t wakeup_time = start + ticks; /* tick absoluto para acordar */

  ASSERT (intr_get_level () == INTR_ON);
  
  enum intr_level old_level = intr_disable(); /* seção crítica */
  struct thread *t = thread_current();
  
  t->wakeup_tick = wakeup_time; /* registra quando deve acordar */

  /* insere na lista ordenada de threads dormindo */
  list_insert_ordered(&sleeping_list, &t->sleep_elem, sort_by_wakeup_tick, NULL);

  thread_block(); /* bloqueia — CPU liberada para outras threads */

  intr_set_level (old_level); /* reabilita interrupções ao acordar */
}

/* Dorme por aproximadamente MS milissegundos. */
void
timer_msleep (int64_t ms) 
{
  real_time_sleep (ms, 1000);
}

/* Dorme por aproximadamente US microssegundos. */
void
timer_usleep (int64_t us) 
{
  real_time_sleep (us, 1000 * 1000);
}

/* Dorme por aproximadamente NS nanossegundos. */
void
timer_nsleep (int64_t ns) 
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

void
timer_mdelay (int64_t ms) 
{
  real_time_delay (ms, 1000);
}

void
timer_udelay (int64_t us) 
{
  real_time_delay (us, 1000 * 1000);
}

void
timer_ndelay (int64_t ns) 
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

static bool
too_many_loops (unsigned loops) 
{
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  start = ticks;
  busy_wait (loops);

  barrier ();
  return start != ticks;
}

static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}

static void
real_time_sleep (int64_t num, int32_t denom) 
{
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      timer_sleep (ticks); 
    }
  else 
    {
      real_time_delay (num, denom); 
    }
}

static void
real_time_delay (int64_t num, int32_t denom)
{
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000)); 
}
