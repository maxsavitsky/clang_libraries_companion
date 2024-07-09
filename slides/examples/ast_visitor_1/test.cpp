const int n = 1, b = 50;

int factorial(int x) {
	int a = 0;
	if (x == 1) {
		return 1;
	}
	return x * factorial(x - 1);
}
