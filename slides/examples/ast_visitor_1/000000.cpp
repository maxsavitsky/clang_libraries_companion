// B.cpp : Defines the entry point for the console application.
//
#include<iostream>
#include<vector>
#include<algorithm>

size_t N,M;
const size_t MAXN = 40000;
std::vector<std::pair<size_t,size_t> > graph[MAXN],query[MAXN],qr;
size_t p[MAXN], ancestor[MAXN];
bool u[MAXN];
size_t ansv[MAXN];
size_t cost[MAXN];

size_t find_set(size_t x){
	return x == p[x] ? x : p[x] = find_set(p[x]);
}
 
void unite(size_t a, size_t b, size_t new_ancestor){
	a = find_set(a),  b = find_set(b);
	if (rand() & 1){
		p[b] = a,  ancestor[a] = new_ancestor;
	}
	else{
		p[a] = b,  ancestor[b] = new_ancestor;
	}
}
 
void dfs(size_t v,size_t ct){
	p[v] = v,  ancestor[v] = v;
	u[v] = true;
	for(size_t i=0; i<graph[v].size(); ++i)
		if (!u[graph[v][i].first]){
			cost[graph[v][i].first] = ct + graph[v][i].second;
			dfs(graph[v][i].first,cost[graph[v][i].first]);
			unite(v,graph[v][i].first,v);
		}
	for(size_t i=0; i<query[v].size(); ++i)
		if (u[query[v][i].first])
			ansv[query[v][i].second]=ancestor[find_set(query[v][i].first)];
}

int main(int argc, char* argv[]){
	std::cin >> N;
	for(size_t i=0; i < (N-1); ++i){
		size_t a,b,c;
		std::cin >> a >> b >> c;
		--a;
		--b;
		graph[a].push_back(std::pair<size_t,size_t>(b,c));
		graph[b].push_back(std::pair<size_t,size_t>(a,c));
	}
	std::cin >> M;
	for(size_t i=0; i < M; ++i){
		size_t a,b;
		std::cin >> a >> b;
		--a;
		--b;
		qr.push_back(std::pair<size_t,size_t>(a,b));
		query[a].push_back (std::pair<size_t,size_t>(b,i));
		query[b].push_back (std::pair<size_t,size_t>(a,i));
	}
	dfs(0,0);
	for(size_t i=0; i < M; ++i){
		std::cout << (cost[qr[i].first] + cost[qr[i].second] - cost[ansv[i]] - cost[ansv[i]]) << std::endl;
	}
	return 0;
}

