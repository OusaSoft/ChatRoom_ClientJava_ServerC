import java.net.* ;
import java.io.* ;
import javax.swing.*;
import java.awt.event.*;
import java.awt.*;


public class connexion {

    
    private Socket socket ;
    private DataInputStream entree;
    public PrintStream sortie;
    private reception rc;
    private String nomClient;
    private JFrame F;
    private Timer T;    
    public connexion(JFrame E,TextArea chat,String serveur,String nom, Timer t) 
    	
    {
    	
    	
    	
    	try 
    	{
        	socket = new Socket(serveur,3557) ;
        	entree=new DataInputStream(socket.getInputStream());
        	sortie=new PrintStream(socket.getOutputStream());
        	
        	sortie.println(nom);sortie.flush();
        	nomClient=entree.readLine();
        	
        	chat.setText("\nDebut de la discussion...\n");
        	
        	F=E;
		T=t;
        	rc=new reception(F,entree,chat,nomClient,T);
    	}
    	
    	catch ( IOException e) 
	{
		System.out.println(e); 
	}
    	
    	
    }
    
    public PrintStream recupererSortie()
    {
    	return sortie;
    	
    }
    
    
}
