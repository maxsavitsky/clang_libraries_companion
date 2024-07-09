// C.cpp : Defines the entry point for the console application.
//
#include<iostream>
#include<map>
#include<string>
#include<vector>
#include<list>
#include<algorithm>

void init(std::map<char,char>& buttons);

class compare{
public:
	bool operator()(const std::pair<std::string, int>& a,const std::pair<std::string, int>& b)const{
		return a.second < b.second;
	}
};

void sortNotes(std::pair<std::string,std::vector<std::pair<std::string, int> > > vect){
	std::stable_sort(vect.second.begin(),vect.second.end(),compare());
}
void bubbleSort(std::list<std::pair<std::string, int> >& list, std::list<std::pair<std::string, int> >::iterator iter){
	std::list<std::pair<std::string, int> >::iterator iter2(iter);
	while((iter2 != list.begin()) && (iter2->second <= iter->second))
		--iter2;
	list.insert(iter2,*iter);
	list.erase(iter);
}
int main(int argc, char* argv[])
{
	std::map<char,char> buttons;
	init(buttons);
	std::map<std::string, std::list<std::pair<std::string, int> > > dictionary;

	{
		std::map<std::string, std::vector<std::pair<std::string, int> > > dictionaryTemp;
		std::map<std::string, std::vector<std::pair<std::string, int> > >::iterator it;

		int N;
		std::cin >> N;
		for(int i=0; i<N; ++i){
			std::string word;
			int			frequency;

			std::string wordTemp;
			std::cin >> wordTemp;
			for(size_t j=0; j<wordTemp.length(); ++j)
				word+=buttons[wordTemp[j]];
			std::cin >> frequency;

			dictionaryTemp[word].push_back(std::pair <std::string, int> (wordTemp,frequency));
		}
		dictionaryTemp["1"].push_back(std::pair <std::string, int> (".",1));
		dictionaryTemp["1"].push_back(std::pair <std::string, int> (",",1));
		dictionaryTemp["1"].push_back(std::pair <std::string, int> ("?",1));

		std::for_each(dictionaryTemp.begin(),dictionaryTemp.end(),sortNotes);

		for(it = dictionaryTemp.begin(); it != dictionaryTemp.end(); ++it)
			dictionary[it->first].assign(it->second.begin(),it->second.end());
	}

	std::string line;
	while(line.empty())
		std::getline(std::cin,line);

	std::string word;
	std::list<std::pair<std::string, int> >::iterator iter;
	bool change = false;
	for(size_t i = 0; i<line.length(); ++i){
		char c = line[i];
		if(c == '1'){
			if(change == false)
				iter = dictionary[word].begin();
			change = false;
			if(iter != dictionary[word].end())
				std::cout << iter->first;
			word.clear();
			word+="1";
		}else if(c == '*'){
			if(change == false)
				iter = dictionary[word].begin();
			++iter;
			change = true;

		}else if(c == ' '){
			if(change == false)
				iter = dictionary[word].begin();
			change = false;
			if(iter != dictionary[word].end()){
				std::cout << iter->first;
				iter->second++;
				bubbleSort(dictionary[word],iter);
			}
			std::cout << " ";
			word.clear();
		}else{
			word+=c;
		}

	}
	if(!word.empty()){
		if(change == false)
			iter = dictionary[word].begin();
		change = false;
		if(iter != dictionary[word].end())
			std::cout << iter->first;
		word.clear();
	}
	return 0;
}

void init(std::map<char,char>& buttons){
	buttons['.'] = '1';
	buttons[','] = '1';
	buttons['?'] = '1';

	buttons['a'] = '2';
	buttons['b'] = '2';
	buttons['c'] = '2';

	buttons['d'] = '3';
	buttons['e'] = '3';
	buttons['f'] = '3';

	buttons['g'] = '4';
	buttons['h'] = '4';
	buttons['i'] = '4';

	buttons['j'] = '5';
	buttons['k'] = '5';
	buttons['l'] = '5';

	buttons['m'] = '6';
	buttons['n'] = '6';
	buttons['o'] = '6';

	buttons['q'] = '7';
	buttons['r'] = '7';
	buttons['s'] = '7';

	buttons['t'] = '8';
	buttons['u'] = '8';
	buttons['v'] = '8';

	buttons['w'] = '9';
	buttons['x'] = '9';
	buttons['y'] = '9';
	buttons['z'] = '9';
}