import java.net.* ;
import java.io.* ;
import javax.swing.*;
import java.awt.event.*;
import java.awt.*;


public class reception extends Thread
{
	private int i=0;
	private DataInputStream entree;
	private TextArea chat;
	private String recu;
	private String nomClient;
	private JFrame F;
	private Timer T;
	public reception(JFrame E,DataInputStream DIS,TextArea texte,String nom,Timer t)
	{	F=E;
		T=t;
		entree=DIS;
		chat=texte;
		nomClient=nom;
		start();
	}
	
	
	public void run()
	{
		String texte;
		
	
			try
			{	
				
				while (true)	
				{	recu=entree.readLine();T.Initialise();
				   //Conditions pour regler des problemes dans l Affichage !!
					if(!recu.endsWith(": ") && !recu.equals(""))
					{//recu=recu.replace(":","");
					chat.setText(chat.getText()+" <-- " +recu+"\n");
					}
					if(recu.endsWith("Deconnection...")) 
					{
						/*si le Nom d utilisateur Existe dejat le Serveur envoi
						 *""\r\nPseudo deja utilise! Deconnection...""
						 *si le Cas en Ferme la Fenetre de Chat et en Affiche la Fenetre de connection
						 */
						F.dispose();
					JOptionPane.showMessageDialog(null,"saisir un autre pseudo","Erreur pseudo",JOptionPane.WARNING_MESSAGE);
						new fenInfo();
	
					}
						
                                   
				}
				
			}
		
			catch(IOException e) { System.out.println(e);System.exit(0);}
		
		
	}	
	
	
}
