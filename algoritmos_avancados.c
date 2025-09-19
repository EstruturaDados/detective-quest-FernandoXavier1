#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Desafio Detective Quest
// Tema 4 - Árvores e Tabela Hash
// Estruturas: árvore binária (mapa), BST (pistas), hash (suspeitos).

/* ============================ UTIL ============================ */
static void flushline(void){ int c; while((c=getchar())!='\n' && c!=EOF){} }
static void lower_inplace(char *s){ for(;*s;++s)*s=(char)tolower((unsigned char)*s); }

/* ==================== 🌱 Nível Novato: Mapa ==================== */
// struct Sala: nome, esquerda, direita (+ pista/suspeito para níveis seguintes)
typedef struct Sala {
    char *nome;
    char *pista;          // texto da pista (opcional)
    char *suspeitoHint;   // suspeito associado à pista (opcional)
    struct Sala *esq, *dir;
} Sala;

Sala* criarSala(const char *nome, const char *pista, const char *suspeito){
    Sala *s = (Sala*)calloc(1,sizeof(Sala));
    if(!s){ perror("calloc"); exit(1); }
    s->nome = strdup(nome);
    s->pista = pista ? strdup(pista) : NULL;
    s->suspeitoHint = suspeito ? strdup(suspeito) : NULL;
    return s;
}
void conectarSalas(Sala *pai, Sala *esq, Sala *dir){
    if(pai){ pai->esq = esq; pai->dir = dir; }
}
void liberarMapa(Sala *r){
    if(!r) return;
    liberarMapa(r->esq); liberarMapa(r->dir);
    free(r->nome); if(r->pista) free(r->pista); if(r->suspeitoHint) free(r->suspeitoHint);
    free(r);
}

/* ================= 🔍 Nível Aventureiro: BST de Pistas ================= */
typedef struct PistaNode {
    char *texto;
    struct PistaNode *esq, *dir;
} PistaNode;

PistaNode* inserirBST(PistaNode *raiz, const char *texto){
    if(!raiz){
        PistaNode *n=(PistaNode*)calloc(1,sizeof(PistaNode));
        if(!n){ perror("calloc"); exit(1); }
        n->texto = strdup(texto);
        return n;
    }
    int cmp = strcmp(texto, raiz->texto);
    if(cmp < 0) raiz->esq = inserirBST(raiz->esq, texto);
    else if(cmp > 0) raiz->dir = inserirBST(raiz->dir, texto);
    return raiz; // ignora duplicata
}
void emOrdem(const PistaNode *r){
    if(!r) return;
    emOrdem(r->esq);
    printf(" - %s\n", r->texto);
    emOrdem(r->dir);
}
void liberarBST(PistaNode *r){
    if(!r) return;
    liberarBST(r->esq); liberarBST(r->dir);
    free(r->texto); free(r);
}
void inserirPista(PistaNode **bst, const char *texto){ *bst = inserirBST(*bst, texto); }
void listarPistas(const PistaNode *bst){
    puts("\nPistas coletadas (ordem alfabética):");
    if(!bst){ puts("(nenhuma)"); return; }
    emOrdem(bst);
}

/* ============== 🧠 Nível Mestre: Hash de Suspeitos ============== */
typedef struct PItem {
    char *texto;
    struct PItem *next;
} PItem;

typedef struct Suspeito {
    char *nome;
    int  cont;       // quantas pistas associadas
    PItem *pistas;   // lista encadeada de pistas
    struct Suspeito *next; // para colisões (encadeamento separado)
} Suspeito;

typedef struct {
    Suspeito **bucket;
    size_t cap;
} Hash;

unsigned long hash_soma_ascii(const char *s){
    unsigned long h=0; for(;*s;++s) h += (unsigned char)*s;
    return h;
}
Hash* inicializarHash(size_t cap){
    Hash *h=(Hash*)calloc(1,sizeof(Hash));
    if(!h){ perror("calloc"); exit(1); }
    h->cap = cap;
    h->bucket = (Suspeito**)calloc(cap,sizeof(Suspeito*));
    if(!h->bucket){ perror("calloc"); exit(1); }
    return h;
}
Suspeito* buscarSuspeito(Hash *h, const char *nome){
    unsigned long idx = hash_soma_ascii(nome) % h->cap;
    for(Suspeito *s=h->bucket[idx]; s; s=s->next)
        if(strcmp(s->nome, nome)==0) return s;
    return NULL;
}
void inserirHash(Hash *h, const char *pista, const char *suspeitoNome){
    if(!suspeitoNome) return; // pista sem suspeito
    unsigned long idx = hash_soma_ascii(suspeitoNome) % h->cap;
    Suspeito *s = buscarSuspeito(h, suspeitoNome);
    if(!s){
        s = (Suspeito*)calloc(1,sizeof(Suspeito));
        if(!s){ perror("calloc"); exit(1); }
        s->nome = strdup(suspeitoNome);
        s->cont = 0; s->pistas = NULL;
        s->next = h->bucket[idx];
        h->bucket[idx] = s;
    }
    // adiciona pista na lista (ignora duplicata simples)
    for(PItem *it=s->pistas; it; it=it->next) if(strcmp(it->texto, pista)==0) { s->cont++; return; }
    PItem *p=(PItem*)calloc(1,sizeof(PItem));
    if(!p){ perror("calloc"); exit(1); }
    p->texto = strdup(pista);
    p->next = s->pistas; s->pistas = p; s->cont++;
}
void listarAssociacoes(const Hash *h){
    puts("\nSuspeitos e suas pistas:");
    int vazio = 1;
    for(size_t i=0;i<h->cap;i++){
        for(Suspeito *s=h->bucket[i]; s; s=s->next){
            vazio = 0;
            printf("- %s (%d pista%s): ", s->nome, s->cont, s->cont==1?"":"s");
            for(PItem *p=s->pistas; p; p=p->next){
                printf("\"%s\"%s", p->texto, p->next?"; ":"");
            }
            puts("");
        }
    }
    if(vazio) puts("(nenhuma associação)");
}
const Suspeito* maisProvavel(const Hash *h){
    const Suspeito *best=NULL;
    for(size_t i=0;i<h->cap;i++)
        for(Suspeito *s=h->bucket[i]; s; s=s->next)
            if(!best || s->cont > best->cont) best = s;
    return best;
}
void liberarHash(Hash *h){
    if(!h) return;
    for(size_t i=0;i<h->cap;i++){
        Suspeito *s=h->bucket[i];
        while(s){
            Suspeito *nx=s->next;
            PItem *p=s->pistas; while(p){ PItem *pn=p->next; free(p->texto); free(p); p=pn; }
            free(s->nome); free(s);
            s=nx;
        }
    }
    free(h->bucket); free(h);
}

/* ===================== Construção do Mapa ====================== */
/*
    Mapa fixo (exemplo):
                 Hall
               /       \
         Biblioteca    Cozinha
          /     \      /     \
     Escritorio Quarto Sotao  Jardim

   Algumas salas possuem pista + suspeito associado.
*/
Sala* construirMapa(void){
    Sala *hall        = criarSala("Hall de Entrada", NULL, NULL);
    Sala *biblioteca  = criarSala("Biblioteca", "marcas de poeira no chão", "Sra. White");
    Sala *cozinha     = criarSala("Cozinha", "faca ausente do suporte", "Sr. Black");
    Sala *escritorio  = criarSala("Escritorio", "documento rasgado", "Sr. Black");
    Sala *quarto      = criarSala("Quarto", "relógio parado 02:15", "Sra. White");
    Sala *sotao       = criarSala("Sotao", "chave antiga enferrujada", "Sr. Black");
    Sala *jardim      = criarSala("Jardim", "luva de couro", "Sr. Black");

    conectarSalas(hall,       biblioteca, cozinha);
    conectarSalas(biblioteca, escritorio,  quarto);
    conectarSalas(cozinha,    sotao,       jardim);

    return hall;
}

/* ===================== Exploração das Salas ==================== */
// Exploração simples: e=esq, d=dir, p=listar pistas, s=sair
void explorarSalas(Sala *raiz, PistaNode **pistasBST, Hash *hSus){
    Sala *cur = raiz;
    puts("Exploracao: [e] esquerda, [d] direita, [p] pistas, [s] sair");
    for(;;){
        printf("\nVocê está em: %s\n", cur->nome);
        if(cur->pista){
            printf("Há uma pista aqui: \"%s\"\n", cur->pista);
            // Coleta automática: adiciona na BST e relaciona na hash
            *pistasBST = inserirBST(*pistasBST, cur->pista);
            inserirHash(hSus, cur->pista, cur->suspeitoHint);
        }else{
            puts("Sem pista neste cômodo.");
        }

        if(!cur->esq && !cur->dir){
            puts("Sala sem saídas. Fim da exploração.");
            break;
        }

        printf("Caminhos: ");
        if(cur->esq) printf("[e] ");
        if(cur->dir) printf("[d] ");
        printf("| Extras: [p]=pistas, [s]=sair\n> ");

        int c=getchar(); if(c=='\n') c=getchar(); flushline();
        c = tolower(c);
        if(c=='s'){ puts("Exploração encerrada."); break; }
        else if(c=='p'){ listarPistas(*pistasBST); continue; }
        else if(c=='e'){
            if(cur->esq) cur=cur->esq; else puts("Não há saída à ESQUERDA.");
        } else if(c=='d'){
            if(cur->dir) cur=cur->dir; else puts("Não há saída à DIREITA.");
        } else {
            puts("Opção inválida.");
        }
    }
}

/* ============================== MAIN =========================== */
int main() {

    // 🌱 Nível Novato: Mapa da Mansão com Árvore Binária
    Sala *mapa = construirMapa();

    // 🔍 Nível Aventureiro: Armazenamento de Pistas com Árvore de Busca
    PistaNode *pistasBST = NULL;

    // 🧠 Nível Mestre: Relacionamento de Pistas com Suspeitos via Hash
    Hash *sus = inicializarHash(53); // número primo para dispersão

    puts("=== Detective Quest — Tema 4 (Árvores e Hash) ===");
    explorarSalas(mapa, &pistasBST, sus);

    // Revisão de evidências (BST em ordem)
    listarPistas(pistasBST);

    // Mostrar suspeitos e pistas (hash)
    listarAssociacoes(sus);

    // Suspeito mais provável
    const Suspeito *best = maisProvavel(sus);
    if(best) printf("\nSuspeito mais provável: %s (%d pista%s)\n",
                    best->nome, best->cont, best->cont==1?"":"s");
    else     puts("\nSem suspeitos associados.");

    // Limpeza
    liberarBST(pistasBST);
    liberarHash(sus);
    liberarMapa(mapa);
    return 0;
}
