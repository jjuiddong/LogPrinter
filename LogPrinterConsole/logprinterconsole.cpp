
#include <iostream>
#include <fstream>

using namespace std;


void main(char argc, char *argv[])
{
	if (argc < 2)
	{
		cout << "commandline <filename>" << endl;
		return;
	}

	ifstream ifs(argv[1]);
	if (!ifs.is_open())
		return;

	
	while (1)
	{



	}

}
