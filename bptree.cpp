#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <queue>
#include <vector>
#include <algorithm>
using namespace std;


class Bptree {
public:

	int blockSize, depth, rootBID;
	int M; //block하나에 들어갈 수 있는 entry의 수
	int t; // split 기준
	int nnum; // 총 block의 개수
	fstream bfile;



	Bptree() {
	}
	~Bptree() {
		bfile.close();
	}

	void readHeader(const char* fileName);
	void afterInsert();
	void insert(int key, int value);
	vector<int> findloc(int key);
	pair<int, int> insertAction(int key, int value, int bid, bool isleaf);
	void print(char* fName);
	int search(int key);
	vector<pair<int,int>> search(int sRange, int eRange);
	
};

//파일헤더의 정보를 읽어옴
void Bptree::readHeader(const char* fileName) {
	string fName = fileName;
	bfile.open(fName, ios::binary | ios::out | ios::in);

	bfile.read(reinterpret_cast<char*>(&blockSize), 4);
	bfile.read(reinterpret_cast<char*>(&rootBID), sizeof(rootBID));
	bfile.read(reinterpret_cast<char*>(&depth), sizeof(depth));

	bfile.seekg(0, ios::end); // 파일의 끝으로 위치 포인터 이동
	nnum = bfile.tellg();
	nnum = (nnum - 12) / blockSize; //총 노드의 수

	M = (blockSize - 4) / 8; //하나의 노드에 들어갈 수 있는 레코드 수
	t = (M + 1) / 2 + 1;//split 기준 
	
}

void Bptree::afterInsert() {
	bfile.seekp(4, ios::beg);
	bfile.write(reinterpret_cast<char*>(&rootBID), 4);
	bfile.write(reinterpret_cast<char*>(&depth), 4);
}


void Bptree::insert(int key, int value) {
	int zero = 0;
	/*cout << "input : " << key << " , " << value << "\n";*/
	if (nnum == 0) {//노드가 하나도 없을 경우
		bfile.seekp(0, ios::end);

		bfile.write(reinterpret_cast<char*>(&key), sizeof(key));
		bfile.write(reinterpret_cast<char*>(&value), sizeof(value));

		/*cout << M << endl;*/
		for (int i = 1; i < M; i++) {
			bfile.write(reinterpret_cast<char*>(&zero), sizeof(zero));
			bfile.write(reinterpret_cast<char*>(&zero), sizeof(zero));
		}
		bfile.write(reinterpret_cast<char*>(&zero), sizeof(zero));
		nnum++;
		rootBID = 1;
	}
	else if (nnum == 1) {//노드가 하나인 경우 : leaf == root
		bfile.seekg(12, ios::beg);

		vector<pair<int, int>> v;

		for (int i = 0; i < M; i++) {
			int tmp1, tmp2;
			bfile.read(reinterpret_cast<char*>(&tmp1), sizeof(tmp1));
			bfile.read(reinterpret_cast<char*>(&tmp2), sizeof(tmp2));
			if (!tmp1) break;
			v.push_back({ tmp1,tmp2 });
		}
		v.push_back({ key,value });
		sort(v.begin(), v.end());
		/*cout << "입력값 : " << key << " , " << value << "\n";*/

		if (v.size() > M) {//split 필요
			int leftchild = 1;
			int rightchild = 2;
			rootBID = 3;
			bfile.seekp(12, ios::beg);
			for (int i = 0; i < M; i++) {//t-1번째까지는 현재 노드에 저장, t번째부터는 오른쪽 자식 노드에 저장
				if (i <= t - 1) {
					bfile.write(reinterpret_cast<char*>(&v[i].first), 4);
					bfile.write(reinterpret_cast<char*>(&v[i].second), 4);
				}
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}
			bfile.write(reinterpret_cast<char*>(&rightchild), 4);//오른쪽 자식 포인터
			for (int i = t; i < t + M; i++) {
				if (i < v.size()) {
					bfile.write(reinterpret_cast<char*>(&v[i].first), 4);
					bfile.write(reinterpret_cast<char*>(&v[i].second), 4);
				}
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}
			bfile.write(reinterpret_cast<char*>(&zero), 4);
			//부모노드 (새로운 root노드)
			bfile.write(reinterpret_cast<char*>(&leftchild), 4);
			bfile.write(reinterpret_cast<char*>(&v[t].first), 4);
			bfile.write(reinterpret_cast<char*>(&rightchild), 4);
			for (int i = 0; i < M - 1; i++) {
				bfile.write(reinterpret_cast<char*>(&zero), 4);
				bfile.write(reinterpret_cast<char*>(&zero), 4);
			}
			depth++;
			nnum = nnum + 2;


		}
		else {//split할 필요 없음
			bfile.seekp(12);
			for (int i = 0; i < v.size(); i++) {
				bfile.write(reinterpret_cast<char*>(&v[i].first), 4);
				bfile.write(reinterpret_cast<char*>(&v[i].second), 4);
				/*cout << v[i].first << " , " << v[i].second << "\n";*/
			}
		}
	}

	else {//노드가 두개 이상 -> 루트에서부터 insert될 위치를 찾아야 한다

		vector<int> bids = findloc(key);//insert될 위치를 찾아온다

		int i = bids.size() - 1;
		/*cout << "findloc 리턴\n";*/
		pair<int, int> pairs = insertAction(key, value, bids[i--], true); //insert 실행하고 split이 일어났는지 확인

		while (pairs.first && pairs.second && i >= 0) {
			pairs = insertAction(pairs.first, pairs.second, bids[i--], false);
		}

	}

	afterInsert();
}


vector<int> Bptree::findloc(int key) {//insert할 노드의 위치를 찾아 내려가며 경로에 있는 node들의 bid를 저장
	vector<int> v;
	int bid = rootBID;
	int d = 0;
	while (1) {//루트에서부터 자식으로 내려감
		vector<int> tmp;
		v.push_back(bid);
		if (d == this->depth) break; //leaf노드인 경우
		bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
		for (int i = 0; i < 2 * M + 1; i++) {
			int x;
			bfile.read(reinterpret_cast<char*>(&x), 4);
			if (!x) break;
			tmp.push_back(x);
		}
		int idx = 1;
		while (1) {
			if (idx >= tmp.size()) break;
			if (tmp[idx] > key) break;
			idx = idx + 2;
		}
		bid = tmp[idx - 1];
		d++;

	}
	return v;
}


pair<int, int> Bptree::insertAction(int key, int value, int bid, bool isleaf) {
	vector<pair<int, int>> tmp;
	int zero = 0;
	if (isleaf) {//leaf node인 경우
		bfile.seekg((bid - 1) * blockSize + 12);
		for (int i = 0; i < M; i++) {
			int x1, x2;
			bfile.read(reinterpret_cast<char*>(&x1), sizeof(x1));
			bfile.read(reinterpret_cast<char*>(&x2), sizeof(x2));
			if (!x1) break;
			tmp.push_back({ x1,x2 });
		}
		tmp.push_back({ key,value });
		sort(tmp.begin(), tmp.end());
		if (tmp.size() > M) {//split이 필요한 경우
			int rightbid;
			/*cout << "leaf node with split\n";*/

			bfile.read(reinterpret_cast<char*>(&rightbid), 4);//기존에 저장되어있던 노드 포인터를 가져온다
			bfile.seekp((bid - 1) * blockSize + 12, ios::beg);
			vector<int> left;
			for (int i = 0; i < M; i++) {
				if (i <= t - 1) {
					bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
					bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				}
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}
			int nextbid = ++nnum;
			bfile.write(reinterpret_cast<char*>(&nextbid), 4);//새로 만들 노드의 blockID
			bfile.seekp(0, ios::end);

			for (int i = t; i < t + M; i++) {
				if (i < tmp.size()) {
				bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
				bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				}
				/*cout << tmp[i].first << " , " << tmp[i].second << "\n";*/
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}
			bfile.write(reinterpret_cast<char*>(&rightbid), 4);//노트 포인터 연결해준다


			return { tmp[t].first,nextbid };//parent 노드에 삽입해야 할 key,bid 리턴
		}
		else {
			/*cout << "leaf node without split\n";*/
			bfile.seekp((bid - 1) * blockSize + 12, ios::beg);
			for (int i = 0; i < tmp.size(); i++) {
				bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
				bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
			}
			return { 0,0 };//split이 일어나지 않은 경우 0,0 리턴
		}
	}
	else {//non leaf node인 경우

		bfile.seekg((bid - 1) * blockSize + 16, ios::beg); //포인터 이동 -> 해당 블록의 첫번째 int값은 바뀌지 않음(처음 key값의 왼쪽 자식 포인터)
		for (int i = 0; i < M; i++) {
			int x1, x2;
			bfile.read(reinterpret_cast<char*>(&x1), sizeof(x1));
			bfile.read(reinterpret_cast<char*>(&x2), sizeof(x2));
			if (!x1) break;
			tmp.push_back({ x1,x2 });
		}
		tmp.push_back({ key,value });
		sort(tmp.begin(), tmp.end());
		if (tmp.size() > M) {//split이 필요한 경우
			/*cout << "non leaf node with split\n";*/
			bfile.seekp((bid - 1) * blockSize + 16, ios::beg);
			for (int i = 0; i < M; i++) {
				if (i <= t - 1) {//t-1번째 까지는 그대로 써준다
					bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
					bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				}
				else {//t번째부터는 0으로 채움
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}//왼쪽 split 부분 작성
			int nextblock = ++nnum;//오른쪽 split 부분의 bid(새로운 노드)
			bfile.seekp(0, ios::end);
			bfile.write(reinterpret_cast<char*>(&(tmp[t].second)), 4);//오른쪽 split의 왼쪽 자식 bid값
			for (int i = t + 1; i < t + M + 1; i++) {
				if (i < tmp.size()) {
					bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
					bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				}
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}//오른쪽 split의 값 작성


			if (bid == rootBID) {//split한 노드가 루트 노드인 경우

				bfile.write(reinterpret_cast<char*>(&bid), 4);
				bfile.write(reinterpret_cast<char*>(&(tmp[t].first)), 4);
				bfile.write(reinterpret_cast<char*>(&(nextblock)), 4);
				for (int i = 0; i < 2 * M - 2; i++) {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
				depth++;
				rootBID = ++nnum;
				return { 0,0 };
			}


			return { tmp[t].first,nextblock };//split하며 사라진 값과 새로운 블록ID 리턴 
		}
		else {
			
			bfile.seekp((bid - 1) * blockSize + 16, ios::beg);
			for (int i = 0; i < tmp.size(); i++) {
				bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
				bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				
			}
			return { 0,0 };//split이 일어나지 않은 경우 0,0 리턴
		}
	}
}

void Bptree::print(char* fName) {
	bfile.seekg((rootBID - 1) * blockSize + 12, ios::beg); //root로 이동 후 key값부터 읽음
	vector<int> tmp, point;
	int x1, x2;
	for (int i = 0; i < M; i++) {
		bfile.read(reinterpret_cast<char*>(&x1), 4);
		bfile.read(reinterpret_cast<char*>(&x2), 4);
		if (!x2) break;
		tmp.push_back(x2);
		point.push_back(x1);
	}
	point.push_back(x1);
	ofstream output;
	output.open(fName);
	if (output.is_open()) {//level 0 출력
		output << "<0>\n";
		for (int i = 0; i < tmp.size(); i++) {
			if (i == tmp.size() - 1) {
				output << tmp[i] << "\n";
			}
			else {
				output << tmp[i] << ",";
			}
		}
	}
	tmp.clear();//tmp벡터 초기화
	for (int i = 0; i < point.size(); i++) {
		int nbid = point[i];
		if (depth>1)
			bfile.seekg((nbid - 1) * blockSize + 16, ios::beg);
		else
			bfile.seekg((nbid - 1) * blockSize + 12, ios::beg);
		for (int i = 0; i < M; i++) {
			bfile.read(reinterpret_cast<char*>(&x1), 4);
			bfile.read(reinterpret_cast<char*>(&x2), 4);
			if (!x1) break;
			tmp.push_back(x1);//레벨 1의 key들을 tmp에 담음
		}
	}
	if (output.is_open()) {//level 1 출력
		output << "<1>\n";
		for (int i = 0; i < tmp.size(); i++) {
			if (i == tmp.size() - 1) {
				output << tmp[i] << "\n";
			}
			else {
				output << tmp[i] << ",";
			}
		}
	}
	
	output.close();
}



int Bptree::search(int key) {

	int bid = rootBID;
	int d = 0;
	while (1) {//루트에서부터 자식으로 내려감
		vector<int> tmp;
		
		if (d == this->depth) break; //leaf노드인 경우 break
		bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
		for (int i = 0; i < 2 * M + 1; i++) {
			int x;
			bfile.read(reinterpret_cast<char*>(&x), 4);
			if (!x) break;
			tmp.push_back(x);
		}
		int idx = 1;
		
		while (1) {
			if (idx >= tmp.size()) break;
			if (tmp[idx] > key) break;
			idx = idx + 2;
		}
		bid = tmp[idx - 1];

		d++;
	}

	bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
	
	for (int i = 0; i < M; i++) {//leaf노드 탐색
		int x1, x2;
		bfile.read(reinterpret_cast<char*>(&x1), 4);
		bfile.read(reinterpret_cast<char*>(&x2), 4);
		
		if (!x1) break;
		if (x1 == key) {
			return x2;//key값이 같다면 해당 entry의 value 리턴
		}
	}
	return -1;
}

vector<pair<int,int>> Bptree::search(int sRange, int eRange) {//range search
	int bid = rootBID;//루트 노드에서 시작
	int d = 0;

	while (d<depth) {//leaf노드 까지 내려감
		
		vector<int> tmp;
		
		bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
		for (int i = 0; i < 2 * M + 1; i++) {//해당 노드의 값을 읽어옴
			int x;
			bfile.read(reinterpret_cast<char*>(&x), 4);
			if (!x) break;
			tmp.push_back(x);
		}
		int idx = 1;

		while (1) {
			if (idx >= tmp.size()) break;
			if (tmp[idx] > sRange) break;
			idx = idx + 2;
		}
		bid = tmp[idx - 1];
		
		d++;
	}

	vector<pair<int, int>> result;
	//range search 시작 leaf 노드로 위치
	while (1) {
		bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
		bool flag = false;
		for (int i = 0; i < M; i++) {
			int x1, x2;
			bfile.read(reinterpret_cast<char*>(&x1), 4);
			bfile.read(reinterpret_cast<char*>(&x2), 4);
			if (x1 != 0) {//0인 경우 노드의 빈 엔트리이므로 고려하지 않음
				if (x1 >= sRange && x1 <= eRange) {//해당 값이 범위 안의 값이라면 벡터에 넣어준다
					result.push_back({ x1,x2 });
				}
				else if (x1 > eRange) {//해당 값이 범위 끝값을 초과한다면 탐색을 중지한다.
					flag = true;
					break;
				}
			}
		}
		if (flag) break;
		
		bfile.read(reinterpret_cast<char*>(&bid), 4);//다음 노드로 탐색을 계속한다
		if (bid == 0) break;//leaf의 끝에 도달한 경우
	}
	return result;
}

int main(int argc, char* argv[]) {

	ofstream outF;
	ifstream inF;
	char command = argv[1][0];
	char* bin = argv[2];
	char* input;
	int blockSize, initial;
	Bptree* mytree = new Bptree();
	char readFile[25];

	switch (command) {
	case 'c':
		blockSize = atoi(argv[3]);
		outF.open(bin, ios::binary);
		initial = 0;
		outF.write(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
		outF.write(reinterpret_cast<char*>(&initial), sizeof(initial));
		outF.write(reinterpret_cast<char*>(&initial), sizeof(initial));
		outF.close();

		break;
	case 'i':
		mytree->readHeader(bin);
		input = argv[3];
		inF.open(input);
		while (inF.getline(readFile, sizeof(readFile))) {
			int i = 0, key = 0, value = 0;
			for (i; readFile[i] != ','; i++) {
				key = key * 10 + (readFile[i] - '0');
			}
			for (++i; readFile[i] != '\0'; i++) {
				value = value * 10 + (readFile[i] - '0');
			}
			mytree->insert(key, value);
		}
		inF.close();
		break;
	case 's':
		mytree->readHeader(bin);
		inF.open(argv[3]);
		outF.open(argv[4]);
		while (inF.getline(readFile, sizeof(readFile))) {
			int i = 0, key = 0;
			for (i; readFile[i] != '\0'; i++) {
				key = key * 10 + (readFile[i] - '0');
			}
			int val = mytree->search(key);
			outF << key << "," << val << "\n";

		}
		inF.close();
		outF.close();
		break;
	case 'r':
		mytree->readHeader(bin);
		inF.open(argv[3]);
		outF.open(argv[4]);
		while (inF.getline(readFile, sizeof(readFile))) {
			int i = 0, st = 0, ed = 0;
			for (i; readFile[i] != ','; i++) {
				st = st * 10 + (readFile[i] - '0');
			}
			for (++i; readFile[i] != '\0'; i++) {
				ed = ed * 10 + (readFile[i] - '0');
			}
			
			vector<pair<int,int>> v = mytree->search(st, ed);
			for (int i = 0; i < v.size(); i++) {
				outF << v[i].first << "," << v[i].second << "\t";
			}
			outF << "\n";
		}
		inF.close();
		outF.close();
		break;
	case 'p':
		mytree->readHeader(bin);
		input = argv[3];
		mytree->print(argv[3]);
		break;
	}

	return 0;
}