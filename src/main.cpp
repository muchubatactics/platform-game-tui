// first, lets make terminal with custom instructions, and quit with q, then restore original terminal and exit
#include <iostream>
void cleanUp() {};

int main()
{
	std::cout << "lets see" << std::endl;
	cleanUp();
	return 0;
}