int main();

int g() {
	return 4;
}

int f() {
	int x[10];
	int i;
	g();
	x[12] = (0xdeadbeef);
	return 0;
}

int main() {
	f();
	return 0;
}
