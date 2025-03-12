# UML Class Diagram

A UML class diagram shows the classes of a system, their attributes, operations, and the relationships among the classes.

![../assets/uml1.png](../assets/uml1.png)

## context
I have a main class webserv which will be instantiated once at the beginning of the program. This class takes a conf file and parses it, also has an internal representation of the conf data as json. Then inside this class i have an array of server classes each of them ,they bind to a specific port. On each of these classes I have a listen method to start the socket and listening for client connection.
Each of the servers got an id and can get their configuration details from  the json stored data>
so each server in the array has an array of handler. There is a handler for redirection if the client message maps to a certain route... or a handler for returning the resource asked for if it exists or pass a cgi request. so the route from the request will be matched to the one in the server config. if no route matching there is a handler for errors.

  

## links
https://www.umlboard.com/docs/relations/#general  
https://sparxsystems.com/resources/tutorials/uml2/class-diagram.html  