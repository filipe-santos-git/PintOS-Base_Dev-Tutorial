# Introduzindo PintOS

<details>
<summary>Mudanças no ./src </summary>

- Para facilitar, em geral os comandos de make já estão com os path's dos programas certos, mas se precisar em geral tem que usar o comando `export PATH=$PATH:<Pintos>/src/utils`; Assim em geral para rodar só precisa do comando de `make check`

- Para funcionar no Arch Linux modifiquei o src/Makefile.build:93 para ele reduzir o tamanho do loader.bin em todas os testes;

- Adicionado logica para ir executando os testes em especifico, no caso do threads, basicamente usa `make test TEST=<nome_do_test>`;

</details>

<details>
<summary>Recursos necessários </summary>

1.
   <details>
   <summary>Make </summary>

   - O make serve para ajudar nossa vida. Não há uma obrigatoriedade de usá-lo, mas iremos;
   - Caso necessite instalá-lo, utilize o comando `sudo apt install make`;
   </details>

2.
   <details>
   <summary>GCC e GDB </summary>

   - Compilador e Depurador para C;
   - Caso necessite instalá-los, utilize os comandos `sudo apt install gcc` e `sudo apt install gdb`;
   </details>

3.
   <details>
   <summary>Qemu </summary>

   - Recurso necessário para executar o sistema nos casos testes;
   - Caso necessite instalá-lo, utilize o comando `sudo apt install qemu-system-i386`;
   </details>

4.
   <details>
   <summary>Boch </summary>

   - Alternativa mais rápida ao Qemu, entretanto não utilizaremos ele;
   - Se desejar saber como fazê-lo funcionar, acesse `https://web.stanford.edu/class/cs140/projects/pintos/pintos_12.html#SEC167`;
   </details>

</details>

<details>
<summary>WSL </summary>

- Caso opte por não fazer em sua máquina com Linux ou não possua permissão para baixar algum recurso no computador, siga o tutorial oficial oficial `https://learn.microsoft.com/pt-br/windows/wsl/install`;

</details>

<details>
<summary>Tutorial de comandos inicias</summary>

- Uma vez que todos os recursos já estiverem instalados e sua branch desse repositório devidamente clonado, basta executar alguns `make`antes de começar a modificar o arquivo;
- Primeiramente, pelo terminal, vá até a pasta `src/utils` do repositório clonado e execute `make`, isso vai gerar alguns executáveis que o projeto usa para testar o código;
- Em seguida dependendo a parte do projeto que se está a pasta vai mudar mas o básico para testagem é entrar na pasta, dentro de `src/` da parte do projeto, 1°: `threads/` , 2°: `userprog/` , 3°: `vm/` , 4°: `filesys/`
- Dentro da pasta certa execute o comando de `make`, ele vai criar o diretório `build/`, em que basicamente vai estar o estado do sistema atual.
- Para executar os testes usa o comando `make check` dentro desse diretório, por padrão executando no terminal e mostrando os testes que passaram.
- Caso deseje executar novamente, lembre-se de dar `make clean`, dentro da pasta `build/`, antes de usar o proximo `make check`;
- Existe o comando `make check VERBOSE=1` fará com que tudo seja executado de maneira mais limpa em que cada teste aparecerá no terminal apenas durante sua execução;
- Caso você queira, pode ir na pasta `src/tests/`, os subdiretórios basicamente contém os códigos dos respectivos testes; Pode ser interessante olhar para ver como cada teste funciona, principalmente na 1° parte do projeto.
- Todos os testes executados geram alguns arquivos relatando a saida e se passou ou não, que ficam na pasta `build/tests`;

</details>

<details>
<summary>Observações </summary>

- Todas as operações descritas foram verificadas também no WSL;
- É possivel executar os comandos do Boch normalmente, porém se você não tiver ajustado ele, os resultados dos testes ficarão quase todas de falhas (testado no WSL);
- Há a opção de seguir o tutorial no arquivo `Exec_pintos.pdf` caso não queira fazer as modificações por si mesmo;

</details>

### Objetivos

#### Parte 1

- [ ] Alarm Clock;
- [ ] Advanced Scheduler - Multi-Level Feedback Queue (mlfqs);

#### Parte 2

- [ ] Alrgument Passing;
- [ ] User Memory Acess;
- [ ] System Calls;
- [ ] Process Termination;
- [ ] Denying Writes to Executables;

#### Parte 3

- [ ] Paging;
- [ ] Stack Growth;
- [ ] Memory Mapped Files;
- [ ] Acessing User Memory;

#### Parte 4

- [ ] Indexed and Extensible Files;
- [ ] Subdirectories;
- [ ] Buffer Cache;
- [ ] Synchronization;

### Detalhamentos

<details>
   <summary>Parte 1</summary>
<details>
   <summary>Objetivos Principais</summary>

   Modificar o PintOS para que a lógica de sleep/wake funcione no alarme de forma devida e implementar devidamente a mlfqs. Nesse processo, os arquivos a serem modificados devem ser apenas os `src/device/timer.` e o `src/threads/thread.`;

</details>
<details>
   <summary>Alarm</summary>

   Reimplementar `timer_sleep()` no `device/time.c` que ta originalmente implementado com 'busy wait', que fica chamando `thread_yield()` enquanto o tempo não tiver passado;

- Ideia:
   `Implementar um estado bloqueado para auxiliar e permitir corrigir o alarme`

</details>
<details>
   <summary>Scheduler</summary>

   Na documentação oficial, ele sugere para dar opção de ter o mlfqs ou o por prioridade, ou seja, implementar ambos, porém nosso projeto não almeja a implementação do por prioridade; com o mlfqs as prioridades definidas pelas threads devem ser ignoradas e controladas pelo escalonador;

   [Fila esquema](https://www.google.com/url?sa=i&url=https://medium.com/@francescofranco_39234/multilevel-feedback-queue-3ae862436a95&psig=AOvVaw0uPvTNvKvDx0bKwYGvKyn_&ust=1718223750727000&source=images&cd=vfe&opi=89978449&ved=0CBIQjRxqFwoTCLD727Sw1IYDFQAAAAAdAAAAABAI)

   Segundo o apêndice que fala do scheduler devemos implementar o conceito de `avg_load`, `thread_nice` e o `cpu_recent_time`;

   O `avg_load` é a carga média do sistema levando em conta a quantidade de threads em ready_list, sem incluir thread ociosa:

   ```math
   avg = (\frac{59}{60}) * avg + (\frac{1}{60}) * (tamanho-da-ready-list)
   ```

   O `cpu_recent_time` é uma média móvel exponencial, específica de cada thread e que começa em 0, que serve como peso na hora de calcular a prioridade, que consiste em considerar uma função exponencial em que com o passar do temp os cpu-time antigos tenham pesos menores e os mais recentes os pesos maiores; todas as threads devem ter seu recent time recalculados 1 vez por segundo (timer_ticks() % TIMER_FREQ == 0) usando:

   ```math
   CpuTime = ( \frac{2 * avg}{2 * avg + 1} * CpuTime + nice) * 100
   ```

   O `nice` é específico de cada thread, há funções a se implementar e fazê-lo funcionar corretamente; ele deve estar entre -20 e 20 e vai servir para calcular a prioridade em que quanto mais positivo, menor a prioridade, que será calculada usando o `recent_time` (apenas se ele mudar) para alterar a thread de fila na mlfq, usando a fórmula:

   ```math
   p = floor(PriMax - (\frac{RecentCpuTime}{4}) - (nice * 2))
   ```

##### Pontos Flutuantes

   O kernel não suporta float nem double, então a documentação recomenda usar o formato de 17.14, 17 bits para a parte inteira e 14 para a fracionária; Para transformar reais nesses tipos basta multiplicar por 2^Q, onde Q é o numero de bits separado para a parte fracionária, e truncar para int, a documentação recomenda usar isso no recent cpu time e no avg, basicamente simulando operações em float usando inteiros (ver [aqui](https://www.scs.stanford.edu/23wi-cs212/pintos/pintos_7.html) como as operações podem ser feitas);
</details>

<details>
    <summary>Tests</summary>

- Esses são todos os testes que serão executados quando usar o comando `make check` (caso não altere o `scr/tests/threads/tests.c`):
- Obs: Pode passar `make check -j<numero de nproc>` para ele rodar os testes de forma paralela, para descobrir um valor bom de nproc é só rodar o comando `nproc`, que retorna o número de unidades de processamento disponíveis no sistema ou para o processo atual.

   | # | Teste | Implementada | Testada | Funcionando |
   |---|-----------|:-----------:|:-------:|:-----------:|
   | 1  | `alarm-single`|      ❌     |    ❌    |      ❌      |
   | 2  | `alarm-multiple`|      ❌     |    ❌    |      ❌      |
   | 3  | `alarm-simultaneous`|      ❌     |    ❌    |      ❌      |
   | 4  | `alarm-priority`*|      ❌     |    ❌    |      ❌      |
   | 5  | `alarm-zero`|      ❌     |    ❌    |      ❌      |
   | 6  | `alarm-negative`|      ❌     |    ❌    |      ❌      |
   | 7  | `priority-change`*|      ❌     |    ❌    |      ❌      |
   | 8  | `priority-donate-one`*|      ❌     |    ❌    |      ❌      |
   | 9  | `priority-donate-multiple`*|      ❌     |    ❌    |      ❌      |
   | 10 | `priority-donate-multiple2`*|      ❌     |    ❌    |      ❌      |
   | 11 | `priority-donate-nest`*|      ❌     |    ❌    |      ❌      |
   | 12 | `priority-donate-sema`*|      ❌     |    ❌    |      ❌      |
   | 13 | `priority-donate-lower`*|      ❌     |    ❌    |      ❌      |
   | 14 | `priority-fifo`*|      ❌     |    ❌    |      ❌      |
   | 15 | `priority-preempt`*|      ❌     |    ❌    |      ❌      |
   | 16 | `priority-sema`*|      ❌     |    ❌    |      ❌      |
   | 17 | `priority-condvar`*|      ❌     |    ❌    |      ❌      |
   | 18 | `priority-donate-chain`*|      ❌     |    ❌    |      ❌      |
   | 19 | `mlfqs-load-1`|      ❌     |    ❌    |      ❌      |
   | 20 | `mlfqs-load-60`|      ❌     |    ❌    |      ❌      |
   | 21 | `mlfqs-load-avg`|      ❌     |    ❌    |      ❌      |
   | 22 | `mlfqs-recent-1`|      ❌     |    ❌    |      ❌      |
   | 23 | `mlfqs-fair-2`|      ❌     |    ❌    |      ❌      |
   | 24 | `mlfqs-fair-20`|      ❌     |    ❌    |      ❌      |
   | 25 | `mlfqs-nice-2`|      ❌     |    ❌    |      ❌      |
   | 26 | `mlfqs-nice-10`|      ❌     |    ❌    |      ❌      |
   | 27 | `mlfqs-block`|      ❌     |    ❌    |      ❌      |

</details>
<details>
   <summary>Detalhes</summary>

- Para nossa aplicação do projeto de Infraestrutura de Software, nenhum dos testes de priority serão exigidos;
- Os testes do alarm, quando executado, podem sinalizar que estão funcionando, porém, estão em espera ocupado, portanto não estão devidamente implementado;
- Há esses vídeos de guia sobre o assunto, caso necessite de ajuda: `https://www.youtube.com/watch?v=myO2bs5LMak` e `https://www.youtube.com/watch?v=57r9OCN1EfA` (são aulas sobre a implementação do projeto completo do PintOS);

</details>
</details>
<details>
   <summary>Parte 2</summary>
<details>
   <summary>Objetivos Principais</summary>
   Implementação da lógica referente a execução de programas no sistema operacional, passando por passagem de argumentos, acesso de memória, system calls, término de processos e permissões de execução; Vai modificar basicamente os arquivos nos diretórios `userprog/`  e `filesys/`
 </details>

<details>
   <summary>Argument Passing</summary>
 </details>

<details>
   <summary>User Memory Acess</summary>
 </details>

<details>
   <summary>System Calls</summary>
 </details>

<details>
   <summary>Process Termination and Wait</summary>
 </details>

<details>
   <summary>Denying Writes to executables</summary>
 </details>

 <details>
    <summary>Tests</summary>

| #  | Teste                                | Implementada | Testada | Funcionando |
|----|--------------------------------------|:------------:|:-------:|:-----------:|
| 1  | `userprog/args-none`                 |      ❌      |   ❌    |      ❌      |
| 2  | `userprog/args-single`               |      ❌      |   ❌    |      ❌      |
| 3  | `userprog/args-multiple`             |      ❌      |   ❌    |      ❌      |
| 4  | `userprog/args-many`                 |      ❌      |   ❌    |      ❌      |
| 5  | `userprog/args-dbl-space`            |      ❌      |   ❌    |      ❌      |
| 6  | `userprog/sc-bad-sp`                 |      ❌      |   ❌    |      ❌      |
| 7  | `userprog/sc-bad-arg`                |      ❌      |   ❌    |      ❌      |
| 8  | `userprog/sc-boundary`               |      ❌      |   ❌    |      ❌      |
| 9  | `userprog/sc-boundary-2`             |      ❌      |   ❌    |      ❌      |
| 10 | `userprog/sc-boundary-3`             |      ❌      |   ❌    |      ❌      |
| 11 | `userprog/halt`                      |      ❌      |   ❌    |      ❌      |
| 12 | `userprog/exit`                      |      ❌      |   ❌    |      ❌      |
| 13 | `userprog/create-normal`             |      ❌      |   ❌    |      ❌      |
| 14 | `userprog/create-empty`              |      ❌      |   ❌    |      ❌      |
| 15 | `userprog/create-null`               |      ❌      |   ❌    |      ❌      |
| 16 | `userprog/create-bad-ptr`            |      ❌      |   ❌    |      ❌      |
| 17 | `userprog/create-long`               |      ❌      |   ❌    |      ❌      |
| 18 | `userprog/create-exists`             |      ❌      |   ❌    |      ❌      |
| 19 | `userprog/create-bound`              |      ❌      |   ❌    |      ❌      |
| 20 | `userprog/open-normal`               |      ❌      |   ❌    |      ❌      |
| 21 | `userprog/open-missing`              |      ❌      |   ❌    |      ❌      |
| 22 | `userprog/open-boundary`             |      ❌      |   ❌    |      ❌      |
| 23 | `userprog/open-empty`                |      ❌      |   ❌    |      ❌      |
| 24 | `userprog/open-null`                 |      ❌      |   ❌    |      ❌      |
| 25 | `userprog/open-bad-ptr`              |      ❌      |   ❌    |      ❌      |
| 26 | `userprog/open-twice`                |      ❌      |   ❌    |      ❌      |
| 27 | `userprog/close-normal`              |      ❌      |   ❌    |      ❌      |
| 28 | `userprog/close-twice`               |      ❌      |   ❌    |      ❌      |
| 29 | `userprog/close-stdin`               |      ❌      |   ❌    |      ❌      |
| 30 | `userprog/close-stdout`              |      ❌      |   ❌    |      ❌      |
| 31 | `userprog/close-bad-fd`              |      ❌      |   ❌    |      ❌      |
| 32 | `userprog/read-normal`               |      ❌      |   ❌    |      ❌      |
| 33 | `userprog/read-bad-ptr`              |      ❌      |   ❌    |      ❌      |
| 34 | `userprog/read-boundary`             |      ❌      |   ❌    |      ❌      |
| 35 | `userprog/read-zero`                 |      ❌      |   ❌    |      ❌      |
| 36 | `userprog/read-stdout`               |      ❌      |   ❌    |      ❌      |
| 37 | `userprog/read-bad-fd`               |      ❌      |   ❌    |      ❌      |
| 38 | `userprog/write-normal`              |      ❌      |   ❌    |      ❌      |
| 39 | `userprog/write-bad-ptr`             |      ❌      |   ❌    |      ❌      |
| 40 | `userprog/write-boundary`            |      ❌      |   ❌    |      ❌      |
| 41 | `userprog/write-zero`                |      ❌      |   ❌    |      ❌      |
| 42 | `userprog/write-stdin`               |      ❌      |   ❌    |      ❌      |
| 43 | `userprog/write-bad-fd`              |      ❌      |   ❌    |      ❌      |
| 44 | `userprog/exec-once`                 |      ❌      |   ❌    |      ❌      |
| 45 | `userprog/exec-arg`                  |      ❌      |   ❌    |      ❌      |
| 46 | `userprog/exec-bound`                |      ❌      |   ❌    |      ❌      |
| 47 | `userprog/exec-bound-2`              |      ❌      |   ❌    |      ❌      |
| 48 | `userprog/exec-bound-3`              |      ❌      |   ❌    |      ❌      |
| 49 | `userprog/exec-multiple`             |      ❌      |   ❌    |      ❌      |
| 50 | `userprog/exec-missing`              |      ❌      |   ❌    |      ❌      |
| 51 | `userprog/exec-bad-ptr`              |      ❌      |   ❌    |      ❌      |
| 52 | `userprog/wait-simple`               |      ❌      |   ❌    |      ❌      |
| 53 | `userprog/wait-twice`                |      ❌      |   ❌    |      ❌      |
| 54 | `userprog/wait-killed`               |      ❌      |   ❌    |      ❌      |
| 55 | `userprog/wait-bad-pid`              |      ❌      |   ❌    |      ❌      |
| 56 | `userprog/multi-recurse`             |      ❌      |   ❌    |      ❌      |
| 57 | `userprog/multi-child-fd`            |      ❌      |   ❌    |      ❌      |
| 58 | `userprog/rox-simple`                |      ❌      |   ❌    |      ❌      |
| 59 | `userprog/rox-child`                 |      ❌      |   ❌    |      ❌      |
| 60 | `userprog/rox-multichild`            |      ❌      |   ❌    |      ❌      |
| 61 | `userprog/bad-read`                  |      ❌      |   ❌    |      ❌      |
| 62 | `userprog/bad-write`                 |      ❌      |   ❌    |      ❌      |
| 63 | `userprog/bad-read2`                 |      ❌      |   ❌    |      ❌      |
| 64 | `userprog/bad-write2`                |      ❌      |   ❌    |      ❌      |
| 65 | `userprog/bad-jump`                  |      ❌      |   ❌    |      ❌      |
| 66 | `userprog/bad-jump2`                 |      ❌      |   ❌    |      ❌      |
| 67 | `userprog/no-vm/multi-oom`           |      ❌      |   ❌    |      ❌      |
| 68 | `filesys/base/lg-create`             |      ❌      |   ❌    |      ❌      |
| 69 | `filesys/base/lg-full`               |      ❌      |   ❌    |      ❌      |
| 70 | `filesys/base/lg-random`             |      ❌      |   ❌    |      ❌      |
| 71 | `filesys/base/lg-seq-block`          |      ❌      |   ❌    |      ❌      |
| 72 | `filesys/base/lg-seq-random`         |      ❌      |   ❌    |      ❌      |
| 73 | `filesys/base/sm-create`             |      ❌      |   ❌    |      ❌      |
| 74 | `filesys/base/sm-full`               |      ❌      |   ❌    |      ❌      |
| 75 | `filesys/base/sm-random`             |      ❌      |   ❌    |      ❌      |
| 76 | `filesys/base/sm-seq-block`          |      ❌      |   ❌    |      ❌      |
| 77 | `filesys/base/sm-seq-random`         |      ❌      |   ❌    |      ❌      |
| 78 | `filesys/base/syn-read`              |      ❌      |   ❌    |      ❌      |
| 79 | `filesys/base/syn-remove`            |      ❌      |   ❌    |      ❌      |
| 80 | `filesys/base/syn-write`             |      ❌      |   ❌    |      ❌      |

  </details>
</details>
<details>
   <summary>Parte 3</summary>
<details>
TODO: Talvez mudar como apresentar as principais estruturas
   <summary>Objetivos Principais</summary>
   Toda essa parte do projeto basicamente vai lidar com diferentes tipos de paginação para diferentes objetivos, em resumo vai ser necessário implementar 4 estruturas de dados, cada uma com uma lógica e servindo para completar um dos objetivos dessa parte; Vai precisar modificar as pastas `vm/`,  `devices/` e `userprog/`
##### Paging
   A 1° estrutura é uma tabela de páginas suplementar que vai conter dados adicionais sobre cada página em uso ou não, envolvendo talvez modificar o `pagedir`(segundo a documentação modificar isso é só para quem é nível avançado) e melhorar o caso da ocorrência de um `page_fault()`
##### Stack Growth
Basicamente implementar uma tabela representando os frames atualmente carregados em memória, usando principalmente de funções como as encontradas em `palloc.c`
##### Memory Mapped Files
Vai criar uma tabela de mapeamento dos arquivos em disco, usando da lógica de páginas virtuais
##### Acessing User Memory
A última estrutura vai ser uma tabela de swap, que vai gerenciar os slots disponíveis e usados na memória e gerenciar como vai ocorrer a cópia dos dados para a partição de swap
 </details>

 <details>
    <summary>Tests</summary>

| # | Teste | Implementada | Testada | Funcionando |
|---|--------|:------------:|:-------:|:-----------:|
| 1 | `userprog/args-none` | ❌ | ❌ | ❌ |
| 2 | `userprog/args-single` | ❌ | ❌ | ❌ |
| 3 | `userprog/args-multiple` | ❌ | ❌ | ❌ |
| 4 | `userprog/args-many` | ❌ | ❌ | ❌ |
| 5 | `userprog/args-dbl-space` | ❌ | ❌ | ❌ |
| 6 | `userprog/sc-bad-sp` | ❌ | ❌ | ❌ |
| 7 | `userprog/sc-bad-arg` | ❌ | ❌ | ❌ |
| 8 | `userprog/sc-boundary` | ❌ | ❌ | ❌ |
| 9 | `userprog/sc-boundary-2` | ❌ | ❌ | ❌ |
| 10 | `userprog/sc-boundary-3` | ❌ | ❌ | ❌ |
| 11 | `userprog/halt` | ❌ | ❌ | ❌ |
| 12 | `userprog/exit` | ❌ | ❌ | ❌ |
| 13 | `userprog/create-normal` | ❌ | ❌ | ❌ |
| 14 | `userprog/create-empty` | ❌ | ❌ | ❌ |
| 15 | `userprog/create-null` | ❌ | ❌ | ❌ |
| 16 | `userprog/create-bad-ptr` | ❌ | ❌ | ❌ |
| 17 | `userprog/create-long` | ❌ | ❌ | ❌ |
| 18 | `userprog/create-exists` | ❌ | ❌ | ❌ |
| 19 | `userprog/create-bound` | ❌ | ❌ | ❌ |
| 20 | `userprog/open-normal` | ❌ | ❌ | ❌ |
| 21 | `userprog/open-missing` | ❌ | ❌ | ❌ |
| 22 | `userprog/open-boundary` | ❌ | ❌ | ❌ |
| 23 | `userprog/open-empty` | ❌ | ❌ | ❌ |
| 24 | `userprog/open-null` | ❌ | ❌ | ❌ |
| 25 | `userprog/open-bad-ptr` | ❌ | ❌ | ❌ |
| 26 | `userprog/open-twice` | ❌ | ❌ | ❌ |
| 27 | `userprog/close-normal` | ❌ | ❌ | ❌ |
| 28 | `userprog/close-twice` | ❌ | ❌ | ❌ |
| 29 | `userprog/close-stdin` | ❌ | ❌ | ❌ |
| 30 | `userprog/close-stdout` | ❌ | ❌ | ❌ |
| 31 | `userprog/close-bad-fd` | ❌ | ❌ | ❌ |
| 32 | `userprog/read-normal` | ❌ | ❌ | ❌ |
| 33 | `userprog/read-bad-ptr` | ❌ | ❌ | ❌ |
| 34 | `userprog/read-boundary` | ❌ | ❌ | ❌ |
| 35 | `userprog/read-zero` | ❌ | ❌ | ❌ |
| 36 | `userprog/read-stdout` | ❌ | ❌ | ❌ |
| 37 | `userprog/read-bad-fd` | ❌ | ❌ | ❌ |
| 38 | `userprog/write-normal` | ❌ | ❌ | ❌ |
| 39 | `userprog/write-bad-ptr` | ❌ | ❌ | ❌ |
| 40 | `userprog/write-boundary` | ❌ | ❌ | ❌ |
| 41 | `userprog/write-zero` | ❌ | ❌ | ❌ |
| 42 | `userprog/write-stdin` | ❌ | ❌ | ❌ |
| 43 | `userprog/write-bad-fd` | ❌ | ❌ | ❌ |
| 44 | `userprog/exec-once` | ❌ | ❌ | ❌ |
| 45 | `userprog/exec-arg` | ❌ | ❌ | ❌ |
| 46 | `userprog/exec-bound` | ❌ | ❌ | ❌ |
| 47 | `userprog/exec-bound-2` | ❌ | ❌ | ❌ |
| 48 | `userprog/exec-bound-3` | ❌ | ❌ | ❌ |
| 49 | `userprog/exec-multiple` | ❌ | ❌ | ❌ |
| 50 | `userprog/exec-missing` | ❌ | ❌ | ❌ |
| 51 | `userprog/exec-bad-ptr` | ❌ | ❌ | ❌ |
| 52 | `userprog/wait-simple` | ❌ | ❌ | ❌ |
| 53 | `userprog/wait-twice` | ❌ | ❌ | ❌ |
| 54 | `userprog/wait-killed` | ❌ | ❌ | ❌ |
| 55 | `userprog/wait-bad-pid` | ❌ | ❌ | ❌ |
| 56 | `userprog/multi-recurse` | ❌ | ❌ | ❌ |
| 57 | `userprog/multi-child-fd` | ❌ | ❌ | ❌ |
| 58 | `userprog/rox-simple` | ❌ | ❌ | ❌ |
| 59 | `userprog/rox-child` | ❌ | ❌ | ❌ |
| 60 | `userprog/rox-multichild` | ❌ | ❌ | ❌ |
| 61 | `userprog/bad-read` | ❌ | ❌ | ❌ |
| 62 | `userprog/bad-write` | ❌ | ❌ | ❌ |
| 63 | `userprog/bad-read2` | ❌ | ❌ | ❌ |
| 64 | `userprog/bad-write2` | ❌ | ❌ | ❌ |
| 65 | `userprog/bad-jump` | ❌ | ❌ | ❌ |
| 66 | `userprog/bad-jump2` | ❌ | ❌ | ❌ |
| 67 | `vm/pt-grow-stack` | ❌ | ❌ | ❌ |
| 68 | `vm/pt-grow-pusha` | ❌ | ❌ | ❌ |
| 69 | `vm/pt-grow-bad` | ❌ | ❌ | ❌ |
| 70 | `vm/pt-big-stk-obj` | ❌ | ❌ | ❌ |
| 71 | `vm/pt-bad-addr` | ❌ | ❌ | ❌ |
| 72 | `vm/pt-bad-read` | ❌ | ❌ | ❌ |
| 73 | `vm/pt-write-code` | ❌ | ❌ | ❌ |
| 74 | `vm/pt-write-code2` | ❌ | ❌ | ❌ |
| 75 | `vm/pt-grow-stk-sc` | ❌ | ❌ | ❌ |
| 76 | `vm/page-linear` | ❌ | ❌ | ❌ |
| 77 | `vm/page-parallel` | ❌ | ❌ | ❌ |
| 78 | `vm/page-merge-seq` | ❌ | ❌ | ❌ |
| 79 | `vm/page-merge-par` | ❌ | ❌ | ❌ |
| 80 | `vm/page-merge-stk` | ❌ | ❌ | ❌ |
| 81 | `vm/page-merge-mm` | ❌ | ❌ | ❌ |
| 82 | `vm/page-shuffle` | ❌ | ❌ | ❌ |
| 83 | `vm/mmap-read` | ❌ | ❌ | ❌ |
| 84 | `vm/mmap-close` | ❌ | ❌ | ❌ |
| 85 | `vm/mmap-unmap` | ❌ | ❌ | ❌ |
| 86 | `vm/mmap-overlap` | ❌ | ❌ | ❌ |
| 87 | `vm/mmap-twice` | ❌ | ❌ | ❌ |
| 88 | `vm/mmap-write` | ❌ | ❌ | ❌ |
| 89 | `vm/mmap-exit` | ❌ | ❌ | ❌ |
| 90 | `vm/mmap-shuffle` | ❌ | ❌ | ❌ |
| 91 | `vm/mmap-bad-fd` | ❌ | ❌ | ❌ |
| 92 | `vm/mmap-clean` | ❌ | ❌ | ❌ |
| 93 | `vm/mmap-inherit` | ❌ | ❌ | ❌ |
| 94 | `vm/mmap-misalign` | ❌ | ❌ | ❌ |
| 95 | `vm/mmap-null` | ❌ | ❌ | ❌ |
| 96 | `vm/mmap-over-code` | ❌ | ❌ | ❌ |
| 97 | `vm/mmap-over-data` | ❌ | ❌ | ❌ |
| 98 | `vm/mmap-over-stk` | ❌ | ❌ | ❌ |
| 99 | `vm/mmap-remove` | ❌ | ❌ | ❌ |
| 100 | `vm/mmap-zero` | ❌ | ❌ | ❌ |
| 101 | `filesys/base/lg-create` | ❌ | ❌ | ❌ |
| 102 | `filesys/base/lg-full` | ❌ | ❌ | ❌ |
| 103 | `filesys/base/lg-random` | ❌ | ❌ | ❌ |
| 104 | `filesys/base/lg-seq-block` | ❌ | ❌ | ❌ |
| 105 | `filesys/base/lg-seq-random` | ❌ | ❌ | ❌ |
| 106 | `filesys/base/sm-create` | ❌ | ❌ | ❌ |
| 107 | `filesys/base/sm-full` | ❌ | ❌ | ❌ |
| 108 | `filesys/base/sm-random` | ❌ | ❌ | ❌ |
| 109 | `filesys/base/sm-seq-block` | ❌ | ❌ | ❌ |
| 110 | `filesys/base/sm-seq-random` | ❌ | ❌ | ❌ |
| 111 | `filesys/base/syn-read` | ❌ | ❌ | ❌ |
| 112 | `filesys/base/syn-remove` | ❌ | ❌ | ❌ |
| 113 | `filesys/base/syn-write` | ❌ | ❌ | ❌ |

  </details>
</details>
<details>
   <summary>Parte 4</summary>
<details>
   <summary>Objetivos Principais</summary>
 </details>

<details>
   <summary>Buffer Cache</summary>
   Implementar um buffer para utilizar de cache dos blocos de arquivos, sugerindo pela documentação de no máximo 64 setores; O algoritmo para gerenciar o cache deve ter desempenho parecido com o algoritmo de clock wise e ser write back/behind, além de que a inserção de blocos subsequentes deve ser feito de forma assíncrona(background).
 </details>

<details>
   <summary>Extensible Files</summary>
   Criação de estruturas de indexação direta, indireta e/ou duplamente indireta para gerenciar a fragmentação dos blocos de arquivos, distribuindo os blocos nos espaços disponíveis; A partição de arquivos será no máximo de 8Mb nos testes, mas deve suportar arquivos maiores que esse limite, permitindo também o crescimento dos arquivos.
 </details>
 <details>
   <summary>Subdirectories</summary>
 Modificação das syscall's para habilitar os padrões de path usados no UNIX(no caso '/', '.', '..' ..,) criando junto disso a lógica de um namespace hierarquico e a separação do sistema entre processos; Permitir também a criação de arquivos com nomes maiores.
 </details>
 <details>
   <summary>Synchronization</summary>
   Lógica de leitura e escrita síncrona no sistema de arquivos, sendo que apenas 1 threads pode modificar o sistema por vez, operações em caches difrentes devem ser independetes e múltiplas leituras devem ser feitas em paralelo.
 </details>

 <details>
    <summary>Tests</summary>

| # | Teste | Implementada | Testada | Funcionando |
|---|-----------|:-----------:|:-------:|:-----------:|
| 1 | `userprog/args-none` | ❌ | ❌ | ❌ |
| 2 | `userprog/args-single` | ❌ | ❌ | ❌ |
| 3 | `userprog/args-multiple` | ❌ | ❌ | ❌ |
| 4 | `userprog/args-many` | ❌ | ❌ | ❌ |
| 5 | `userprog/args-dbl-space` | ❌ | ❌ | ❌ |
| 6 | `userprog/sc-bad-sp` | ❌ | ❌ | ❌ |
| 7 | `userprog/sc-bad-arg` | ❌ | ❌ | ❌ |
| 8 | `userprog/sc-boundary` | ❌ | ❌ | ❌ |
| 9 | `userprog/sc-boundary-2` | ❌ | ❌ | ❌ |
| 10 | `userprog/sc-boundary-3` | ❌ | ❌ | ❌ |
| 11 | `userprog/halt` | ❌ | ❌ | ❌ |
| 12 | `userprog/exit` | ❌ | ❌ | ❌ |
| 13 | `userprog/create-normal` | ❌ | ❌ | ❌ |
| 14 | `userprog/create-empty` | ❌ | ❌ | ❌ |
| 15 | `userprog/create-null` | ❌ | ❌ | ❌ |
| 16 | `userprog/create-bad-ptr` | ❌ | ❌ | ❌ |
| 17 | `userprog/create-long` | ❌ | ❌ | ❌ |
| 18 | `userprog/create-exists` | ❌ | ❌ | ❌ |
| 19 | `userprog/create-bound` | ❌ | ❌ | ❌ |
| 20 | `userprog/open-normal` | ❌ | ❌ | ❌ |
| 21 | `userprog/open-missing` | ❌ | ❌ | ❌ |
| 22 | `userprog/open-boundary` | ❌ | ❌ | ❌ |
| 23 | `userprog/open-empty` | ❌ | ❌ | ❌ |
| 24 | `userprog/open-null` | ❌ | ❌ | ❌ |
| 25 | `userprog/open-bad-ptr` | ❌ | ❌ | ❌ |
| 26 | `userprog/open-twice` | ❌ | ❌ | ❌ |
| 27 | `userprog/close-normal` | ❌ | ❌ | ❌ |
| 28 | `userprog/close-twice` | ❌ | ❌ | ❌ |
| 29 | `userprog/close-stdin` | ❌ | ❌ | ❌ |
| 30 | `userprog/close-stdout` | ❌ | ❌ | ❌ |
| 31 | `userprog/close-bad-fd` | ❌ | ❌ | ❌ |
| 32 | `userprog/read-normal` | ❌ | ❌ | ❌ |
| 33 | `userprog/read-bad-ptr` | ❌ | ❌ | ❌ |
| 34 | `userprog/read-boundary` | ❌ | ❌ | ❌ |
| 35 | `userprog/read-zero` | ❌ | ❌ | ❌ |
| 36 | `userprog/read-stdout` | ❌ | ❌ | ❌ |
| 37 | `userprog/read-bad-fd` | ❌ | ❌ | ❌ |
| 38 | `userprog/write-normal` | ❌ | ❌ | ❌ |
| 39 | `userprog/write-bad-ptr` | ❌ | ❌ | ❌ |
| 40 | `userprog/write-boundary` | ❌ | ❌ | ❌ |
| 41 | `userprog/write-zero` | ❌ | ❌ | ❌ |
| 42 | `userprog/write-stdin` | ❌ | ❌ | ❌ |
| 43 | `userprog/write-bad-fd` | ❌ | ❌ | ❌ |
| 44 | `userprog/exec-once` | ❌ | ❌ | ❌ |
| 45 | `userprog/exec-arg` | ❌ | ❌ | ❌ |
| 46 | `userprog/exec-bound` | ❌ | ❌ | ❌ |
| 47 | `userprog/exec-bound-2` | ❌ | ❌ | ❌ |
| 48 | `userprog/exec-bound-3` | ❌ | ❌ | ❌ |
| 49 | `userprog/exec-multiple` | ❌ | ❌ | ❌ |
| 50 | `userprog/exec-missing` | ❌ | ❌ | ❌ |
| 51 | `userprog/exec-bad-ptr` | ❌ | ❌ | ❌ |
| 52 | `userprog/wait-simple` | ❌ | ❌ | ❌ |
| 53 | `userprog/wait-twice` | ❌ | ❌ | ❌ |
| 54 | `userprog/wait-killed` | ❌ | ❌ | ❌ |
| 55 | `userprog/wait-bad-pid` | ❌ | ❌ | ❌ |
| 56 | `userprog/multi-recurse` | ❌ | ❌ | ❌ |
| 57 | `userprog/multi-child-fd` | ❌ | ❌ | ❌ |
| 58 | `userprog/rox-simple` | ❌ | ❌ | ❌ |
| 59 | `userprog/rox-child` | ❌ | ❌ | ❌ |
| 60 | `userprog/rox-multichild` | ❌ | ❌ | ❌ |
| 61 | `userprog/bad-read` | ❌ | ❌ | ❌ |
| 62 | `userprog/bad-write` | ❌ | ❌ | ❌ |
| 63 | `userprog/bad-read2` | ❌ | ❌ | ❌ |
| 64 | `userprog/bad-write2` | ❌ | ❌ | ❌ |
| 65 | `userprog/bad-jump` | ❌ | ❌ | ❌ |
| 66 | `userprog/bad-jump2` | ❌ | ❌ | ❌ |
| 67 | `filesys/base/lg-create` | ❌ | ❌ | ❌ |
| 68 | `filesys/base/lg-full` | ❌ | ❌ | ❌ |
| 69 | `filesys/base/lg-random` | ❌ | ❌ | ❌ |
| 70 | `filesys/base/lg-seq-block` | ❌ | ❌ | ❌ |
| 71 | `filesys/base/lg-seq-random` | ❌ | ❌ | ❌ |
| 72 | `filesys/base/sm-create` | ❌ | ❌ | ❌ |
| 73 | `filesys/base/sm-full` | ❌ | ❌ | ❌ |
| 74 | `filesys/base/sm-random` | ❌ | ❌ | ❌ |
| 75 | `filesys/base/sm-seq-block` | ❌ | ❌ | ❌ |
| 76 | `filesys/base/sm-seq-random` | ❌ | ❌ | ❌ |
| 77 | `filesys/base/syn-read` | ❌ | ❌ | ❌ |
| 78 | `filesys/base/syn-remove` | ❌ | ❌ | ❌ |
| 79 | `filesys/base/syn-write` | ❌ | ❌ | ❌ |
| 80 | `filesys/extended/dir-empty-name` | ❌ | ❌ | ❌ |
| 81 | `filesys/extended/dir-mk-tree` | ❌ | ❌ | ❌ |
| 82 | `filesys/extended/dir-mkdir` | ❌ | ❌ | ❌ |
| 83 | `filesys/extended/dir-open` | ❌ | ❌ | ❌ |
| 84 | `filesys/extended/dir-over-file` | ❌ | ❌ | ❌ |
| 85 | `filesys/extended/dir-rm-cwd` | ❌ | ❌ | ❌ |
| 86 | `filesys/extended/dir-rm-parent` | ❌ | ❌ | ❌ |
| 87 | `filesys/extended/dir-rm-root` | ❌ | ❌ | ❌ |
| 88 | `filesys/extended/dir-rm-tree` | ❌ | ❌ | ❌ |
| 89 | `filesys/extended/dir-rmdir` | ❌ | ❌ | ❌ |
| 90 | `filesys/extended/dir-under-file` | ❌ | ❌ | ❌ |
| 91 | `filesys/extended/dir-vine` | ❌ | ❌ | ❌ |
| 92 | `filesys/extended/grow-create` | ❌ | ❌ | ❌ |
| 93 | `filesys/extended/grow-dir-lg` | ❌ | ❌ | ❌ |
| 94 | `filesys/extended/grow-file-size` | ❌ | ❌ | ❌ |
| 95 | `filesys/extended/grow-root-lg` | ❌ | ❌ | ❌ |
| 96 | `filesys/extended/grow-root-sm` | ❌ | ❌ | ❌ |
| 97 | `filesys/extended/grow-seq-lg` | ❌ | ❌ | ❌ |
| 98 | `filesys/extended/grow-seq-sm` | ❌ | ❌ | ❌ |
| 99 | `filesys/extended/grow-sparse` | ❌ | ❌ | ❌ |
| 100 | `filesys/extended/grow-tell` | ❌ | ❌ | ❌ |
| 101 | `filesys/extended/grow-two-files` | ❌ | ❌ | ❌ |
| 102 | `filesys/extended/syn-rw` | ❌ | ❌ | ❌ |
| 103 | `filesys/extended/dir-empty-name-persistence` | ❌ | ❌ | ❌ |
| 104 | `filesys/extended/dir-mk-tree-persistence` | ❌ | ❌ | ❌ |
| 105 | `filesys/extended/dir-mkdir-persistence` | ❌ | ❌ | ❌ |
| 106 | `filesys/extended/dir-open-persistence` | ❌ | ❌ | ❌ |
| 107 | `filesys/extended/dir-over-file-persistence` | ❌ | ❌ | ❌ |
| 108 | `filesys/extended/dir-rm-cwd-persistence` | ❌ | ❌ | ❌ |
| 109 | `filesys/extended/dir-rm-parent-persistence` | ❌ | ❌ | ❌ |
| 110 | `filesys/extended/dir-rm-root-persistence` | ❌ | ❌ | ❌ |
| 111 | `filesys/extended/dir-rm-tree-persistence` | ❌ | ❌ | ❌ |
| 112 | `filesys/extended/dir-rmdir-persistence` | ❌ | ❌ | ❌ |
| 113 | `filesys/extended/dir-under-file-persistence` | ❌ | ❌ | ❌ |
| 114 | `filesys/extended/dir-vine-persistence` | ❌ | ❌ | ❌ |
| 115 | `filesys/extended/grow-create-persistence` | ❌ | ❌ | ❌ |
| 116 | `filesys/extended/grow-dir-lg-persistence` | ❌ | ❌ | ❌ |
| 117 | `filesys/extended/grow-file-size-persistence` | ❌ | ❌ | ❌ |
| 118 | `filesys/extended/grow-root-lg-persistence` | ❌ | ❌ | ❌ |
| 119 | `filesys/extended/grow-root-sm-persistence` | ❌ | ❌ | ❌ |
| 120 | `filesys/extended/grow-seq-lg-persistence` | ❌ | ❌ | ❌ |
| 121 | `filesys/extended/grow-seq-sm-persistence` | ❌ | ❌ | ❌ |
| 122 | `filesys/extended/grow-sparse-persistence` | ❌ | ❌ | ❌ |
| 123 | `filesys/extended/grow-tell-persistence` | ❌ | ❌ | ❌ |
| 124 | `filesys/extended/grow-two-files-persistence` | ❌ | ❌ | ❌ |
| 125 | `filesys/extended/syn-rw-persistence` | ❌ | ❌ | ❌ |

  </details>
  </details>
