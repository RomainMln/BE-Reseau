#include <mictcp.h>
#include <api/mictcp_core.h>

mic_tcp_sock sock_source;
mic_tcp_sock sock_puit;
mic_tcp_sock_addr addr_dest;

mic_tcp_pdu pdu;
short PE = 0;
short PA =0;
short msg_passe =0;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    int result = initialize_components(sm);
    set_loss_rate(1);
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
    sock_puit.state = ESTABLISHED;
    return(0);
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    sock_source.state = ESTABLISHED;
    addr_dest = addr;
    return(0);
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    int renvoie =0;
    mic_tcp_pdu newPDU;
    int valide = -1;
    int test;
    if(mic_sock != sock_source.fd){
        return(-1);
    }
    else{
        pdu.header.seq_num = PE;
        pdu.payload.data = mesg;
        pdu.payload.size = mesg_size;
        PE=(PE+1)%2;
        while(valide){
            if((renvoie==1) && (msg_passe>=48)){
                printf("skip\n");
                PE=(PE+1)%2;
                msg_passe=0;
                return(0);
            }
            IP_send(pdu,addr_dest);
            test = IP_recv(&newPDU,&addr_dest,100);
            if(test != -1)
            {
                if(newPDU.header.ack_num == PE){
                    valide = 0;
                }
            }
            renvoie=1;
        }
        msg_passe++;
        return(0);
    }
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size) //source
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    pdu.payload.data = mesg;
    pdu.payload.size = max_mesg_size;
    return(app_buffer_get(pdu.payload));
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
    mic_tcp_pdu ack;
    if(pdu.header.seq_num == PA){
        PA=(PA+1)%2;
        app_buffer_put(pdu.payload);
    }
    else{
        printf("Pas bon \n");
    }
    ack.header.ack_num = PA;
    IP_send(ack,addr);
}
