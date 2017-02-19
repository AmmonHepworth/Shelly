#include <iostream>
#include <string> 
#include <unistd.h>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <chrono>

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

int main()
{
	std::vector<std::string> history;
	double pTime=0;

	while(true)
	{
		std::string line;
		std::string buffer;
		std::string cmd;
		std::vector<std::string> args;

		std::cout << "[cmd]: ";
		getline(std::cin, line);

		std::stringstream ss(line);
		ss >> cmd;
		history.push_back(line);
		if(cmd=="exit") exit(0);
		args.push_back(cmd);

		for(int i=0;ss>>buffer;++i)
		{
			args.push_back(buffer);
		}

		if(cmd=="^" && args.size()>1)
		{
			if((unsigned int)std::stoi(args[1])<history.size())
			{
				std::stringstream ss(history[std::stoi(args[1])-1]);
				cmd = std::string();
				args = std::vector<std::string>();
				ss >> cmd;
				args.push_back(cmd);
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


		if(cmd=="history")
		{
			int i=1;
			for(auto && h:history)
			{
				std::cout <<"("<< i++ <<") " << h << std::endl;
			}
		}
		else if(cmd=="ptime")
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
			auto pid = fork();

			if(pid<0) perror("Encountered an error...");

			if(!pid)
			{
				execvp(cmd.c_str(),makeStupidString(args));
				perror("");
				exit(69);
			}
			if(pid)
			{
				int status;
				auto start = std::chrono::steady_clock::now();
				waitpid(pid, &status, 0);
				auto end = std::chrono::steady_clock::now();
				pTime+= std::chrono::duration <double, std::micro>(end-start).count();
			}
		}
	}
}
