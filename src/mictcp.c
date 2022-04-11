#include <mictcp.h>
#include <api/mictcp_core.h>
#include <pthread.h>

mic_tcp_sock sock_source;
mic_tcp_sock sock_puit;
mic_tcp_sock_addr addr_dest;

short PE = 0;
short PA =0;
short msg_passe =0;
short msg_conseq = 4;

pthread_cond_t wait = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    int result = initialize_components(sm);
    set_loss_rate(0);
    if(sm == CLIENT)
    {
        sock_source.fd = result;
    }
    else
    {
        sock_puit.fd = result;
    }
    return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    int result;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    if(sock_puit.fd == socket)
    {
        sock_puit.state = IDLE;
        sock_puit.addr = addr;
        result = 0;
    }
    else{
        result = -1;
    }
    return result;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    sock_puit.state = LISTEN; 
    pthread_mutex_lock(&mutex); 
    while (sock_puit.state  != ESTABLISHED )
        pthread_cond_wait(&wait,&mutex);
    printf("[MIC-TCP]:  Connexion Etablie \n");
    pthread_mutex_unlock(&mutex); 
    return(0);
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu Syn;
    mic_tcp_pdu SynAckC; 
    SynAckC.payload.size = 0; 
    mic_tcp_pdu AckC;
    Syn.header.syn = 1;
    Syn.payload.size = 1;
    Syn.payload.data = "4";
    int bon = -1;
    int test;
    while(bon){
        IP_send(Syn,addr);
        sock_source.state = SYN_SENT; 
        test = IP_recv(&SynAckC,&addr,50);
        printf("test = %d \n", test); 
        if(test != -1){
            if(SynAckC.header.syn == 1 && SynAckC.header.ack == 1){
                printf("SYN | ACK reçu \n"); 
                bon = 0;
            }
        }
    }   
    AckC.header.ack = 1;
    AckC.header.syn = 0;
    AckC.header.fin = 0;
    AckC.header.seq_num = 0; 
    AckC.header.ack_num = 0; 
  
    AckC.payload.data = NULL;
    AckC.payload.size = 0;
    IP_send(AckC,addr);
    sock_source.state = ESTABLISHED;
    return(0);
}
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    mic_tcp_payload donnees;
    if(mic_sock != sock_source.fd){
        return(-1);
    }
    donnees.data = mesg;
    donnees.size = mesg_size;
    app_buffer_put(donnees);
    return(0);
}

int mic_tcp_sd (char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    int renvoie =0;
    mic_tcp_pdu newPDU;
    mic_tcp_pdu NPDU;
    int valide = -1;
    int test;
    NPDU.header.seq_num = PE;
    NPDU.header.ack = 0;
    NPDU.header.syn = 0;
    NPDU.header.ack_num = 0;
    NPDU.payload.data = mesg;
    NPDU.payload.size = mesg_size;
    PE=(PE+1)%2;
    while(valide){
        if((renvoie==1) && (msg_passe>=msg_conseq)){
            PE=(PE+1)%2;
            msg_passe=0;
            return(0);
        }
        IP_send(NPDU,addr_dest);
        test = IP_recv(&newPDU,&addr_dest,500);
        if(test != -1)
        {
            printf("wesh\n");
            if((newPDU.header.ack == 1) && (newPDU.header.syn == 1)){
                NPDU.header.ack = 1;
                IP_send(NPDU,addr_dest);
                printf("Bonjour\n");
            }
            else if(newPDU.header.ack_num == PE){
                valide = 0;
                printf("coucou\n");
            }
        }
        renvoie=1;
    }
    msg_passe++;
    return(0);
}
/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */


/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size) 
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu NPDU;
    NPDU.payload.data = mesg;
    NPDU.payload.size = max_mesg_size;
    return(app_buffer_get(NPDU.payload));
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr) //puit
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu SynAck;
    mic_tcp_pdu ack;
    int perte;
    float taux_min;
    if(pdu.header.syn == 1 && sock_puit.state == LISTEN){
        pthread_mutex_lock(&mutex);
        sock_puit.state = SYN_RECEIVED;
        pthread_mutex_unlock(&mutex);
        SynAck.header.syn = 1;
        SynAck.header.ack = 1;
        SynAck.payload.data = NULL;
        SynAck.payload.size = 0;
        printf("Envoi du SYN | ACK \n");
        IP_send(SynAck,addr);
        perte = atoi(pdu.payload.data);
        taux_min = (float)perte/(perte+1);
        printf("Le taux de message passé est : %f\n",taux_min*100.0);

    } else if (pdu.header.ack == 1 &&  sock_puit.state == SYN_RECEIVED) {
        pthread_mutex_lock(&mutex); 
        printf("ACK reçu, sock state passe à ESTABLISHED \n"); 
        sock_puit.state = ESTABLISHED; 
        pthread_cond_broadcast(&wait); 
        pthread_mutex_unlock(&mutex); 
    
    }
    else{
        if(pdu.header.seq_num == PA){
            PA=(PA+1)%2;
            app_buffer_put(pdu.payload);
        }
        ack.header.ack_num = PA;
        ack.header.ack = 0;
        ack.header.syn = 0;
        IP_send(ack,addr);
    }
}
//gharbi.ghada@gmail.com
