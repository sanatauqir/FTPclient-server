/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package ftclient;

import java.io.*;
import java.net.*;

/**
 *
 * @author Sana Tauqir
 * @project Project 2 Client/Server TCP
 * References: Oracle Docs Java Socket Programming Tutorial
 */
public class Ftclient {

    private Socket echoSocket;
    private ServerSocket servSocket;
    private Socket dataSocket;
    PrintWriter out;
    BufferedReader in;
    PrintWriter dataOut;
    BufferedReader dataIn;
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) throws IOException {
       
        /*if (args.length != 5) {
            System.err.println(
                "Usage: java EchoClient <host name> <port number> <command> <filename> <dataport>");
            System.exit(1);
        }*/
 
        String hostName = args[0];
        int portNumber = Integer.parseInt(args[1]);
        String filename = ""; int datanum = 0;
        String command = args[2];
        if (command.equals("-l")){
            if (args.length != 4)
                System.err.println("Usage: java EchoClient <host name> "
                        + "<port number> <command> <dataport>");
            datanum = Integer.parseInt(args[3]);
        }else {
            if (args.length != 5)
                System.err.println("Usage: java EchoClient <host name> "
                        + "<port number> <command> <filename> <dataport>");
            filename = args[3];
            datanum = Integer.parseInt(args[4]);
        }
        
        Ftclient one = new Ftclient();
        one.startControl(hostName, portNumber);
        //System.out.println("have set up the control connection\n");
        
        one.sendCommand(command, filename, hostName, datanum);
        
        String servResponse = one.in.readLine();
        if (servResponse != null){
                System.out.println(servResponse);
            if (servResponse.equals("invalid command"))
                System.exit(0);
        else{
            //byte[] myarray = new byte[1024];
            String bytesRead; String[] retVal;
            one.startData(datanum);
         
            if (command.equals("-l")){
                while ((bytesRead = one.dataIn.readLine()) != null){
                   // bytesRead = one.dataIn.readLine();
                    retVal = bytesRead.split(",");
            
                    for (String temp: retVal){
                        System.out.println(temp);
                    }
                    //System.out.println(bytesRead);
                }
            }else {
                bytesRead = one.dataIn.readLine();
                
                File file = new File ("./" + "new.txt");
                if (file.createNewFile()){
                    FileWriter fw = new FileWriter("new.txt");
                    fw.write(bytesRead);
                    fw.close();
                }else {
                    System.out.println("file already exists.");
                }
                
                System.out.println("transfer complete");
            
            }
                    
            //String clientResponse = one.dataIn.readLine();
            //System.out.println(clientResponse);
            //one.dataOut.println("ready to get started exchaning data");
        }
    }
        one.servSocket.close();
        one.dataSocket.close();
        one.echoSocket.close();
        
        System.exit(0);
        
    }
    
    void startControl(String name, int number){
        try {
            echoSocket = new Socket(name, number);
            out = new PrintWriter(
                    echoSocket.getOutputStream(), true);
            in = new BufferedReader(
                    new InputStreamReader(echoSocket.getInputStream()));
            
        }
        catch (IOException e)
        {
            System.err.println("Couldn't get IO for connection to" + name);
            e.printStackTrace();
        }
        
    }
    
    void sendCommand(String command, String filename, String host, int datanum){
        String stringData = Integer.toString(datanum);
        if (command.equals("-l")){
            command = command.concat("#" + host + "#" + stringData +"#");
            //System.out.println("sending: " + command);
            out.println(command);
        }
        else {
            command = command.concat("#" + host + "#" + stringData + "#" + filename + "#");
            //System.out.println("sending: " + command);
            out.println(command);
        }
        System.out.println("finished sending command");
    }
    
    void startData(int datanum){
        try {
            servSocket = new ServerSocket(datanum);
            //servSocket.setSoTimeout(10000);
            //System.out.println("wait for a client");
            dataSocket = servSocket.accept();
            //System.out.println("ftclient.java has been accepted");
            dataOut = new PrintWriter(
                    dataSocket.getOutputStream(), true);
            dataIn = new BufferedReader(
                    new InputStreamReader(dataSocket.getInputStream()));
            
        }
        catch (IOException e)
        {
            System.err.println("Couldn't get IO for connection");
            e.printStackTrace();
        }
    }
}
