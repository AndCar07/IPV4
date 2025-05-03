#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <math.h>
#include <string.h>
#include <pty.h>
#include <errno.h>

int cnt;//numero di figli

struct hip{
    int mit[4];
    int des[4];
    char mes[200];
};

struct proc{//per gli host
	char nome[20];
    int ip[4];
    int sub[4];
    int gat[4];
    int fd[2];
};

struct routing{//per il routing
    int ip[4];
    int sub[4];
    int inv;//indica la porta su cui devo inviare il messaggio
};

void son(struct proc all[], int pid, int fd);//pid per conoscere quale e' il suo ip

int porta(int ip[], struct proc all[]);//se trova corrisponedenza restituisce il valore della porta sennÃ² cnt

void router(struct proc all[], int fd);//scrive su tutte escluso fd dove legge

void help_me(struct proc all[], struct hip pac, struct routing entry[],int max, char port0[2][20], char port1[2][20]);//trovo la porta del router su cui inviare

int main(){
    int pipefd[2];
    pipe(pipefd);
    
    //inizializzazione della porta ethernet del router
    
    int gat[4];
    int sub[4];
    printf("inserisci IP di Gateway: ");
    scanf("%d.%d.%d.%d", &gat[0], &gat[1], &gat[2], &gat[3]);
    printf("Inserisci Subnetmask: ");
    scanf("%d.%d.%d.%d", &sub[0], &sub[1], &sub[2], &sub[3]);
    
    //inizializzazione dei figli
    printf("Quanti host: ");
    scanf("%d", &cnt);
    
    struct proc all[cnt];
    
    for(int i=0;i<cnt;i++){
        printf("inserisci IP: ");
        scanf("%d.%d.%d.%d", &all[i].ip[0], &all[i].ip[1], &all[i].ip[2], &all[i].ip[3]);
        printf("inserisci nome Host (di addio alla tua rete se inserisci un nome già usato): ");
        scanf("%s", all[i].nome);
        memcpy(all[i].sub, sub, sizeof(sub));//scambio puntatori
        memcpy(all[i].gat, gat, sizeof(gat));
        pipe(all[i].fd);
    }
    
    for(int i=0;i<cnt;i++){
        int pid=fork();
        if(pid==0){
            son(all, i, pipefd[1]);
            exit(EXIT_SUCCESS);
        }
    }
    router(all, pipefd[0]);
    return 0;
}

void son(struct proc all[], int pid, int fd){//scrive su tutte le pipe esclusa la propria da cui legge
    int interazione=open(all[pid].nome, 0 | O_CREAT, 0644);
    char n='3';
    while(1){
        int interazione=open(all[pid].nome, 0 | O_CREAT, 0644);
        char n='3';
        read(interazione, &n, sizeof(char));
        struct hip pac;
        int ip[4];
        if(n=='1'){
                printf("Inserisci IP di destinazione: ");
                scanf("%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
                printf("Inserisci testo messaggio:\n");
                getchar();  // Consuma il newline rimasto da scanf precedente
                fgets(pac.mes, sizeof(pac.mes), stdin);
                memcpy(pac.des, ip, sizeof(ip));
                memcpy(pac.mit, all[pid].ip, sizeof(all[pid].ip));
                int por=porta(ip, all);
                if(por==cnt){
                    write(fd, &pac, sizeof(struct hip));
                }
                else{
                    write(all[por].fd[1], &pac, sizeof(struct hip));
                }
          }
          else if(n=='2'){
                fcntl(all[pid].fd[0], F_SETFL, O_NONBLOCK);
                int result = read(all[pid].fd[0], &pac, sizeof(struct hip));
                if (result > 0) {
                    printf("Il messaggio e': %s", pac.mes);
                    scanf("%s", &n);//per poter modificare il file
                } else if (result == -1 && errno == EAGAIN) {
                    printf("Nessun messaggio disponibile\n");
                    scanf("%s", &n);//per poter modificare il file
                }
        }
        close(interazione);
    }
}

int porta(int ip[], struct proc all[]){//cerca se l'ip di destinazione nella rete
    bool vero=true;//avrei potuto usare anche and, ma non mi dava la porta precisa
    for(int i=0;i<cnt;i++){//se presente bene sennÃ² al router
	vero=true;
        for(int j=0;j<4;j++){//escluso quando l'ip non esiste il messaggio arriva diretto a destinazione
            if(ip[j]!=all[i].ip[j]){
                vero=false;
                break;
            }
        }
        if(vero){
            return i;
        }
    }
    return cnt;
}

void router(struct proc all[], int fd){//quello che gli passo solo per i figli, la prima per scrivere la seconda per leggere
    //chiedo i file a cui sono collegati le porte che sono due. Anche ip e sub anche se non si usano
    int ip[4][2];
    int sub[4][2];
    char port0[2][20]={"m", "n"};//per la prima porta
    char port1[2][20]={"m", "n"};//per la seconda porta
    
    int scelta;
    for(int i=0;i<2;i++){
        printf("Vuoi usare la porta n° %d (0/1)? :", i);
        scanf("%d", &scelta);
        if(scelta==1){
            printf("Inserisci IP: ");
            scanf("%d.%d.%d.%d", &ip[0][i], &ip[1][i], &ip[2][i], &ip[3][i]);//memorizzati verticalmente
            printf("Inserisci Subnetmask: ");
            scanf("%d.%d.%d.%d", &sub[0][i], &sub[1][i], &sub[2][i], &sub[3][i]);
            if(i==0){
                printf("Quale e' la pipe per scrivere da questa porta?");
                scanf("%s", port0[1]);
                printf("Quale e' la pipe per leggere da questa porta?");
                scanf("%s", port0[0]);
            }
            else{
                printf("Quale e' la pipe per scrivere da questa porta?");
                scanf("%s", port1[1]);
                printf("Quale e' la pipe per leggere da questa porta?");
                scanf("%s", port1[0]);
            }
        }
    }
    //chiedo il routing
    int max;
    
    printf("Quanti routing vuoi fare? : ");
    scanf("%d", &max);
    struct routing entry[max];
    
    for(int i=0;i<max;i++){
        printf("Inserisci rete IP: ");
        scanf("%d.%d.%d.%d", &entry[i].ip[0], &entry[i].ip[1], &entry[i].ip[2], &entry[i].ip[3]);
        printf("Inserisci Subnetmask: ");
        scanf("%d.%d.%d.%d", &entry[i].sub[0], &entry[i].sub[1], &entry[i].sub[2], &entry[i].sub[3]);
        printf("Inserisci la porta in cui e' presente il next hope");
        scanf("%d", &entry[i].inv);
    }
    //leggo se arriva qualcosa, sono presenti solo tre porte
    struct hip pac;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    int f=open(port0[0], O_RDONLY | O_NONBLOCK);
    int d=open(port1[0], O_RDONLY | O_NONBLOCK);
    while(1){
        int result = read(fd, &pac, sizeof(struct hip));
        if (result > 0) {
            printf("Il messaggio viene da %d.%d.%d.%d ed e' diretto a %d.%d.%d.%d\n", pac.mit[0], pac.mit[1],pac.mit[2],pac.mit[3],pac.des[0],pac.des[1],pac.des[2],pac.des[3]);
            help_me(all, pac, entry, max, port0, port1);
        }
        result= read(f,&pac, sizeof(struct hip));
        if (result > 0) {
            printf("Il messaggio viene da %d.%d.%d.%d ed e' diretto a %d.%d.%d.%d\n", pac.mit[0], pac.mit[1],pac.mit[2],pac.mit[3],pac.des[0],pac.des[1],pac.des[2],pac.des[3]);
            help_me(all, pac, entry, max, port0, port1);
        }
        result= read(d,&pac, sizeof(struct hip));
        if (result > 0) {
            printf("Il messaggio viene da %d.%d.%d.%d ed e' diretto a %d.%d.%d.%d\n", pac.mit[0], pac.mit[1],pac.mit[2],pac.mit[3],pac.des[0],pac.des[1],pac.des[2],pac.des[3]);
            help_me(all, pac, entry, max, port0, port1);
        }
    }
    //scorro in all se trovo invio
    //senno' scorro la routing, qui comparo la rete con l'and, se trovato invio sulla porta precisa
}

void help_me(struct proc all[], struct hip pac, struct routing entry[],int max, char port0[2][20], char port1[2][20]){
    for(int i=0;i<cnt;i++){
        if(all[i].ip[0]==pac.des[0]&&all[i].ip[1]==pac.des[1]&&all[i].ip[2]==pac.des[2]&&all[i].ip[3]==pac.des[3]){
            write(all[i].fd[1], &pac, sizeof(struct hip));
            break;
        }
    }
    int tmp[4];
    for(int i=0;i<max;i++){
        for(int j=0;j<4;j++){
            tmp[j]=entry[i].sub[j]&pac.des[j];
        }
        if(tmp[0]==entry[i].ip[0]&&tmp[1]==entry[i].ip[1]&&tmp[2]==entry[i].ip[2]&&tmp[3]==entry[i].ip[3]){
            if(entry[i].inv==0){
                int p=open(port0[1], 1);
                write(p,&pac, sizeof(struct hip));
                close(p);
            }
            else{
                int p=open(port1[1], 1);
                write(p,&pac, sizeof(struct hip));
                close(p);
            }
            break;
        }
    }
}
