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
	int M; //block�ϳ��� �� �� �ִ� entry�� ��
	int t; // split ����
	int nnum; // �� block�� ����
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

//��������� ������ �о��
void Bptree::readHeader(const char* fileName) {
	string fName = fileName;
	bfile.open(fName, ios::binary | ios::out | ios::in);

	bfile.read(reinterpret_cast<char*>(&blockSize), 4);
	bfile.read(reinterpret_cast<char*>(&rootBID), sizeof(rootBID));
	bfile.read(reinterpret_cast<char*>(&depth), sizeof(depth));

	bfile.seekg(0, ios::end); // ������ ������ ��ġ ������ �̵�
	nnum = bfile.tellg();
	nnum = (nnum - 12) / blockSize; //�� ����� ��

	M = (blockSize - 4) / 8; //�ϳ��� ��忡 �� �� �ִ� ���ڵ� ��
	t = (M + 1) / 2 + 1;//split ���� 
	
}

void Bptree::afterInsert() {
	bfile.seekp(4, ios::beg);
	bfile.write(reinterpret_cast<char*>(&rootBID), 4);
	bfile.write(reinterpret_cast<char*>(&depth), 4);
}


void Bptree::insert(int key, int value) {
	int zero = 0;
	/*cout << "input : " << key << " , " << value << "\n";*/
	if (nnum == 0) {//��尡 �ϳ��� ���� ���
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
	else if (nnum == 1) {//��尡 �ϳ��� ��� : leaf == root
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
		/*cout << "�Է°� : " << key << " , " << value << "\n";*/

		if (v.size() > M) {//split �ʿ�
			int leftchild = 1;
			int rightchild = 2;
			rootBID = 3;
			bfile.seekp(12, ios::beg);
			for (int i = 0; i < M; i++) {//t-1��°������ ���� ��忡 ����, t��°���ʹ� ������ �ڽ� ��忡 ����
				if (i <= t - 1) {
					bfile.write(reinterpret_cast<char*>(&v[i].first), 4);
					bfile.write(reinterpret_cast<char*>(&v[i].second), 4);
				}
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}
			bfile.write(reinterpret_cast<char*>(&rightchild), 4);//������ �ڽ� ������
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
			//�θ��� (���ο� root���)
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
		else {//split�� �ʿ� ����
			bfile.seekp(12);
			for (int i = 0; i < v.size(); i++) {
				bfile.write(reinterpret_cast<char*>(&v[i].first), 4);
				bfile.write(reinterpret_cast<char*>(&v[i].second), 4);
				/*cout << v[i].first << " , " << v[i].second << "\n";*/
			}
		}
	}

	else {//��尡 �ΰ� �̻� -> ��Ʈ�������� insert�� ��ġ�� ã�ƾ� �Ѵ�

		vector<int> bids = findloc(key);//insert�� ��ġ�� ã�ƿ´�

		int i = bids.size() - 1;
		/*cout << "findloc ����\n";*/
		pair<int, int> pairs = insertAction(key, value, bids[i--], true); //insert �����ϰ� split�� �Ͼ���� Ȯ��

		while (pairs.first && pairs.second && i >= 0) {
			pairs = insertAction(pairs.first, pairs.second, bids[i--], false);
		}

	}

	afterInsert();
}


vector<int> Bptree::findloc(int key) {//insert�� ����� ��ġ�� ã�� �������� ��ο� �ִ� node���� bid�� ����
	vector<int> v;
	int bid = rootBID;
	int d = 0;
	while (1) {//��Ʈ�������� �ڽ����� ������
		vector<int> tmp;
		v.push_back(bid);
		if (d == this->depth) break; //leaf����� ���
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
	if (isleaf) {//leaf node�� ���
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
		if (tmp.size() > M) {//split�� �ʿ��� ���
			int rightbid;
			/*cout << "leaf node with split\n";*/

			bfile.read(reinterpret_cast<char*>(&rightbid), 4);//������ ����Ǿ��ִ� ��� �����͸� �����´�
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
			bfile.write(reinterpret_cast<char*>(&nextbid), 4);//���� ���� ����� blockID
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
			bfile.write(reinterpret_cast<char*>(&rightbid), 4);//��Ʈ ������ �������ش�


			return { tmp[t].first,nextbid };//parent ��忡 �����ؾ� �� key,bid ����
		}
		else {
			/*cout << "leaf node without split\n";*/
			bfile.seekp((bid - 1) * blockSize + 12, ios::beg);
			for (int i = 0; i < tmp.size(); i++) {
				bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
				bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
			}
			return { 0,0 };//split�� �Ͼ�� ���� ��� 0,0 ����
		}
	}
	else {//non leaf node�� ���

		bfile.seekg((bid - 1) * blockSize + 16, ios::beg); //������ �̵� -> �ش� ����� ù��° int���� �ٲ��� ����(ó�� key���� ���� �ڽ� ������)
		for (int i = 0; i < M; i++) {
			int x1, x2;
			bfile.read(reinterpret_cast<char*>(&x1), sizeof(x1));
			bfile.read(reinterpret_cast<char*>(&x2), sizeof(x2));
			if (!x1) break;
			tmp.push_back({ x1,x2 });
		}
		tmp.push_back({ key,value });
		sort(tmp.begin(), tmp.end());
		if (tmp.size() > M) {//split�� �ʿ��� ���
			/*cout << "non leaf node with split\n";*/
			bfile.seekp((bid - 1) * blockSize + 16, ios::beg);
			for (int i = 0; i < M; i++) {
				if (i <= t - 1) {//t-1��° ������ �״�� ���ش�
					bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
					bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				}
				else {//t��°���ʹ� 0���� ä��
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}//���� split �κ� �ۼ�
			int nextblock = ++nnum;//������ split �κ��� bid(���ο� ���)
			bfile.seekp(0, ios::end);
			bfile.write(reinterpret_cast<char*>(&(tmp[t].second)), 4);//������ split�� ���� �ڽ� bid��
			for (int i = t + 1; i < t + M + 1; i++) {
				if (i < tmp.size()) {
					bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
					bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				}
				else {
					bfile.write(reinterpret_cast<char*>(&zero), 4);
					bfile.write(reinterpret_cast<char*>(&zero), 4);
				}
			}//������ split�� �� �ۼ�


			if (bid == rootBID) {//split�� ��尡 ��Ʈ ����� ���

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


			return { tmp[t].first,nextblock };//split�ϸ� ����� ���� ���ο� ���ID ���� 
		}
		else {
			
			bfile.seekp((bid - 1) * blockSize + 16, ios::beg);
			for (int i = 0; i < tmp.size(); i++) {
				bfile.write(reinterpret_cast<char*>(&(tmp[i].first)), 4);
				bfile.write(reinterpret_cast<char*>(&(tmp[i].second)), 4);
				
			}
			return { 0,0 };//split�� �Ͼ�� ���� ��� 0,0 ����
		}
	}
}

void Bptree::print(char* fName) {
	bfile.seekg((rootBID - 1) * blockSize + 12, ios::beg); //root�� �̵� �� key������ ����
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
	if (output.is_open()) {//level 0 ���
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
	tmp.clear();//tmp���� �ʱ�ȭ
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
			tmp.push_back(x1);//���� 1�� key���� tmp�� ����
		}
	}
	if (output.is_open()) {//level 1 ���
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
	while (1) {//��Ʈ�������� �ڽ����� ������
		vector<int> tmp;
		
		if (d == this->depth) break; //leaf����� ��� break
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
	
	for (int i = 0; i < M; i++) {//leaf��� Ž��
		int x1, x2;
		bfile.read(reinterpret_cast<char*>(&x1), 4);
		bfile.read(reinterpret_cast<char*>(&x2), 4);
		
		if (!x1) break;
		if (x1 == key) {
			return x2;//key���� ���ٸ� �ش� entry�� value ����
		}
	}
	return -1;
}

vector<pair<int,int>> Bptree::search(int sRange, int eRange) {//range search
	int bid = rootBID;//��Ʈ ��忡�� ����
	int d = 0;

	while (d<depth) {//leaf��� ���� ������
		
		vector<int> tmp;
		
		bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
		for (int i = 0; i < 2 * M + 1; i++) {//�ش� ����� ���� �о��
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
	//range search ���� leaf ���� ��ġ
	while (1) {
		bfile.seekg(((bid - 1) * blockSize) + 12, ios::beg);
		bool flag = false;
		for (int i = 0; i < M; i++) {
			int x1, x2;
			bfile.read(reinterpret_cast<char*>(&x1), 4);
			bfile.read(reinterpret_cast<char*>(&x2), 4);
			if (x1 != 0) {//0�� ��� ����� �� ��Ʈ���̹Ƿ� ������� ����
				if (x1 >= sRange && x1 <= eRange) {//�ش� ���� ���� ���� ���̶�� ���Ϳ� �־��ش�
					result.push_back({ x1,x2 });
				}
				else if (x1 > eRange) {//�ش� ���� ���� ������ �ʰ��Ѵٸ� Ž���� �����Ѵ�.
					flag = true;
					break;
				}
			}
		}
		if (flag) break;
		
		bfile.read(reinterpret_cast<char*>(&bid), 4);//���� ���� Ž���� ����Ѵ�
		if (bid == 0) break;//leaf�� ���� ������ ���
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