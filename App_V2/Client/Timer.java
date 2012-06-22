import java.net.* ;
import java.io.* ;
import javax.swing.*;
public class Timer extends Thread
{
	private int i;
	private JFrame F;
	
	public Timer(JFrame E)
	{	
		F=E;
		i=0;		
		start();
	}	
	public void Initialise()
	{
		i=0;
	}	
		
	public void run()
	{
		while(true)
		{
			try 
			{
				sleep(1000);
			}
			catch(Exception e)
			{
				System.out.println("Erreur Sleep... \n"+e);
			}
			if(i>60)
			{
				F.dispose();
				JOptionPane.showMessageDialog(null,"il Faut se Reconnecter au serveur","Erreur pseudo",JOptionPane.WARNING_MESSAGE);
				new fenInfo();
	
				stop();
			}
		}
	}	
	
}
