#include <iostream>
#include <string> 
#include <unistd.h>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <chrono>
#include <fcntl.h>

char* const* makeStupidString(const std::vector<std::string> &args)
{
	char** argsAr = new char*[args.size()+1];
	for(unsigned int k=0;k<args.size();++k)
	{
		argsAr[k] = new char[args[k].size()+1];
		strcpy(argsAr[k], args[k].c_str());
	}
	argsAr[args.size()] = NULL;	
	return argsAr;
}

bool isIO(std::string s)
{
	return(s=="|" || s=="<" || s==">");
}
bool isDelim(std::string c)
{
	return(c==" " || isIO(c));
}


int main()
{
	std::vector<std::string> history;
	double pTime=0;
	int p[2][2]; // {OUT_READ, OUT_WRITE, IN_READ, IN_WRITE}
p[0][0] = 0;
p[0][1] = 0;
p[1][0] = 0;
p[1][1] = 0;

while(true)
{
	std::string line;
	std::string buffer;
	std::vector<std::string> cmds;
	std::vector<std::vector<std::string> > argu;
	std::vector<std::string> ioCmds;
	std::string inFile = "";
	std::string outFile = "";
	//bool shouldReset = false;
	//int origFD;

	std::cout << "[cmd]: ";


	getline(std::cin, line);

	std::stringstream ss(line);
	history.push_back(line);
	int commandNum=0;
	do{
		argu.push_back(std::vector<std::string>());

		for(int k=0;ss>>buffer && !isIO(buffer);++k)
		{
			argu[commandNum].push_back(buffer);
		}

		++commandNum;
		if(!ss.eof())
		{
			if(buffer== "<")
			{
				ss >> buffer;
				inFile = buffer;
			}
			if(buffer==">")
			{
				ss >> buffer;
				outFile = buffer;
			}
			ioCmds.push_back(buffer);
		}
	}while(!ss.eof() );


	int cmdNum=0; //To tell if we are on the first argument or the last one
	if(pipe(p[0]) < 0) exit(69);
	if(pipe(p[1]) < 0) exit(69);
	for(auto && args: argu)
	{
		if(args[0]=="exit") exit(0);
		if(args[0]=="^" && args.size()>1)
		{
			if((unsigned int)std::stoi(args[1])<history.size())
			{
				std::stringstream ss(history[std::stoi(args[1])-1]);
				args[0] = std::string();
				args = std::vector<std::string>();
				ss >> args[0];
				args.push_back(args[0]);
				for(int i=0;ss>>buffer;++i)
				{
					args.push_back(buffer);
				}
			}
			else
			{
				std::cout << "There are not that many commands in the history yet" << std::endl;
				continue;
			}
		}

		if(args[0]=="history")
		{
			int i=1;
			for(auto && h:history)
			{
				std::cout <<"("<< i++ <<") " << h << std::endl;
			}
		}
		else if(args[0]=="ptime")
		{
			std::cout <<"Time spent executing child processes: ";
			int temp = pTime;
			int us = temp%1000;
			temp /= 1000;
			int ms = temp%100;
			temp /= 1000;
			int sec = temp%1000;
			std::cout << sec << " Seconds, " << ms << " Milliseconds, " << us << " Microseconds." << std::endl;
		}
		else 
		{
			if(pipe(p[1]) < 0) exit(69); //create new output pipe, as the input pipe from the last command is useless
			auto pid = fork();

			if(pid<0){ perror("Encountered an error..."); exit(101);}

			if(!pid)
			{
				if(cmdNum==0 && argu.size()>1) 
				{
					//first process
					if(inFile!="")
					{
						int fd = open(inFile.c_str(), O_RDONLY, 0);
						p[0][0] = fd;
						dup2(p[0][0], STDIN_FILENO);
						close(fd);
						//shouldReset = true;
					}
					dup2(p[1][1],STDOUT_FILENO); // replace STDOUT with the output write
				}
				else if(cmdNum==0 && (int)argu.size()-1 <1)
				{
					//lone argument, no piping needed
					if(inFile != "")
					{
						int fd = open(inFile.c_str(), O_CREAT|O_RDWR,00200|00400);
						p[0][0] = fd;
						dup2(p[0][0], STDIN_FILENO);
						close(fd);
					}
					if(outFile != "")
					{
						int fd = open(outFile.c_str(), O_CREAT|O_RDWR,00200|00400);
						p[1][1] = fd;
						dup2(p[1][1], STDOUT_FILENO);
						close(fd);
					}
				}

				else if(cmdNum == (int)argu.size()-1 && argu.size()>1) 
				{
					//last process
					dup2(p[0][0], STDIN_FILENO); //Last process, swap input to be the previous processes output.
					if(outFile != "")
					{
						std::cerr << "Writing to file..." << std::endl;
						int fd = open(outFile.c_str(), O_CREAT|O_RDWR,00200|00400);
						p[1][1] = fd;
						dup2(p[1][1], STDOUT_FILENO);
						close(fd);
					}
				}
				else
				{
					//middle piped process
					dup2(p[0][0], STDIN_FILENO); //replace STDIN with read end of 1st pipe
					dup2(p[1][1], STDOUT_FILENO); //replace STDOUT with write of 2nd pipe
				}
				close(p[0][1]);
				close(p[1][1]);
				execvp(args[0].c_str(),makeStupidString(args));
				perror("");
				exit(69);
			}
			if(pid)
			{
				int status;
				auto start = std::chrono::steady_clock::now();
				waitpid(pid, &status, 0);
				close(p[1][1]);
				auto end = std::chrono::steady_clock::now();
				pTime+= std::chrono::duration <double, std::micro>(end-start).count();
				std::swap(p[0],p[1]);
				/*
					 if(shouldReset)
					 {
					 p[0][0] = origFD;
					 shouldReset = false;
					 }
					 std::swap(p[0],p[1]);
					 */

			}
		}
		++cmdNum;
	}
}
}
