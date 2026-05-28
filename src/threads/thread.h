#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/fixed_points.h"

/* Estados possíveis na vida de uma thread. */
enum thread_status
  {
    THREAD_RUNNING,     /* Thread em execução. */
    THREAD_READY,       /* Pronta para executar, mas aguardando a CPU. */
    THREAD_BLOCKED,     /* Bloqueada, aguardando um evento. */
    THREAD_DYING        /* Prestes a ser destruída. */
  };

typedef int tid_t;
#define TID_ERROR ((tid_t) -1)

/* Limites e valor padrão de prioridade. */
#define PRI_MIN 0         /* Prioridade mínima. */
#define PRI_DEFAULT 31    /* Prioridade padrão. */
#define PRI_MAX 63        /* Prioridade máxima. */

/*
 * Estrutura que representa uma thread (ou processo de kernel).
 *
 * Cada thread ocupa exatamente uma página de 4 KB:
 * - A struct fica na base (offset 0).
 * - A pilha do kernel cresce para baixo a partir do topo da página.
 *
 * CAMPOS NOVOS adicionados para este projeto:
 *
 * [Alarm Clock]
 *   sleep_elem   — elemento de lista usado na sleeping_list (timer.c).
 *                  Separado do elem para não conflitar com ready_list/semáforos.
 *   wakeup_tick  — tick absoluto em que esta thread deve ser acordada.
 *
 * [MLFQS]
 *   recent_cpu   — estimativa de quanto tempo de CPU esta thread usou
 *                  recentemente, armazenada em ponto fixo (fixed_t).
 *   nice         — valor "nice" da thread: positivo = cede CPU (menos prioritária),
 *                  negativo = egoísta (mais prioritária). Varia de -20 a +20.
 */
struct thread
  {
    /* Gerenciado por thread.c */
    tid_t tid;                          /* Identificador único da thread. */
    enum thread_status status;          /* Estado atual da thread. */
    char name[16];                      /* Nome (para depuração). */
    uint8_t *stack;                     /* Ponteiro salvo para a pilha. */
    int priority;                       /* Prioridade de escalonamento. */
    struct list_elem allelem;           /* Elemento na lista de todas as threads. */

    /* NOVO (Alarm Clock): campos para o mecanismo de sleep sem busy-wait */
    struct list_elem sleep_elem;        /* Elemento na sleeping_list do timer. */
    int64_t wakeup_tick;                /* Tick em que esta thread deve acordar. */

    /* NOVO (MLFQS): métricas do escalonador de múltiplas filas */
    fixed_t recent_cpu;                 /* Uso recente de CPU (ponto fixo). */
    int nice;                           /* Valor nice (-20 a +20). */

    /* Compartilhado entre thread.c e synch.c */
    struct list_elem elem;              /* Elemento na ready_list ou waiters. */

#ifdef USERPROG
    uint32_t *pagedir;                  /* Diretório de páginas (user process). */
#endif

    unsigned magic;                     /* Detecta estouro de pilha. */
  };

/* Se false (padrão): usa escalonador round-robin.
   Se true: usa o escalonador MLFQS (Multi-Level Feedback Queue Scheduler).
   Controlado pela opção de linha de comando do kernel "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);
void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/*
 * NOVAS declarações (MLFQS): funções públicas chamadas pelo timer_interrupt
 * em timer.c para atualizar as métricas do escalonador.
 */
void mlfqs_update_priority(struct thread *t, void *aux);    /* recalcula prioridade */
void mlfqs_update_recent_cpu(struct thread *t, void *aux);  /* decaimento de recent_cpu */
void mlfqs_update_load_avg(void);                           /* atualiza load_avg global */
void mlfqs_increment_recent_cpu(void);                      /* +1 na thread atual */

typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

#endif /* threads/thread.h */
