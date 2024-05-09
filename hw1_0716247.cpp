#include <cstdio>
#include <string.h> //strlen
#include <cstdlib>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include <map>
 
#define TRUE 1
#define FALSE 0
#define PORT 10004

#define USERISJOINED 1
#define NOTINCHANNEL 2
#define BANNEDFROMCHAN 3
#define TOOMANYCHANNELS 4
#define BADCHANNELKEY 5
#define CHANNELISFULL 6
#define NOSUCHCHANNEL 7
#define USERISBANNED 8
#define BADCHANMASK 9
#define USERNOTINCHANNEL -1
#define USERNOTFOUND -1

using namespace std;
// server info
int user_num=0;
char rcvmsg[1024];
char sentmsg[1024];

int client_socket[1024] ;
char other_client[1024];
int count=0,server=1;


typedef struct ui{
 int port;
 bool regnick;
 bool reguser;
 string realname;
 string nickname;
 string hostname;
 string servername;
 string username;
}user_info;


typedef struct chan{
   string channame;
   string topic;
   vector<string> chanuser;

}channel;

map<string, channel> allchannel;
map<int , string> _member;
vector < user_info> users;
vector <string > args;
map<int, string> username;
map<int, string> nickname;
map<int, string> ipaddr;
vector<channel> channellist;
map<int,vector<channel>>channels;

void listalluser(int id)
{
   strcpy(sentmsg,":mircd 392 ");
	strcat(sentmsg,(users[id].nickname).c_str());
	strcat(sentmsg," :UserID           Terminal  Host\r\n");
	for(int i=0;i<1024;i++)
	{
		if(users[i].regnick && users[i].reguser)
		{
			strcat(sentmsg,":mircd 393 ");
			strcat(sentmsg,nickname[id].c_str());
			strcat(sentmsg," :");
			strcat(sentmsg,nickname[i].c_str());
			strcat(sentmsg,"			-	");
			strcat(sentmsg,ipaddr[i].c_str());
			strcat(sentmsg," \r\n");
		}
	}
	strcat(sentmsg,":mircd 394 ");
	strcat(sentmsg,(users[id].nickname).c_str());
	strcat(sentmsg," :End of users\r\n");
}

void privatemsg(int id)
{
   bool sendpacket = true;
   if(args.size()>2)
		{
			bool channel_exist =false;
			int i,j,k;
			string name,text;
			name.clear();text.clear();
			name =args[1];
			for(i=2;i<args.size();i++)
			{
				text +=args[i];
				text+=" ";
			}
			text.pop_back();
			
			for(i=0;i<channellist.size();i++)
			{
				if(name==channellist[i].channame)
				{
				   channel_exist =true;
					break;
				}	
			}
			if(channel_exist)
			{
				sendpacket =false;
				strcpy(sentmsg,":");
				strcat(sentmsg,nickname[id].c_str());
				strcat(sentmsg," PRIVMSG ");
				strcat(sentmsg,name.c_str());
				strcat(sentmsg," ");
				strcat(sentmsg,text.c_str());
				strcat(sentmsg,"\r\n");
				for(j=0;j<(channellist[i].chanuser).size();j++)
				{
					for(int k=0;k<1024;k++)//find other user in the channel send
					{
						if(channellist[i].chanuser[j] == users[k].nickname && k!=id)
						{
							send(client_socket[k],sentmsg,strlen(sentmsg),0);
						}
					}
				}
			}
			else
			{						
				strcpy(sentmsg,":mircd 401 ");
				strcat(sentmsg,nickname[id].c_str());
				strcat(sentmsg," ");
				strcat(sentmsg,name.c_str());
				strcat(sentmsg," :No such nick/channel\r\n");
			}
		}
		else if(args.size()==2)
		{
			strcpy(sentmsg,":mircd 412 ");
			strcat(sentmsg,nickname[id].c_str());
			strcat(sentmsg, " :No text to send \r\n");
		}
		else
		{
			strcpy(sentmsg,":mircd 411 ");
			strcat(sentmsg,nickname[id].c_str());
			strcat(sentmsg, " :No recipient given (PRIVMSG)\r\n");
		}
}	

void createchannel(string channelname,int id)
{
   int i=0;
   channel nchan;
   auto it = allchannel.begin();
   if(it == allchannel.end())
   {
      if(channelname[0] != '#')
      {
         strcat(sentmsg,"Invalid channelname");
      }
      nchan.channame = channelname;
      (nchan.topic).clear();
      (nchan.chanuser).push_back(users[id].nickname);
      //allchannel.insert(pair<string,string>((nchan.channame),(nchan.chanuser)));
      channellist.push_back(nchan);
   }
   
}

void init()
{
   memset(rcvmsg,'\0',sizeof(rcvmsg));
   users.clear();
   channellist.clear();
   args.clear();
   user_info _user;
   channel _channel;
   _user.port =0;
   (_user.realname).clear();
   (_user.hostname).clear();
   (_user.servername).clear();
   (_user.nickname).clear();
   (_user.username).clear();
   _user.regnick = false;
   _user.regnick = false;
   (_channel.channame).clear();
   (_channel.topic).clear();
   (_channel.chanuser).clear();
   for( int i=0; i<1024; i++)
   {
      users.push_back(_user);
   }
}

void commandread(int id)
{
   stringstream ss;
   memset(sentmsg,'\0',sizeof(sentmsg));
   ss.clear();
   ss.str("");
   ss << rcvmsg;
   string str;
   str.clear();
   //memset(rcvmsg,'\0',sizeof(rcvmsg));
   //cout<<rcvmsg<<endl;

   for(int i=0; i<strlen(rcvmsg); i++)
   {
      int j=rcvmsg[i];
      cout<< i << " "<<rcvmsg[i]<<" "<<j<<endl;
   }
   memset(rcvmsg,'\0',sizeof(rcvmsg));
   args.clear();
   string s;
   s.clear();
   bool sendpacket= true;
   int k=0;
   while(ss >> s)
   {
      args.push_back(s);
      cout<<k++<<" "<<s<<endl;
      s.clear();
   }
   if(args[0] == "NICK")
   {
      
      if(args.size() == 2 || args.size() == 3)
      {
         int i;
         string nickn = args[1];
         for( i=0;i<1024;i++)
         {
            if(users[i].nickname == args[1])
            {  
               break;
            }
         }
         if(i == 1024)
         {
            users[id].nickname = args[1];
            nickname.insert(pair<int,string>(id,users[id].nickname));
            users[id].regnick =true;
         }
         else if(i==1024 && users[id].regnick)
         {
            sendpacket = false;
            string prev_name = nickname[id];
            nickname[id] = args[1];
            strcpy(sentmsg,":");
            strcat(sentmsg, prev_name.c_str());
            strcat(sentmsg," NICK ");
            strcat(sentmsg,args[1].c_str());
            strcat(sentmsg,"\r\n");
            for(int i=0;i<1024;i++)
            {
               if(users[i].regnick)
               {
                  send(client_socket[i],sentmsg,strlen(sentmsg),0);
               }
            }

         }
         else
         {
            strcpy(sentmsg,":mircd 436 ");
            strcat(sentmsg, (users[id].nickname).c_str());
            strcat(sentmsg, " :NICKNAME collision KILL\r\n");
         }
      }
      else
      {
         strcpy(sentmsg,":mircd 431 :No nickname given\r\n");
      }
   }
   else if(args[0] == "USER")
   {
      
      if(args.size()>4)
      {
         string str;
         users[id].username = args[1];
         username[id] = args[1];
         count++;
         users[id].hostname = args[2];
         users[id].servername = args[3];
         
         str.clear();
         for(int i=4;i<args.size();i++)
         {
            str += args[i];
            str +="";

         }
         str.pop_back();
         users[id].realname = str;
         users[id].reguser = true;
         char num[60];
         sprintf(num,":There are %d users and 0 invisible on 1 server\r\n",count);
         strcpy(sentmsg,":mircd 001 ");
         strcat(sentmsg,nickname[id].c_str());
         strcat(sentmsg," :Welcome to the minimized IRC daemon!!!\r\n");
         send(client_socket[id],sentmsg,strlen(sentmsg),0);
         memset(sentmsg,'\0',sizeof(sentmsg));
         strcat(sentmsg,":mircd 251 ");
         strcat(sentmsg,nickname[id].c_str());
         strcat(sentmsg,num);
         strcat(sentmsg,":mircd 375 ");
         strcat(sentmsg,nickname[id].c_str());
         strcat(sentmsg," :- mircd Message of the day -\r\n");
         strcat(sentmsg,":mircd 372 ");
         strcat(sentmsg,nickname[id].c_str());
         strcat(sentmsg," : - Welcome to my MIRCD server - \r\n");
         strcat(sentmsg,":mircd 376 ");
         strcat(sentmsg,nickname[id].c_str());
         strcat(sentmsg," : -End of message of the day - \r\n");
      }
      else
      {
         strcpy(sentmsg, ":mircd 461 ");
         strcat(sentmsg, (users[id].nickname).c_str());
         strcat(sentmsg, " :Not Enough parameters");
      }
   }
   else if(users[id].regnick && users[id].reguser)
   {
      if(args[0] =="PING")
      {
         strcpy(sentmsg, "PONG ");
         strcat(sentmsg,args[1].c_str());
         strcat(sentmsg,"\r\n");
      }

      else if(args[0] == "LIST")
      {
         /*string alllist(":");
         auto it = allchannel.begin();
         while(it != allchannel.end())
         {
            for(const auto &s : allchannel)
            {
               alllist.append(s.first);
               it++;
               
            }
         }*/
         int i,j;
         strcpy(sentmsg,":mircd 321 ");
			strcat(sentmsg,nickname[id].c_str());
			strcat(sentmsg," Channel :Users Name\r\n");
         if(args.size()==1)
         {
            for(i=0;i<channellist.size();i++)
				{
					strcat(sentmsg,":mircd 322 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,(channellist[i].channame).c_str());
					char p[8]; memset(p,'\0',sizeof(p));
					sprintf(p," %lu ",(channellist[i].chanuser).size());
					strcat(sentmsg,p);
					strcat(sentmsg,(channellist[i].topic).c_str());
					strcat(sentmsg,"\r\n");
				}
         }
         else
         {
            bool found=false;
            for(i=0;i<channellist.size();i++)
            {
               if(channellist[i].channame == args[1])
               {
                  found=true;
                  break;
               }
            }
            if(found)
            {
               strcat(sentmsg,":mircd 322 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,args[1].c_str());
					char p[8]; memset(p,'\0',sizeof(p));
					sprintf(p," %lu ",(channellist[i].chanuser).size());
					strcat(sentmsg,p);
					strcat(sentmsg,(channellist[i].topic).c_str());
					strcat(sentmsg,"\r\n");  
            }
         }
         //LISTEND
         strcat(sentmsg,":mircd 323 ");
			strcat(sentmsg,nickname[id].c_str());
			strcat(sentmsg," :End of List\r\n");
      }
      else if(args[0] == "JOIN")
      {
         int i;
			bool findchannel =false; bool finduser=false;
			for( i=0;i<channellist.size();i++)
			{
				if(channellist[i].channame == args[1])
				{
					findchannel =true;
					break;
				}
			}
			if(findchannel) //channel exist
			{
				for(int j=0;j<(channellist[i].chanuser).size();j++)
				{
					if(channellist[i].chanuser[j] == users[id].nickname)
					{
						finduser =true;
						break;
					}
				}
				if(!finduser) // first join this channel
				{
					(channellist[i].chanuser).push_back(users[id].nickname);
				}
			}
			else
			{
				createchannel(args[1],id);
			}
         if(args.size()>1)
         {
            memset(other_client,0,sizeof(other_client));
            strcpy(sentmsg,":");
            strcat(sentmsg,nickname[id].c_str());
            strcat(sentmsg," JOIN ");
            strcat(sentmsg,args[1].c_str());
            strcat(sentmsg,"\r\n");
            send(client_socket[id],sentmsg,strlen(sentmsg),0);
            strcpy(other_client,sentmsg);
            for(const auto &ptr : nickname)
            {
               for(int m=0;m<(channellist[i].chanuser).size();m++)
               {
                  if(ptr.second == channellist[i].chanuser[m])
                  {
                     if(ptr.first!=id)
                     {
                        send(client_socket[ptr.first],other_client,strlen(other_client),0);
                     }
                  }
               }
            }
            memset(sentmsg,'\0',sizeof(sentmsg));
            strcpy(sentmsg,":mircd 331 ");
            strcat(sentmsg,(users[id].nickname).c_str());
            strcat(sentmsg," ");
            strcat(sentmsg,args[1].c_str());
            strcat(sentmsg," ");
            if((channellist[i].topic).size()==0 || !findchannel) 
            {
               strcat(sentmsg,":No topic is set\r\n");
            }
            else
            {
               strcat(sentmsg,(channellist[i].topic).c_str());
               strcat(sentmsg,"\r\n");
            }
            send(client_socket[id],sentmsg,strlen(sentmsg),0);
            memset(sentmsg,'\0',sizeof(sentmsg));
            strcpy(sentmsg,":mircd 353 ");
            strcat(sentmsg,(users[id].nickname).c_str());
            strcat(sentmsg," ");
            strcat(sentmsg,args[1].c_str());
            strcat(sentmsg," :");
            if((channellist[i].chanuser).size())
            {
               for(int k=0;k<(channellist[i].chanuser).size();k++)
               {
                  strcat(sentmsg,(channellist[i].chanuser[k]).c_str());
                  strcat(sentmsg," ");
               }
            }
            strcat(sentmsg,"\r\n");
            send(client_socket[id],sentmsg,strlen(sentmsg),0);
            memset(sentmsg,'\0',sizeof(sentmsg));
            strcpy(sentmsg,":mircd 366 ");
            strcat(sentmsg,(users[id].nickname).c_str());
            strcat(sentmsg," ");
            strcat(sentmsg,args[1].c_str());
            strcat(sentmsg," :End of Names List\r\n");
         }
         else
         {
            strcpy(sentmsg,":mircd 461 ");
				strcat(sentmsg,nickname[id].c_str());
				strcat(sentmsg, " JOIN :Not enough parameters\r\n");
         } 
      }
      else if(args[0] == "TOPIC")
      {
         if(args.size()>1)
         {
            int i,j;
            bool user_exist = false, channel_exist=false;
            for(i=0;i<channellist.size();i++)
            {
               if(channellist[i].channame == args[1]);
               {
                  channel_exist =true;
                  break;
               }
            }
            if(channel_exist == true)
            {
               //check if the user is in the server or not
               if(args.size() == 2)
               {
                  strcpy(sentmsg,":mircd 331 ");
						strcat(sentmsg,nickname[id].c_str());
						strcat(sentmsg," ");
						strcat(sentmsg,args[1].c_str());
						strcat(sentmsg," :No topic is set\r\n");
               }
               for(j=0;j<(channellist[i].chanuser).size();j++)
               {
                  if((channellist[i].chanuser[j]) == nickname[id])
                  {
                     user_exist =true ;
                     break;
                  }
               }
               if(user_exist)
               {
                  strcpy(sentmsg,":mircd 332 ");
						strcat(sentmsg,nickname[id].c_str());
						strcat(sentmsg," ");
						strcat(sentmsg,args[1].c_str());
						strcat(sentmsg," ");
						string str; str.clear();
						str+=":";
						for(int k=2;k<args.size();k++)
						{
							str+=args[k];
							str+=" ";
						}
						str.pop_back();
						if(str.size()==0)
						{
							strcat(sentmsg,(channellist[i].topic).c_str());
							strcat(sentmsg,"\r\n");
						}
                  else
						{
							strcat(sentmsg,str.c_str());
							strcat(sentmsg,"\r\n");
							channellist[i].topic= str;
						}
               }
               else
               {
                  strcpy(sentmsg,":mircd 442 ");
						strcat(sentmsg,nickname[id].c_str());
						strcat(sentmsg," ");
						strcat(sentmsg,args[1].c_str());
						strcat(sentmsg," :You are not on that channel\r\n");
               }
            }
            else
            {
               strcpy(sentmsg,":mircd 403 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,args[1].c_str());
					strcat(sentmsg," :No such channel\r\n");
            }
         }
         else
			{
				strcpy(sentmsg,":mircd 461 ");
				strcat(sentmsg,nickname[id].c_str());
				strcat(sentmsg, " TOPIC :Not enough parameters\r\n");
			}
      }
      else if(args[0] == "NAMES")
      {
         int i,j;
         sendpacket =false;
         
         if(args.size()>1)
         {
            bool channel_exist=false, user_exist=false;
            string name,s;
            name.clear();
            s.clear();
            name= args[1];
            for(i=0;i<channellist.size();i++)
            {
               if(name == channellist[i].channame)
               {
                  channel_exist = true;
                  break;
               }  
            }
            if(channel_exist )
            {
               
               strcpy(sentmsg,":mircd 353 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,args[1].c_str());
					strcat(sentmsg," :");
					for( j=0;j<(channellist[i].chanuser).size();j++)
					{
						s+= (channellist[i].chanuser[j]).c_str();
						s+=" ";
						//strcat(sentmsg,(channellist[i].chanuser[j]).c_str());
					}
					if(str.size())str.pop_back();
					strcat(sentmsg,str.c_str());
					strcat(sentmsg,"\r\n");
					send(client_socket[id],sentmsg,strlen(sentmsg),0);
               str.clear();
					memset(sentmsg,'\0',sizeof(sentmsg));
					strcpy(sentmsg,":mircd 366 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,args[1].c_str());
					strcat(sentmsg," :End of Names List\r\n");
					send(client_socket[id],sentmsg,strlen(sentmsg),0);
            }
            else
            {
               strcpy(sentmsg,":mircd 366 ");
               strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,args[1].c_str());
					strcat(sentmsg," :End of Names List\r\n");
					send(client_socket[id],sentmsg,strlen(sentmsg),0);
            }
         }
         else
         {
            for(i=0; i<channellist.size(); i++)
				{
					strcpy(sentmsg,":mircd 353 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,(channellist[i].channame).c_str());
					strcat(sentmsg," :");
					for( j=0;j<(channellist[i].chanuser).size();j++)
					{
						str+= (channellist[i].chanuser[j]).c_str();
						str+=" ";
						//strcat(sentmsg,(channellist[i].chanuser[j]).c_str());
					}
					if(str.size())str.pop_back();
					strcat(sentmsg,str.c_str());
					strcat(sentmsg,"\r\n");
					send(client_socket[id],sentmsg,strlen(sentmsg),0);
					memset(sentmsg,'\0',sizeof(sentmsg));
					strcpy(sentmsg,":mircd 366 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,(channellist[i].channame).c_str());
					strcat(sentmsg," :End of Names List\r\n");
					send(client_socket[id],sentmsg,strlen(sentmsg),0);
				}
         }
      }
      else if (args[0] == "PART")
      {
         if(args.size()>1)
			{
				int i,j;
            string s;
				bool in_channel=false, channel_exist =false;
				for( i=0;i<channellist.size();i++)
				{
					if(channellist[i].channame == args[1])
					{
						channel_exist = true;
						break;
					}
				}
				if(channel_exist)
				{
					for( j=0;j<(channellist[i].chanuser).size();j++)
					{
						if((channellist[i].chanuser[j])==nickname[id])
						{
							in_channel=true;
							break;
						}
					}
					if(in_channel)
					{
						strcpy(sentmsg,":");
						strcat(sentmsg,nickname[id].c_str());
						strcat(sentmsg," PART :");
						strcat(sentmsg,args[1].c_str());
                  strcat(sentmsg, "\r\n");
                  strcpy(other_client,sentmsg);
                  for(const auto &ptr : nickname)
                  {
                     for(int m=0;m<(channellist[i].chanuser).size();m++)
                     {
                        if(ptr.second == channellist[i].chanuser[m])
                        {
                           if(m!=id)
                           {
                              send(client_socket[m],other_client,strlen(other_client),0);
                           }
                        }
                     }
                  }
						if(j<(channellist[i].chanuser).size()-1) //users nickname is not at the last position
						{
							
							for(int k=j;k<(channellist[i].chanuser).size()-1;k++)
							{
								s.clear();
								s = channellist[i].chanuser[k];
								channellist[i].chanuser[k]=channellist[i].chanuser[k+1];
								channellist[i].chanuser[k+1] =s;
							}
						}
						(channellist[i].chanuser).pop_back();
					}
					else
					{
						strcpy(sentmsg,":mircd 442 ");
						strcat(sentmsg,nickname[id].c_str());
						strcat(sentmsg," ");
						strcat(sentmsg,args[1].c_str());
						strcat(sentmsg," :You are not on that channel\r\n");
					}
				}
				else
				{
					strcpy(sentmsg,":mircd 442 ");
					strcat(sentmsg,nickname[id].c_str());
					strcat(sentmsg," ");
					strcat(sentmsg,args[1].c_str());
					strcat(sentmsg," :You are not on that channel\r\n");
				}
			}
			else
			{
				strcpy(sentmsg,":mircd 461 ");
				strcat(sentmsg,nickname[id].c_str());
				strcat(sentmsg, " PART :Not enough parameters\r\n");
			}
      }
      else if(args[0] == "USERS")
      {
         listalluser(id);
      }
      else if(args[0] == "PRIVMSG")
      {
         privatemsg(id);
      }
      else if(args[0] == "QUIT")
      {
         users[id].port =0;
			(users[id].realname).clear();
			(users[id].hostname).clear();
			(users[id].servername).clear();
			(users[id].nickname).clear();
			(users[id].username).clear();
			ipaddr[id].clear();
			users[id].regnick = false;
			users[id].reguser= false;
			user_num--;
			int sd =client_socket[id];
   		client_socket[id]=0;
   			
		}
		else
		{
			strcpy(sentmsg,":mircd 421  ");
			strcat(sentmsg,(users[id].nickname).c_str());
			strcat(sentmsg," ");
			strcat(sentmsg,args[0].c_str());
			strcat(sentmsg," :Unknown command\r\n");
			//strcpy(sentmsg,"421: ERR_UNKNOWNCOMMAND\r\n");
		}
   }
   else
   {
         /*strcpy(sentmsg,":mircd 421  ");
         strcat(sentmsg,args[0].c_str());
         strcat(sentmsg," :Unknown command\r\n");*/
         strcpy(sentmsg,"421: ERR_UNKNOWNCOMMAND\r\n");
   }
   if(sendpacket)send(client_socket[id],sentmsg,strlen(sentmsg),0);
   cout<<sentmsg;
	for (int k=0;k<args.size();k++)
	{
		args[k].clear();
	}
	   args.clear();
   
}
 
int main(int argc , char *argv[])
{
   int opt = TRUE;
   int master_socket , addrlen , new_socket , max_clients = 1024 , activity, i , valread , sd;
   int max_sd;
   struct sockaddr_in address;
   
   char buffer[1025]; //data buffer of 1K
   //set of socket descriptors
   fd_set readfds;
      
   //a message
   char message[1024];
   char a1[8]; char a2[8]; char a3[8]; char a4[8];
   strcpy(message,"Welcome entering irc simple server!\r\n");
   //strcpy(message,":mircd 372 user1 :-  Hello, World! :mircd 372 user1 :-               @                    _ :mircd 372 user1 :-   ____  ___   _   _ _   ____.     | | :mircd 372 user1 :-  /  _ `'_  \ | | | '_/ /  __|  ___| | :mircd 372 user1 :-  | | | | | | | | | |   | |    /  _  | :mircd 372 user1 :-  | | | | | | | | | |   | |__  | |_| | :mircd 372 user1 :-  |_| |_| |_| |_| |_|   \____| \___,_| :mircd 372 user1 :-  minimized internet relay chat daemon :mircd 372 user1 :-");
   //initialise all client_socket[] to 0 so not checked

   
   //create a master socket
   if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
   {
      perror("socket failed\n");
      exit(EXIT_FAILURE);
   }
   
   //set master socket to allow multiple connections ,
   //this is just a good habit, it will work without this
   if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
   {
      perror("setsockopt failed\n");
      exit(EXIT_FAILURE);
   }
   
   //type of socket created
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons( PORT );
   
   //bind the socket to localhost port 8888
   if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)
   {
   perror("bind failed");
   exit(EXIT_FAILURE);
   }
   printf("Listener on port %d \n", PORT);
   
   //try to specify maximum of 1005 pending connections for the master socket
   if (listen(master_socket, 1005) < 0)
   {
   perror("listen");
   exit(EXIT_FAILURE);
   }
   
   //accept the incoming connection
   addrlen = sizeof(address);
   //puts("Waiting for connections ...");

   init();
   while(TRUE)
   {
      char userstr1[16];
      char time1[1024];
      //clear the socket set
      FD_ZERO(&readfds);
      
      //add master socket to set
      FD_SET(master_socket, &readfds);
      max_sd = master_socket;
         
      //add child sockets to set
      for ( i = 0 ; i < max_clients ; i++)
      {
         //socket descriptor
         sd = client_socket[i];
         
         //if valid socket descriptor then add to read list
         if(sd > 0)
         FD_SET( sd , &readfds);
         
         //highest file descriptor number, need it for the select function
         if(sd > max_sd)
         max_sd = sd;
      }
      
      activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
      //printf("%d\n",activity);
      if ((activity < 0) && (errno!=EINTR))
      {
         printf("select error.\n");
      }
         
      //If something happened on the master socket ,
      //then its an incoming connection
      if (FD_ISSET(master_socket, &readfds))
      {
         if ((new_socket = accept(master_socket,(struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
         {
         perror("accept error.\n");
         exit(EXIT_FAILURE);
         }
         
         //inform user of socket number - used in send and receive commands
         printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
         
         //add new socket to array of sockets
         for (i = 0; i < max_clients; i++)
         {
            //if position is empty
            if( client_socket[i] == 0 )
            {
               client_socket[i] = new_socket;
               users[i].port = ntohs(address.sin_port);
               ipaddr[i] = inet_ntoa(address.sin_addr);
               printf("Adding to list of sockets as %d\n" , i);
               break;
            }
         }
      }
         
      //else its some IO operation on some other socket
      for (i = 0; i < max_clients; i++)
      {
         sd = client_socket[i]; 
         if (FD_ISSET( sd , &readfds))
         {
            //Check if it was for closing , and also read the
            //incoming message
            if ((valread = read( sd , rcvmsg, 1024)) == 0)
            {
            //cout<<"///////////"<<endl;
            //Somebody disconnected , get his details and print
            getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
            user_num--;
            memset(sentmsg,'\0',sizeof(sentmsg));
            for(int i=0;i<1024;i++)
            {
               if(client_socket[i]==sd)
               {
               users[i].port =0;
               break;
               }
            }
            cout<<"disconnected from server!\n";
            client_socket[i] = 0;
            close(sd);
         
            }
         
            else
            {
               commandread(i);
            }
         }
      }
   }
   
 return 0;
}