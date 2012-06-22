/* serveur de chat sous Linux avec les thread */
/* gcc server.c -o server -lpthread -D_REENTRANT */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

#define HELP_MSG "Commandes disponibles :\r\n- /quit=[message] --> quitter avec (ou sans) message\r\n- /list --> obtenir la liste des clients connectes\r\n- /? --> afficher l\'aide\r\n"

#define MAX_CLIENTS 500
#define LS_CLIENT_NB 5
#define INVALID_SOCKET -1
#define PORT 3557

volatile int nb_clients = 0;
int first_free = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//Liste des clients :: pour Faciliter l'Acces au pseudo , le descripteur de Socket et le Thread Associer a ce Client
typedef struct _s_client
{
	pthread_t id;   //Id du Thread Client
	int sock;   	//Descripteur du Socket associera ce Client
	char *pseudo;   //Le Pseudo du Client
} s_client;

s_client *clients[MAX_CLIENTS];

/* creation d'un serveur */
int create_server(int port)
{
  int sock,optval = 1;
  struct sockaddr_in sockname;

  if((sock = socket(PF_INET,SOCK_STREAM,0))<0)
    {
      printf("Erreur d'ouverture de la socket");
      exit(-1);
    }
  
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int));
  memset((char *) &sockname,0,sizeof(struct sockaddr_in));
  sockname.sin_family = AF_INET;
  sockname.sin_port = htons(port);
  sockname.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(sock,(struct sockaddr *) &sockname, sizeof(struct sockaddr_in)) < 0)
    {
      printf("Erreur de bind!");
      exit(-1);
    }

  if(listen(sock,LS_CLIENT_NB) <0)
    {
      printf("listen error!");
      exit(-1);
    }
  
  return sock;
}

/* accepte une connexion avec ou sans timeout */
int server_accept(int main_sock,int timeout)
{
  int sock;

  if(timeout > 0)
    alarm(timeout);

  if((sock = accept(main_sock,NULL,0)) < 0)
    {
      if(errno == EINTR)
	{
	  shutdown(main_sock,SHUT_RDWR);
	  close(main_sock);
	  if(timeout > 0)
	    alarm(0);
	  return -1;
	}
      else
	{
	  printf("\nAccept error.\n");
	  exit(-1);
	}
    }

  if(timeout > 0)
    alarm(0);
  fcntl(sock,F_SETFD,1);
  
  return sock;
}

/* envoyer une chaine de caractere a un client */
int send_msg(int sock,char *msg)
{
	return send(sock,msg,strlen(msg),0);
}

/* envoyer un message a tout le monde sauf a la socket not_to */
int send_all(char *msg, int not_to)
{
	int i;
	
	pthread_mutex_lock(&mutex);	// debut de la section critique
	for(i=0;i<first_free;i++)
	{
		if(clients[i]->sock != not_to)
			send_msg(clients[i]->sock,msg);
	}
	pthread_mutex_unlock(&mutex);	// fin de la section critique
	
	return 0;
}

/* gestion de fin de connection d'un client */
void client_quit(s_client *me, char *msg)
{
	/*dans cette Fonction on a gerer les 2 cas du sortie du client,,
	 *s'il quitte le programme son Laisser de message,
	 *(si le client ne veut pas Envoyer un Message un Message avant le <<Leave>> ou s'il est interrompu)
 	 *ou bien s'il Vent Envoyer un Dernier Message en Quittant l'application
	 *
	 */
	int i,j;
	char buf[8192+1];
	//On Concatene le Pseudo et le Message du Client qui quitte l'application  dans la variable Buf
	if(msg)	snprintf(buf,8192,"%s nous quitte...(%s)\r\n",me->pseudo,msg);
	else	snprintf(buf,8192,"%s nous quitte...\r\n",me->pseudo);
	buf[8192] = '\0';
	send_all(buf,me->sock);//l'Envoie du Message à tous les Client de notre Liste
	pthread_mutex_lock(&mutex);	// debut de la section critique
	// recherche de l'index de notre Client dans le tableau pour reorganiser le Tableau Apres la suppression 
	for(i=0;(clients[i]->sock != me->sock);i++);	
	close(me->sock);//la Fermeture du Socket
	//Liberer la Memoire des 2 pointeur
	free(me->pseudo);
	free(me);
	
	for(j=i+1;j<first_free;j++)	// on reorganise le tableau en decalant les clients situes apres celui qui a Quitter
	{
		clients[j-1] = clients[j];
	}
	nb_clients--;
	first_free--;
	pthread_mutex_unlock(&mutex);	// fin de la section critique
	printf("Un client a Quitter ... %d clients\n",nb_clients);
}

/* interaction avec le client (thread) */
void *interaction(void *param)
{
	int sck = *((int *) param);
	char msg[4096+1];
	char msg_to_send[8192+1];
	s_client *me = NULL;
	char *buf = NULL;
	int len;
	int i;
	//Allocation du Memoire pour 1 nouveau Client
	me = (s_client *) malloc(sizeof(s_client));
	//Le Cas D'erreur on Affiche le Msg d'erreur et en Ferme la Socket et le Thread et le Nbr du client diminue de 1	
	if(!me)
	{
		printf("\nErreur d'allocation memoire!\n");
		close(sck);
		nb_clients--;
		pthread_exit(NULL);
	}
	//Si nn on Initialise Les Champs de la Ctructure Client !!
	bzero(me,sizeof(s_client));
	
	//REception du 1er Message du Client,,,le Message contient le Login du Client
	len = recv(sck,msg,4096,0);
	//le Cas d'erreur ..
	if(len <= 0)
	{
		printf("\nErreur\n");
		close(sck);
		free(me);
		me = NULL;
		nb_clients--;
		pthread_exit(NULL);
	}
	msg[255] = '\0';	// on limite le pseudo a 255 caracteres


	
	for(i=0;(msg[i]!='\0') && (msg[i]!='\r') && (msg[i]!='\n') && (msg[i]!='\t');i++);
	msg[i] = '\0';	// on isole le pseudo
	
	pthread_mutex_lock(&mutex);	// debut de la section critique pour l'ajout d'un Seul client a la fois  
	for(i=0;i<first_free;i++)
	{//il faut comparer si le Pseudo Existe dejà Utiliser par un autre Client
		if(!strcmp(msg,clients[i]->pseudo))//si le Cas 
		{//On Envoie un Message qui "Pseudo deja utilise! !..." + fermeture de la connection 
			send_msg(sck,"\r\nPseudo deja utilise! Deconnection...");
			close(sck);
			free(me);
			nb_clients--;
			pthread_mutex_unlock(&mutex);	// fin de la section critique
			pthread_exit(NULL);
		}
	}
	pthread_mutex_unlock(&mutex);	// fin de la section critique
	//S'il n y a pas d'erreur on Affecte les Valeur du Client en Cours au Client "Me"
	me->id = pthread_self();
	me->sock = sck;
	me->pseudo = strdup(msg);
	
	pthread_mutex_lock(&mutex);	// debut de la section critique
	clients[first_free] = me;       //on ajoute "Me" a la 1ere case Vide de Notre Tableau de Clients
	first_free++;
	pthread_mutex_unlock(&mutex);	// fin de la section critique
	

	send_all(msg_to_send,INVALID_SOCKET);

       //Debut de la Conversastion du Client Avec Notre Serveur en Lui Envoyant le Message Help qui Contient les commandes Possible
       send_msg(me->sock,HELP_MSG);
	//Debut de la Comminication l'Envoi et la Reception des Messages et des Commandes     
	while(1)
	{	bzero(msg,sizeof(msg));
		len = recv(sck,msg,4096,0);//Reception d'un Message
		
		if(len <= 0)//Verifier s'il y a des Erreur
		{
			client_quit(me,"Erreur reseau");
			pthread_exit(NULL);
		}


		msg[len] = '\0';
		if(msg[0] == '/')	// si le message Commance par "/" ca ve dire qu'il s'agit d'une commande
		{
			int valid_command = 0;	//Variable pour contoler si une commande est Valide ou pas
			
			
			if(!strncmp(msg,"/quit",5))	// sortie "propre" du serveur (avec ou sans message)
			{
				int i;
				
				if(msg[5]=='=')//le Cas du sortie Avec Message
				{
					for(i=6;(msg[i]!='\0') && (msg[i]!='\r') && (msg[i]!='\n') && (msg[i]!='\t');i++);
					msg[i]='\0';
					client_quit(me,&msg[6]);//la Fonction Client_quit pour Quitter la Conversation et l'Envoye du Messsage
				}
				else client_quit(me,NULL);//si non ,on Quitte sans Message 
				valid_command = 1;	
				pthread_exit(NULL);     
				  
			}
			if(!strncmp(msg,"/list",5))	  // obtenir la liste des Clients Connecté sur le serveur
			{
				pthread_mutex_lock(&mutex);	// debut de la section critique
				/*on doit parcourir le Tableau des Clients,						 *
				 *et on Va Envoyer la Liste au Seulement au client "Me" (Celui qui a Demmander la Liste) *
				 *la Liste Sera sous la Forme " Client <indice du Client> : <Pseudo> "        *
				 */
				for(i=0;i<first_free;i++)
				{      char listesClient[100];
					snprintf(listesClient,100,"Client %d : %s ",(i+1),clients[i]->pseudo);
					send_msg(me->sock,listesClient);
					send_msg(me->sock,"\r\n");
				}
				pthread_mutex_unlock(&mutex);	// fin de la section critique
				valid_command = 1;
			}
			
			
			if(!strncmp(msg,"/?",2)) //pour Obtenir la liste des Commandes Possible
			{
				send_msg(me->sock,HELP_MSG);
				valid_command = 1;
			}
				
			if(!valid_command)	// commande invalide
				send_msg(sck,"Commande non valide!\r\n");
		}
		else			// message teste normal ,envoyer a tout les Client connecté
		{
			//le Message a Envoyer aura la forme "From <Pseudo> : Msssage" , la Forme est Citer dans le Cahier des Charges
			snprintf(msg_to_send,sizeof(me->pseudo)+sizeof(msg),"From < %s > : %s\n",me->pseudo,msg);
			msg_to_send[sizeof(me->pseudo)+sizeof(msg)] = '\0';
			send_all(msg_to_send,me->sock);//l'Envoie du Message a tous les client sauf "Me",celui qui l'a Envoyer !

		}
	}
	
	return NULL;
}

/* fonction principale ( main Function)*/
int main(int argc, char **argv)
{
	int server,sck;
	pthread_t th_id;
	//Message confirmant le Demmarage du serveur
	printf("Serveur de chat \n  .:Lansari:.\n\n");
	
	server = create_server(PORT);//Ouverture du Socket,Bind() et Listen() !!
	while(1)
	{
		sck = server_accept(server,0);//le Accept() !!
		if(sck == INVALID_SOCKET)// dans le Cas d'erruer en Evoi le Message d'erreur "Ereeur Accept()" et Fermeture du Serveur
		{
			printf("\nErreur de accept()!\n");
			exit(-1);
		}
		if(nb_clients < MAX_CLIENTS)// Creation du Thread Client,et le Demarrage de l'interaction entre ce Client et le Serveur
		{
			pthread_create(&th_id,NULL,interaction,(void *)&sck);
			nb_clients++;						//on increment le nombre de Client par 1
			//Message Afficher sur l'ecran du serveur Pour Indiquer qu'il s'agit d'un nouveau Client
			printf("Nouveau client! %d clients\n",nb_clients);	
		}
		else close(sck);
	}
	
	return 0;
}
