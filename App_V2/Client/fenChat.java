import javax.swing.*;
import java.awt.event.*;
import java.awt.*;
import java.net.* ;
import java.io.* ;


public class fenChat extends JFrame implements ActionListener{

		
		//private JLabel label1=new JLabel("Le java est de retour");
		private JPanel panneau=new JPanel();
		private TextArea chat=new TextArea(20,40);
		private TextArea envoi=new TextArea(10,40);
		private JButton Envoyer=new JButton("Envoyer");
		//private expedition exped;
		private PrintStream sortie;
		private connexion connec;
		private String nom;
		private String serveur;
		private Timer T;
		
	

    	public fenChat(String n,int x,int y,int l,int L,String s) 
    	{
    		super(n);
    		setBounds(x,y,l,L);
        	setResizable(false);
        	setVisible(true);
        	setAlwaysOnTop(false);
		setLayout(new FlowLayout ());
        	add("Center",chat);
        	add("Center",envoi);
        	add(Envoyer);
        	chat.setEditable(false);
        	envoi.requestFocusInWindow();
        	nom=n;
        	serveur=s;
        	T=new Timer(this);
        	addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
       		 }});

        	
        	
        	Envoyer.addActionListener(this);
			
			
			connec=new connexion(this,chat,serveur,nom,T);
			sortie=connec.recupererSortie();
			
			
			
			
			
			
			
        	show();
        	
			
    	}
    	
    	
    	
    	public void actionPerformed(ActionEvent e)              
		{ 
			
			String label = e.getActionCommand();
			
			if (label.equals("Envoyer")) 
        		{
        			envoyer();
        		}

		}
		
		
		public void envoyer()
		{
			String texte=envoi.getText();
			T.Initialise();
        		sortie.println(texte);sortie.flush();
			if(texte.startsWith("/quit")) 
			{
				System.exit(0);
				T.stop();
			}
        		chat.setText(chat.getText()+" --> " +nom+"  : "+texte+"\n");
        		envoi.setText(null);
        		envoi.requestFocusInWindow();
			
		}
	
			
		

    
    
}
