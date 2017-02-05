//read number of inFile in data, and write the sum into out.txt
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

int main()
{
	fstream inFile("in.txt");
	fstream outFile("out.txt", ofstream::out);
	while (inFile)
	{
		string szLine;
		if (!getline(inFile, szLine))
			break;

		stringstream lineStream(szLine);
		int sum(0);
		while (lineStream)
		{
			string szData;
			if (!getline(lineStream, szData, ','))
				break;

			stringstream dataStream(szData);
			int data(0);
			dataStream >> data;
			sum += data;
		}
		cout << "Sum = " << sum << endl;

		//output the result
		stringstream outStream;
		outStream << "Sum = " << sum << endl;
		outFile << outStream.str();
	}

	inFile.close();
	outFile.close();

	system("pause");
	return 0;
}