/* Este arquivo é derivado do código fonte do sistema operacional instrucional Nachos. */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*
 * Inicializa o semáforo SEMA com o valor VALUE.
 * Um semáforo é um inteiro não-negativo com dois operadores atômicos:
 * - down (P): espera o valor ser positivo, depois decrementa.
 * - up   (V): incrementa o valor (e acorda uma thread, se houver).
 */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);
  sema->value = value;
  list_init (&sema->waiters);
}

/*
 * Operação down (P) no semáforo.
 * Se o valor for 0, bloqueia a thread atual até que o semáforo seja liberado.
 * Ao ser acordada, decrementa o valor e continua.
 */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_push_back (&sema->waiters, &thread_current ()->elem);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

/*
 * Operação down (P) sem bloqueio.
 * Tenta decrementar o semáforo; retorna true se bem-sucedido,
 * false se o valor já era 0. Pode ser chamada de dentro de interrupções.
 */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/*
 * MODIFICADO (MLFQS): Operação up (V) no semáforo.
 *
 * Incrementa o valor do semáforo e acorda uma thread da lista de espera.
 *
 * Modificação: ao final, se o modo MLFQS estiver ativo e as interrupções
 * estiverem habilitadas (não estamos dentro de um handler de interrupção),
 * chama thread_yield(). Isso garante que, ao liberar um recurso, o
 * escalonador possa imediatamente dar a CPU para a thread de maior
 * prioridade — que pode ser a que acabou de ser desbloqueada.
 */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));
  sema->value++;
  intr_set_level (old_level);

  /* NOVO: cede a CPU para garantir que a thread de maior prioridade execute */
  if (thread_mlfqs && intr_get_level() == INTR_ON) {
    thread_yield();
  }
}

static void sema_test_helper (void *sema_);

void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Inicializa o lock: holder = NULL, semáforo interno inicializado em 1. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);
  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Adquire o lock, bloqueando se necessário até ele estar disponível. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
}

/* Tenta adquirir o lock sem bloquear. Retorna true se conseguiu. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Libera o lock. Deve ser chamado apenas pela thread que o possui. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  lock->holder = NULL;
  sema_up (&lock->semaphore);
}

/* Retorna true se a thread atual é a detentora do lock. */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);
  return lock->holder == thread_current ();
}

/* Estrutura auxiliar: semáforo dentro de uma lista (usado em cond_wait). */
struct semaphore_elem 
  {
    struct list_elem elem;
    struct semaphore semaphore;
  };

/* Inicializa a variável de condição COND. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);
  list_init (&cond->waiters);
}

/*
 * Libera o lock e espera que COND seja sinalizado.
 * Após ser sinalizado, readquire o lock antes de retornar.
 * Implementa monitor no estilo Mesa (o chamador deve re-verificar a condição).
 */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  
  sema_init (&waiter.semaphore, 0);
  list_push_back (&cond->waiters, &waiter.elem);
  lock_release (lock);
  sema_down (&waiter.semaphore);
  lock_acquire (lock);
}

/* Sinaliza uma thread esperando em COND (acorda a da frente da lista). */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) 
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
}

/* Acorda todas as threads esperando em COND. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}
