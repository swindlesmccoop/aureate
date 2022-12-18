#define COLORS //comment this line to disable color output
#define SUDO "sudo" //set privilege escalation program here

#ifdef COLORS
	#define RED "\033[0;31m"
	#define GREEN "\033[0;32m"
	#define BLUE "\033[0;34m"
	#define BOLDRED "\033[1;31m"
	#define BOLDGREEN "\033[1;32m"
	#define BOLDBLUE "\033[1;34m"
	#define RESET "\033[0m"
#else
	#define RED "\033[0m"
	#define GREEN "\033[0m"
	#define BLUE "\033[0m"
	#define BOLDRED "\033[0m"
	#define BOLDGREEN "\033[0m"
	#define BOLDBLUE "\033[0m"
	#define RESET "\033[0m"
#endif
