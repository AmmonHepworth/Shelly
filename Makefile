release: main.cpp
	g++ -std=c++14 -Werror -Wextra -Wall -g0 -O3 -o./release main.cpp
debug: main.cpp
	g++ -std=c++14 -Werror -Wextra -Wall -g3 -O0 -o./debug main.cpp
